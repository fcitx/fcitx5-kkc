//
// Copyright (C) 2013~2017 by CSSlayer
// wengxt@gmail.com
// Copyright (C) 2017~2017 by luren
// byljcron@gmail.com
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//

#include "kkc.h"
#include <fcitx-utils/fs.h>
#include <fcitx-utils/log.h>
#include <fcitx-utils/standardpath.h>
#include <fcitx/inputcontextmanager.h>
#include <fcntl.h>

FCITX_DEFINE_LOG_CATEGORY(kkc_logcategory, "kkc");

#define KKC_DEBUG() FCITX_LOGC(kkc_logcategory, Debug)

namespace fcitx {

namespace {

template <typename T>
std::unique_ptr<T, decltype(&g_object_unref)> makeGObjectUnique(T *p) {
    return {p, &g_object_unref};
}
} // namespace

class KkcState : public InputContextProperty {
public:
    KkcState(KkcEngine *parent, InputContext &ic)
        : parent_(parent), ic_(&ic),
          context_(kkc_context_new(parent->model()), &g_object_unref) {
        kkc_context_set_dictionaries(context_.get(), parent_->dictionaries());
        kkc_context_set_typing_rule(context_.get(), KKC_RULE(parent_->rule()));
    }

    KkcEngine *parent_;
    InputContext *ic_;
    std::unique_ptr<KkcContext, decltype(&g_object_unref)> context_;
};
KkcEngine::KkcEngine(Instance *instance)
    : instance_(instance),
      factory_([this](InputContext &ic) { return new KkcState(this, ic); }),
      model_(nullptr, &g_object_unref), dictionaries_(nullptr, &g_object_unref),
      userRule_(nullptr, &g_object_unref) {
#if !GLIB_CHECK_VERSION(2, 36, 0)
    g_type_init();
#endif
    kkc_init();

    fs::makePath(stringutils::joinPath(
        StandardPath::global().userDirectory(StandardPath::Type::PkgData),
        "kkc/dictionary"));
    fs::makePath(stringutils::joinPath(
        StandardPath::global().userDirectory(StandardPath::Type::PkgData),
        "kkc/rule"));

    // We can only create kkc object here after we called kkc_init().
    model_.reset(kkc_language_model_load("sorted3", NULL));
    dictionaries_.reset(kkc_dictionary_list_new());
    loadDictionary();
    loadRule();

    if (!userRule_ || !dictionaries_) {
        throw std::runtime_error("Failed to load kkc data");
    }

    instance_->inputContextManager().registerProperty("kkcState", &factory_);

    reloadConfig();
}

KkcEngine::~KkcEngine() {}
void KkcEngine::activate(const InputMethodEntry &entry,
                         InputContextEvent &event) {}
void KkcEngine::keyEvent(const InputMethodEntry &entry, KeyEvent &keyEvent) {}
void KkcEngine::reloadConfig() {}
void KkcEngine::reset(const InputMethodEntry &entry, InputContextEvent &event) {
}
void KkcEngine::save() { kkc_dictionary_list_save(dictionaries_.get()); }

void KkcEngine::updateUI(InputContext *inputContext) {}

void KkcEngine::loadDictionary() {
    kkc_dictionary_list_clear(dictionaries_.get());
    auto file = StandardPath::global().open(StandardPath::Type::PkgData,
                                            "kkc/dictionary_list", O_RDONLY);
    if (file.fd() < 0) {
        return;
    }
    std::unique_ptr<FILE, decltype(&fclose)> fp(fdopen(file.fd(), "r"),
                                                &std::fclose);
    if (!fp) {
        return;
    }
    file.release();

    char *buf = nullptr;
    size_t len = 0;

    while (getline(&buf, &len, fp.get()) != -1) {
        auto trimmed = stringutils::trim(buf);

        auto tokens = stringutils::split(stringutils::trim(buf), ",");

        if (tokens.size() < 3) {
            continue;
        }
        bool typeFile = false;
        std::string path;
        int mode = 0;
        for (auto &token : tokens) {
            auto equal = token.find('=');
            if (equal == std::string::npos) {
                continue;
            }

            auto key = token.substr(0, equal);
            auto value = token.substr(equal + 1);
            if (key == "type") {
                if (value == "file") {
                    typeFile = true;
                }
            } else if (key == "file") {
                path = value;
            } else if (key == "mode") {
                if (value == "readonly") {
                    mode = 1;
                } else if (value == "readwrite") {
                    mode = 2;
                }
            }
        }

        if (mode == 0 || path.empty() || !typeFile) {
            break;
        }

        if (mode == 1) {
            auto dict = makeGObjectUnique(reinterpret_cast<KkcDictionary *>(
                kkc_system_segment_dictionary_new(path.c_str(), "EUC-JP",
                                                  NULL)));
            if (dict) {
                KKC_DEBUG() << "Loaded readonly dict: " << path;
                kkc_dictionary_list_add(dictionaries_.get(),
                                        KKC_DICTIONARY(dict.get()));
            }
        } else {
            constexpr char configDir[] = "$FCITX_CONFIG_DIR/";
            constexpr auto len = sizeof(configDir) - 1;
            std::string realpath = path;
            if (stringutils::startsWith(path, "$FCITX_CONFIG_DIR/")) {
                realpath =
                    stringutils::joinPath(StandardPath::global().userDirectory(
                                              StandardPath::Type::PkgData),
                                          path.substr(len));
            }

            auto userdict = makeGObjectUnique(reinterpret_cast<KkcDictionary *>(
                kkc_user_dictionary_new(realpath.c_str(), NULL)));
            if (userdict) {
                KKC_DEBUG() << "Loaded user dict: " << realpath;
                kkc_dictionary_list_add(dictionaries_.get(),
                                        KKC_DICTIONARY(userdict.get()));
            }
        }
    }

    free(buf);
}

void KkcEngine::loadRule() {
    auto file = StandardPath::global().open(StandardPath::Type::PkgData,
                                            "kkc/dictionary_list", O_RDONLY);
    if (file.fd() < 0) {
        return;
    }
    std::unique_ptr<FILE, decltype(&fclose)> fp(fdopen(file.fd(), "r"),
                                                &std::fclose);
    if (!fp) {
        return;
    }
    file.release();

    char *line = nullptr;
    size_t bufsize = 0;
    getline(&line, &bufsize, fp.get());

    if (!line) {
        return;
    }

    auto rule = stringutils::trim(line);
    free(line);

    auto meta = kkc_rule_metadata_find(rule.c_str());
    if (!meta) {
        meta = kkc_rule_metadata_find("default");
    }
    if (!meta) {
        return;
    }
    std::string basePath = stringutils::joinPath(
        StandardPath::global().userDirectory(StandardPath::Type::PkgData),
        "kkc/rule");
    userRule_.reset(
        kkc_user_rule_new(meta, basePath.c_str(), "fcitx-kkc", NULL));
}

std::string KkcEngine::subMode(const InputMethodEntry &, InputContext &) {
    return "";
}

KkcState *KkcEngine::state(InputContext *ic) {
    return ic->propertyFor(&factory_);
}

} // namespace fcitx
FCITX_ADDON_FACTORY(fcitx::KkcFactory);
