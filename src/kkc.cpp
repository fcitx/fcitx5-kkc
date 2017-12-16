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
#include <fcitx-utils/standardpath.h>
#include <fcitx/inputcontextmanager.h>
#include <fcntl.h>

namespace fcitx {

namespace {

template <typename T>
std::unique_ptr<T, decltype(&g_object_unref)> makeGObjectUnique(T *p) {
    return {p, &g_object_unref};
}
}

class KKCState : public InputContextProperty {
public:
    KKCState(KKC *parent, InputContext &ic)
        : parent_(parent), ic_(&ic),
          context_(kkc_context_new(parent->model()), &g_object_unref) {
        KkcDictionaryList *kkcdicts =
            kkc_context_get_dictionaries(context_.get());
        kkc_dictionary_list_clear(kkcdicts);
        for (auto &dictionary : parent->dictionaries()) {
            kkc_dictionary_list_add(kkcdicts, KKC_DICTIONARY(dictionary.get()));
        }
    }

    KKC *parent_;
    InputContext *ic_;
    std::unique_ptr<KkcContext, decltype(&g_object_unref)> context_;
};
KKC::KKC(Instance *instance)
    : instance_(instance),
      factory_([this](InputContext &ic) { return new KKCState(this, ic); }),
      model_(nullptr, &g_object_unref) {
#if !GLIB_CHECK_VERSION(2, 36, 0)
    g_type_init();
#endif
    kkc_init();

    model_.reset(kkc_language_model_load("sorted3", NULL));
    loadDictionary();
    loadRule();

    instance_->inputContextManager().registerProperty("kkcState", &factory_);

    reloadConfig();
}

KKC::~KKC() {}
void KKC::activate(const InputMethodEntry &entry, InputContextEvent &event) {}
void KKC::keyEvent(const InputMethodEntry &entry, KeyEvent &keyEvent) {}
void KKC::reloadConfig() {}
void KKC::reset(const InputMethodEntry &entry, InputContextEvent &event) {}
void KKC::save() {}

void KKC::updateUI(InputContext *inputContext) {}

void KKC::loadDictionary() {
    dictionaries_.clear();
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
                dictionaries_.push_back(std::move(dict));
            }
        } else {
            constexpr char configDir[] = "$FCITX_CONFIG_DIR/";
            constexpr auto len = sizeof(configDir) - 1;
            std::string realpath = path;
            if (stringutils::startsWith(path, "$FCITX_CONFIG_DIR/")) {
                realpath =
                    stringutils::joinPath(StandardPath::global().userDirectory(
                                              StandardPath::Type::PkgData),
                                          "kkc", path.substr(len));
            }

            auto userdict = makeGObjectUnique(reinterpret_cast<KkcDictionary *>(
                kkc_user_dictionary_new(realpath.c_str(), NULL)));
            if (userdict) {
                dictionaries_.push_back(std::move(userdict));
            }
        }
    }

    free(buf);
}

void KKC::loadRule() {}

std::string KKC::subMode(const InputMethodEntry &, InputContext &) {
    return "";
}

KKCState *KKC::state(InputContext *ic) { return ic->propertyFor(&factory_); }

} // namespace fcitx
FCITX_ADDON_FACTORY(fcitx::KKCFactory);
