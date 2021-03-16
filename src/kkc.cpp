/*
 * SPDX-FileCopyrightText: 2013~2017 CSSlayer <wengxt@gmail.com>
 * SPDX-FileCopyrightText: 2017~2017 luren <byljcron@gmail.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 */

#include "kkc.h"
#include <fcitx-utils/fs.h>
#include <fcitx-utils/log.h>
#include <fcitx-utils/standardpath.h>
#include <fcitx/action.h>
#include <fcitx/inputcontextmanager.h>
#include <fcitx/inputpanel.h>
#include <fcitx/menu.h>
#include <fcitx/userinterfacemanager.h>
#include <fcntl.h>

FCITX_DEFINE_LOG_CATEGORY(kkc_logcategory, "kkc");

#define KKC_DEBUG() FCITX_LOGC(kkc_logcategory, Debug)

namespace fcitx {

class KkcState : public InputContextProperty {
public:
    KkcState(KkcEngine *parent, InputContext &ic)
        : parent_(parent), ic_(&ic),
          context_(kkc_context_new(parent->model())) {
        kkc_context_set_dictionaries(context_.get(), parent_->dictionaries());
        kkc_context_set_input_mode(context_.get(),
                                   *parent_->config().inputMode);
        lastMode_ = kkc_context_get_input_mode(context_.get());
        applyConfig();

        signal_ =
            g_signal_connect(context_.get(), "notify::input-mode",
                             G_CALLBACK(&KkcState::inputModeChanged), this);

        updateInputMode();
    }

    ~KkcState() {
        g_signal_handler_disconnect(context_.get(), signal_);
        // In order to workaround libkkc context clear the dictionaries upon
        // context deleting.
        kkc_context_set_dictionaries(context_.get(),
                                     parent_->dummyEmptyDictionaries());
    }

    static void inputModeChanged(GObject *, GParamSpec *, gpointer user_data) {
        auto that = static_cast<KkcState *>(user_data);
        that->updateInputMode();
    }

    void updateInputMode() {
        parent_->updateInputMode(ic_);
        auto newMode = kkc_context_get_input_mode(context_.get());
        if (newMode != lastMode_) {
            lastMode_ = newMode;
            modeChanged_ = true;
        }
    }

    void applyConfig() {
        KkcCandidateList *kkcCandidates =
            kkc_context_get_candidates(context_.get());
        kkc_candidate_list_set_page_start(
            kkcCandidates, *parent_->config().nTriggersToShowCandWin);
        kkc_candidate_list_set_page_size(kkcCandidates,
                                         *parent_->config().pageSize);
        kkc_context_set_punctuation_style(context_.get(),
                                          *parent_->config().punctuationStyle);
        kkc_context_set_auto_correct(context_.get(),
                                     *parent_->config().autoCorrect);
        if (parent_->rule()) {
            kkc_context_set_typing_rule(context_.get(),
                                        KKC_RULE(parent_->rule()));
        }
    }

    KkcEngine *parent_;
    InputContext *ic_;
    GObjectUniquePtr<KkcContext> context_;
    gulong signal_;
    bool modeChanged_ = false;
    KkcInputMode lastMode_ = KKC_INPUT_MODE_HIRAGANA;
};

namespace {

template <typename T>
auto makeGObjectUnique(T *p) {
    return UniqueCPtr<T, g_object_unref>{p};
}

struct {
    const char *icon;
    const char *label;
    const char *description;
} input_mode_status[] = {
    {"", "\xe3\x81\x82", N_("Hiragana")},
    {"", "\xe3\x82\xa2", N_("Katakana")},
    {"", "\xef\xbd\xb1", N_("Half width Katakana")},
    {"", "A_", N_("Latin")},
    {"", "\xef\xbc\xa1", N_("Wide latin")},
    {"", "A", N_("Direct input")},
};

auto inputModeStatus(KkcEngine *engine, InputContext *ic) {
    auto state = engine->state(ic);
    auto mode = kkc_context_get_input_mode(state->context_.get());
    return (mode >= 0 && mode < FCITX_ARRAY_SIZE(input_mode_status))
               ? &input_mode_status[mode]
               : nullptr;
}

class KkcModeAction : public Action {
public:
    KkcModeAction(KkcEngine *engine) : engine_(engine) {}

    std::string shortText(InputContext *ic) const override {
        if (auto status = inputModeStatus(engine_, ic)) {
            return stringutils::concat(status->label, " - ",
                                       _(status->description));
        }
        return "";
    }
    std::string longText(InputContext *ic) const override {
        if (auto status = inputModeStatus(engine_, ic)) {
            return _(status->description);
        }
        return "";
    }
    std::string icon(InputContext *ic) const override {
        if (auto status = inputModeStatus(engine_, ic)) {
            return status->icon;
        }
        return "";
    }

private:
    KkcEngine *engine_;
};

class KkcModeSubAction : public SimpleAction {
public:
    KkcModeSubAction(KkcEngine *engine, KkcInputMode mode)
        : engine_(engine), mode_(mode) {
        setShortText(
            stringutils::concat(input_mode_status[mode].label, " - ",
                                _(input_mode_status[mode].description)));
        setLongText(_(input_mode_status[mode].description));
        setIcon(input_mode_status[mode].icon);
        setCheckable(true);
    }
    bool isChecked(InputContext *ic) const override {
        auto state = engine_->state(ic);
        return mode_ == kkc_context_get_input_mode(state->context_.get());
    }
    void activate(InputContext *ic) override {
        auto state = engine_->state(ic);
        kkc_context_set_input_mode(state->context_.get(), mode_);
    }

private:
    KkcEngine *engine_;
    KkcInputMode mode_;
};

class KkcCandidateWord : public CandidateWord {
public:
    KkcCandidateWord(KkcEngine *engine, Text text, int idx)
        : CandidateWord(), engine_(engine), idx_(idx) {
        setText(std::move(text));
    }

    void select(InputContext *inputContext) const override {
        auto state = engine_->state(inputContext);
        auto context = state->context_.get();
        KkcCandidateList *kkcCandidates = kkc_context_get_candidates(context);
        if (kkc_candidate_list_select_at(
                kkcCandidates,
                idx_ % kkc_candidate_list_get_page_size(kkcCandidates))) {
            engine_->updateUI(inputContext);
        }
    }

private:
    KkcEngine *engine_;
    int idx_;
};

class KkcFcitxCandidateList : public CandidateList,
                              public PageableCandidateList,
                              public CursorMovableCandidateList {
public:
    KkcFcitxCandidateList(KkcEngine *engine, InputContext *ic)
        : engine_(engine), ic_(ic) {
        setPageable(this);
        setCursorMovable(this);
        auto kkcstate = engine_->state(ic_);
        auto context = kkcstate->context_.get();
        KkcCandidateList *kkcCandidates = kkc_context_get_candidates(context);
        gint size = kkc_candidate_list_get_size(kkcCandidates);
        gint cursor_pos = kkc_candidate_list_get_cursor_pos(kkcCandidates);
        guint page_start = kkc_candidate_list_get_page_start(kkcCandidates);
        guint page_size = kkc_candidate_list_get_page_size(kkcCandidates);

        // Assume size = 27, cursor = 14, page_start = 4, page_size = 10
        // 0~3 not in page.
        // 4~13 1st page
        // 14~23 2nd page
        // 24~26 3nd page
        int currentPage = (cursor_pos - page_start) / page_size;
        int totalPage = (size - page_start + page_size - 1) / page_size;
        int pageFirst = currentPage * page_size + page_start;
        int pageLast = std::min(size, static_cast<int>(pageFirst + page_size));

        for (int i = pageFirst; i < pageLast; i++) {
            GObjectUniquePtr<KkcCandidate> kkcCandidate(
                kkc_candidate_list_get(kkcCandidates, i));
            Text text;
            text.append(kkc_candidate_get_text(kkcCandidate.get()));
            if (*engine->config().showAnnotation) {
                auto annotation =
                    kkc_candidate_get_annotation(kkcCandidate.get());
                // Make sure annotation is not null, empty, or equal to "?".
                // ? seems to be a special debug purpose value.
                if (annotation && annotation[0] &&
                    g_strcmp0(annotation, "?") != 0) {
                    text.append(stringutils::concat(
                        " [", kkc_candidate_get_annotation(kkcCandidate.get()),
                        "]"));
                }
            }
            if (i == cursor_pos) {
                cursorIndex_ = i - pageFirst;
            }

            labels_.emplace_back(std::to_string(i - pageFirst + 1) + ". ");
            words_.emplace_back(std::make_unique<KkcCandidateWord>(
                engine, text, i - page_start));
        }

        hasPrev_ = currentPage != 0;
        hasNext_ = currentPage + 1 < totalPage;
    }

    bool hasPrev() const override { return hasPrev_; }

    bool hasNext() const override { return hasNext_; }

    void prev() override { return paging(true); }

    void next() override { return paging(false); }

    bool usedNextBefore() const override { return true; }

    void prevCandidate() override { return moveCursor(true); }

    void nextCandidate() override { return moveCursor(false); }

    const Text &label(int idx) const override { return labels_[idx]; }

    const CandidateWord &candidate(int idx) const override {
        return *words_[idx];
    }

    int size() const override { return words_.size(); }

    int cursorIndex() const override { return cursorIndex_; }

    CandidateLayoutHint layoutHint() const override {
        return *engine_->config().candidateLayout;
    }

private:
    void paging(bool prev) {
        auto kkcstate = engine_->state(ic_);
        auto context = kkcstate->context_.get();
        KkcCandidateList *kkcCandidates = kkc_context_get_candidates(context);
        if (kkc_candidate_list_get_page_visible(kkcCandidates)) {
            if (prev) {
                kkc_candidate_list_page_up(kkcCandidates);
            } else {
                kkc_candidate_list_page_down(kkcCandidates);
            }
            engine_->updateUI(ic_);
        }
    }
    void moveCursor(bool prev) {
        auto kkcstate = engine_->state(ic_);
        auto context = kkcstate->context_.get();
        KkcCandidateList *kkcCandidates = kkc_context_get_candidates(context);
        if (kkc_candidate_list_get_page_visible(kkcCandidates)) {
            if (prev) {
                kkc_candidate_list_cursor_up(kkcCandidates);
            } else {
                kkc_candidate_list_cursor_down(kkcCandidates);
            }
            engine_->updateUI(ic_);
        }
    }

    KkcEngine *engine_;
    InputContext *ic_;
    std::vector<Text> labels_;
    std::vector<std::unique_ptr<KkcCandidateWord>> words_;
    int cursorIndex_ = -1;
    bool hasPrev_ = false;
    bool hasNext_ = false;
};

Text kkcContextGetPreedit(KkcContext *context) {
    Text preedit;
    KkcSegmentList *segments = kkc_context_get_segments(context);
    if (kkc_segment_list_get_cursor_pos(segments) >= 0) {
        int offset = 0;
        for (int i = 0; i < kkc_segment_list_get_size(segments); i++) {
            auto segment = makeGObjectUnique(kkc_segment_list_get(segments, i));
            const gchar *str = kkc_segment_get_output(segment.get());
            TextFormatFlags format;
            if (i < kkc_segment_list_get_cursor_pos(segments)) {
                offset += strlen(str);
            }
            if (i == kkc_segment_list_get_cursor_pos(segments)) {
                format = TextFormatFlag::HighLight;
            } else {
                format = TextFormatFlag::Underline;
            }
            preedit.append(str, format);
        }
        preedit.setCursor(offset);
    } else {
        gchar *str = kkc_context_get_input(context);
        if (str && str[0]) {
            preedit.append(str, TextFormatFlag::Underline);
            preedit.setCursor(strlen(str));
        }
        g_free(str);
    }
    return preedit;
}

} // namespace

KkcEngine::KkcEngine(Instance *instance)
    : instance_(instance),
      factory_([this](InputContext &ic) { return new KkcState(this, ic); }) {
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
    dummyEmptyDictionaries_.reset(kkc_dictionary_list_new());

    reloadConfig();

    if (!userRule_ || !dictionaries_) {
        throw std::runtime_error("Failed to load kkc data");
    }

    modeAction_ = std::make_unique<KkcModeAction>(this);
    menu_ = std::make_unique<Menu>();
    modeAction_->setMenu(menu_.get());

    instance_->userInterfaceManager().registerAction("kkc-input-mode",
                                                     modeAction_.get());

#define _ADD_ACTION(MODE, NAME)                                                \
    subModeActions_.emplace_back(                                              \
        std::make_unique<KkcModeSubAction>(this, MODE));                       \
    instance_->userInterfaceManager().registerAction(                          \
        NAME, subModeActions_.back().get());

    _ADD_ACTION(KKC_INPUT_MODE_HIRAGANA, "kkc-input-mode-hiragana");
    _ADD_ACTION(KKC_INPUT_MODE_KATAKANA, "kkc-input-mode-katakana");
    _ADD_ACTION(KKC_INPUT_MODE_HANKAKU_KATAKANA,
                "kkc-input-mode-hankaku-katakana");
    _ADD_ACTION(KKC_INPUT_MODE_LATIN, "kkc-input-mode-latin");
    _ADD_ACTION(KKC_INPUT_MODE_WIDE_LATIN, "kkc-input-mode-wide-latin");
    for (auto &subModeAction : subModeActions_) {
        menu_->addAction(subModeAction.get());
    }
    instance_->inputContextManager().registerProperty("kkcState", &factory_);
    instance_->inputContextManager().foreach([this](InputContext *ic) {
        auto state = this->state(ic);
        kkc_context_set_input_mode(state->context_.get(), *config_.inputMode);
        return true;
    });
}

KkcEngine::~KkcEngine() {}

void KkcEngine::activate(const InputMethodEntry &, InputContextEvent &event) {
    auto &statusArea = event.inputContext()->statusArea();
    statusArea.addAction(StatusGroup::InputMethod, modeAction_.get());
}

void KkcEngine::deactivate(const InputMethodEntry &entry,
                           InputContextEvent &event) {
    if (event.type() == EventType::InputContextSwitchInputMethod) {
        auto kkcstate = this->state(event.inputContext());
        auto context = kkcstate->context_.get();
        auto text = kkcContextGetPreedit(context);
        auto str = text.toString();
        if (!str.empty()) {
            event.inputContext()->commitString(str);
        }
    }
    reset(entry, event);
}

void KkcEngine::keyEvent(const InputMethodEntry &, KeyEvent &keyEvent) {
    auto state = static_cast<uint32_t>(keyEvent.rawKey().states());
    state &= static_cast<uint32_t>(KeyState::SimpleMask);
    if (keyEvent.isRelease()) {
        state |= KKC_MODIFIER_TYPE_RELEASE_MASK;
    }

    KKC_DEBUG() << "Kkc received key: " << keyEvent.rawKey()
                << " isRelease: " << keyEvent.isRelease()
                << " keycode: " << keyEvent.rawKey().code();

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

    auto key = makeGObjectUnique(kkc_key_event_new_from_x_event(
        keyEvent.rawKey().sym(), keyEvent.rawKey().code() - 8,
        static_cast<KkcModifierType>(state)));
    if (!key) {
        KKC_DEBUG() << "Failed to obtain kkc key event";
        return;
    }
    kkcstate->modeChanged_ = false;
    if (kkc_context_process_key_event(context, key.get())) {
        keyEvent.filterAndAccept();
        updateUI(keyEvent.inputContext());
    }
    KKC_DEBUG() << "Key event filtered: " << keyEvent.filtered();
}
void KkcEngine::reloadConfig() {
    readAsIni(config_, "conf/kkc.conf");

    loadDictionary();
    loadRule();

    if (factory_.registered()) {
        instance_->inputContextManager().foreach([this](InputContext *ic) {
            auto state = this->state(ic);
            state->applyConfig();
            return true;
        });
    }
}
void KkcEngine::reset(const InputMethodEntry &, InputContextEvent &event) {
    auto state = this->state(event.inputContext());
    auto context = state->context_.get();
    kkc_context_reset(context);
    updateUI(event.inputContext());
}
void KkcEngine::save() { kkc_dictionary_list_save(dictionaries_.get()); }

void KkcEngine::updateUI(InputContext *inputContext) {
    auto state = this->state(inputContext);
    auto context = state->context_.get();

    auto &inputPanel = inputContext->inputPanel();
    inputPanel.reset();
    Text preedit = kkcContextGetPreedit(context);
    if (inputContext->capabilityFlags().test(CapabilityFlag::Preedit)) {
        inputPanel.setClientPreedit(preedit);
        inputContext->updatePreedit();
    } else {
        inputPanel.setPreedit(preedit);
    }

    KkcCandidateList *kkcCandidates = kkc_context_get_candidates(context);
    if (kkc_candidate_list_get_page_visible(kkcCandidates)) {
        inputPanel.setCandidateList(
            std::make_unique<KkcFcitxCandidateList>(this, inputContext));
    }

    if (kkc_context_has_output(context)) {
        gchar *str = kkc_context_poll_output(context);
        inputContext->commitString(str);
        g_free(str);
    }

    // Ensure we are not composing any text.
    if (state->modeChanged_ && preedit.empty() && !inputPanel.candidateList()) {
        instance_->showInputMethodInformation(inputContext);
    }

    inputContext->updateUserInterface(UserInterfaceComponent::InputPanel);
}

void KkcEngine::updateInputMode(InputContext *ic) { modeAction_->update(ic); }

void KkcEngine::loadDictionary() {
    kkc_dictionary_list_clear(dictionaries_.get());
    auto file = StandardPath::global().open(StandardPath::Type::PkgData,
                                            "kkc/dictionary_list", O_RDONLY);
    if (file.fd() < 0) {
        return;
    }
    UniqueFilePtr fp(fdopen(file.fd(), "r"));
    if (!fp) {
        return;
    }
    file.release();

    UniqueCPtr<char> buf;
    size_t len = 0;

    while (getline(buf, &len, fp.get()) != -1) {
        auto tokens = stringutils::split(stringutils::trim(buf.get()), ",");

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
}

void KkcEngine::loadRule() {
    auto meta = kkc_rule_metadata_find(config_.rule->data());
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

std::string KkcEngine::subMode(const InputMethodEntry &, InputContext &ic) {
    if (auto status = inputModeStatus(this, &ic)) {
        return _(status->description);
    }
    return "";
}

std::string KkcEngine::subModeLabelImpl(const InputMethodEntry &,
                                        InputContext &ic) {
    if (auto status = inputModeStatus(this, &ic)) {
        return _(status->label);
    }
    return "";
}

KkcState *KkcEngine::state(InputContext *ic) {
    return ic->propertyFor(&factory_);
}
} // namespace fcitx

FCITX_ADDON_FACTORY(fcitx::KkcFactory);
