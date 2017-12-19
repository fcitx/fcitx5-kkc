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
#include <fcitx/inputpanel.h>
#include <fcntl.h>

FCITX_DEFINE_LOG_CATEGORY(kkc_logcategory, "kkc");

#define KKC_DEBUG() FCITX_LOGC(kkc_logcategory, Debug)

namespace fcitx {

namespace {

template <typename T>
std::unique_ptr<T, decltype(&g_object_unref)> makeGObjectUnique(T *p) {
    return {p, &g_object_unref};
}

class KkcCandidateWord : public CandidateWord {
public:
    KkcCandidateWord(KkcEngine *engine, Text text, int idx)
        : CandidateWord(), engine_(engine), idx_(idx) {
        setText(std::move(text));
    }

    void select(InputContext *inputContext) const {
        // TODO, select candidate and update UI.
        // Port FcitxKkcGetCandWord
    }

private:
    KkcEngine *engine_;
    int idx_;
};

class KkcFcitxCandidateList : public CandidateList,
                              public PageableCandidateList,
                              public CursorMovableCandidateList {
public:
    KkcFcitxCandidateList(KkcEngine *engine, KkcCandidateList *list) {
        setPageable(this);
        setCursorMovable(this);
        // TODO fill candidate list, port from FcitxKkcGetCandWords
    }

    bool hasPrev() const override {}

    bool hasNext() const override {}

    void prev() override {}

    void next() override {}

    bool usedNextBefore() const override { return true; }

    void prevCandidate() override {}

    void nextCandidate() override {}

    const Text &label(int idx) const override {}

    std::shared_ptr<const CandidateWord> candidate(int idx) const override {}

    int size() const override {}

    int cursorIndex() const {}

    CandidateLayoutHint layoutHint() const override {}

private:
    std::vector<std::shared_ptr<KkcCandidateWord>> words_;
};

} // namespace

class KkcState : public InputContextProperty {
public:
    KkcState(KkcEngine *parent, InputContext &ic)
        : parent_(parent), ic_(&ic),
          context_(kkc_context_new(parent->model()), &g_object_unref) {
        kkc_context_set_dictionaries(context_.get(), parent_->dictionaries());
        kkc_context_set_input_mode(context_.get(),
                                   *parent_->config().inputMode);
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
        "kkc/rules"));

    // We can only create kkc object here after we called kkc_init().
    model_.reset(kkc_language_model_load("sorted3", NULL));
    dictionaries_.reset(kkc_dictionary_list_new());
    instance_->inputContextManager().registerProperty("kkcState", &factory_);

    reloadConfig();

    if (!userRule_ || !dictionaries_) {
        throw std::runtime_error("Failed to load kkc data");
    }
    instance_->inputContextManager().foreach([this](InputContext *ic) {
        auto state = this->state(ic);
        kkc_context_set_input_mode(state->context_.get(), *config_.inputMode);
        return true;
    });
}

KkcEngine::~KkcEngine() {}
void KkcEngine::activate(const InputMethodEntry &entry,
                         InputContextEvent &event) {}
void KkcEngine::keyEvent(const InputMethodEntry &entry, KeyEvent &keyEvent) {
    auto state = static_cast<uint32_t>(keyEvent.rawKey().states());
    if (keyEvent.isRelease()) {
        state |= KKC_MODIFIER_TYPE_RELEASE_MASK;
    }

    auto kkcstate = this->state(keyEvent.inputContext());
    auto context = kkcstate->context_.get();
    KkcCandidateList *kkcCandidates = kkc_context_get_candidates(context);
    if (kkc_candidate_list_get_page_visible(kkcCandidates) &&
        !keyEvent.isRelease()) {
        if (keyEvent.key().checkKeyList(*config_.cursorUpKey)) {
            kkc_candidate_list_cursor_up(kkcCandidates);
            keyEvent.filterAndAccept();
        } else if (keyEvent.key().checkKeyList(*config_.cursorDownKey)) {
            kkc_candidate_list_cursor_down(kkcCandidates);
            keyEvent.filterAndAccept();
        } else if (keyEvent.key().checkKeyList(*config_.prevPageKey)) {
            kkc_candidate_list_page_up(kkcCandidates);
            keyEvent.filterAndAccept();
        } else if (keyEvent.key().checkKeyList(*config_.nextPageKey)) {
            kkc_candidate_list_page_down(kkcCandidates);
            keyEvent.filterAndAccept();
        } else if (keyEvent.key().isDigit()) {
            KeySym syms[] = {
                FcitxKey_1, FcitxKey_2, FcitxKey_3, FcitxKey_4, FcitxKey_5,
                FcitxKey_6, FcitxKey_7, FcitxKey_8, FcitxKey_9, FcitxKey_0,
            };

            KeyList selectionKey;
            KeyStates states;
            for (auto sym : syms) {
                selectionKey.emplace_back(sym, states);
            }
            auto idx = keyEvent.key().keyListIndex(selectionKey);
            if (idx >= 0) {
                kkc_candidate_list_select_at(
                    kkcCandidates,
                    idx % kkc_candidate_list_get_page_size(kkcCandidates));
                keyEvent.filterAndAccept();
            }
        }
    }

    if (keyEvent.filtered()) {
        updateUI(keyEvent.inputContext());
        return;
    }

    GObjectUniquePtr<KkcKeyEvent> key =
        makeGObjectUnique(kkc_key_event_new_from_x_event(
            keyEvent.rawKey().sym(), keyEvent.rawKey().code() - 8,
            static_cast<KkcModifierType>(state)));
    if (!key) {
        return;
    }
    if (kkc_context_process_key_event(context, key.get())) {
        keyEvent.filterAndAccept();
        updateUI(keyEvent.inputContext());
    }
}
void KkcEngine::reloadConfig() {
    readAsIni(config_, "conf/kkc.conf");

    loadDictionary();
    loadRule();

    instance_->inputContextManager().foreach([this](InputContext *ic) {
        auto state = this->state(ic);
        KkcCandidateList *kkcCandidates =
            kkc_context_get_candidates(state->context_.get());
        kkc_candidate_list_set_page_start(kkcCandidates,
                                          *config_.nTriggersToShowCandWin);
        kkc_candidate_list_set_page_size(kkcCandidates, *config_.pageSize);
        kkc_context_set_punctuation_style(state->context_.get(),
                                          *config_.punctuationStyle);
        kkc_context_set_auto_correct(state->context_.get(),
                                     *config_.autoCorrect);
        if (rule()) {
            kkc_context_set_typing_rule(state->context_.get(),
                                        KKC_RULE(rule()));
        }
        return true;
    });
}
void KkcEngine::reset(const InputMethodEntry &entry, InputContextEvent &event) {
}
void KkcEngine::save() { kkc_dictionary_list_save(dictionaries_.get()); }

void KkcEngine::updateUI(InputContext *inputContext) {
    auto state = this->state(inputContext);
    auto context = state->context_.get();

    auto &inputPanel = inputContext->inputPanel();
    inputPanel.reset();
    Text preedit;
    // TODO: fill preedit.

    if (inputContext->capabilityFlags().test(CapabilityFlag::Preedit)) {
        inputPanel.setClientPreedit(preedit);
        inputContext->updatePreedit();
    } else {
        inputPanel.setPreedit(preedit);
    }

    KkcCandidateList *kkcCandidates = kkc_context_get_candidates(context);
    if (kkc_candidate_list_get_page_visible(kkcCandidates)) {
        inputPanel.setCandidateList(
            std::make_unique<KkcFcitxCandidateList>(this, kkcCandidates));
    }

    // TODO: Update candidate list.

    if (kkc_context_has_output(context)) {
        gchar *str = kkc_context_poll_output(context);
        inputContext->commitString(str);
        g_free(str);
    }

    inputContext->updateUserInterface(UserInterfaceComponent::InputPanel);
}

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
        "kkc/rules");
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
