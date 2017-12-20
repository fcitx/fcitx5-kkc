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
#include <fcitx/action.h>
#include <fcitx/inputcontextmanager.h>
#include <fcitx/inputpanel.h>
#include <fcitx/menu.h>
#include <fcitx/userinterfacemanager.h>
#include <fcntl.h>

FCITX_DEFINE_LOG_CATEGORY(kkc_logcategory, "kkc");

#define KKC_DEBUG() FCITX_LOGC(kkc_logcategory, Debug)

namespace fcitx {

static void _kkc_input_mode_changed_cb(GObject *gobject, GParamSpec *pspec,
                                       gpointer user_data);

class KkcState : public InputContextProperty {
public:
    KkcState(KkcEngine *parent, InputContext &ic)
        : parent_(parent), ic_(&ic),
          context_(kkc_context_new(parent->model()), &g_object_unref) {
        kkc_context_set_dictionaries(context_.get(), parent_->dictionaries());
        kkc_context_set_input_mode(context_.get(),
                                   *parent_->config().inputMode);
        applyConfig();

        signal_ =
            g_signal_connect(context_.get(), "notify::input-mode",
                             G_CALLBACK(&KkcState::inputModeChanged), this);
        updateInputMode();
    }

    ~KkcState() { g_signal_handler_disconnect(context_.get(), signal_); }

    static void inputModeChanged(GObject *, GParamSpec *, gpointer user_data) {
        auto that = static_cast<KkcState *>(user_data);
        that->updateInputMode();
    }

    void updateInputMode() { parent_->updateInputMode(ic_); }

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
    std::unique_ptr<KkcContext, decltype(&g_object_unref)> context_;
    gulong signal_;
};

namespace {

template <typename T>
std::unique_ptr<T, decltype(&g_object_unref)> makeGObjectUnique(T *p) {
    return {p, &g_object_unref};
}

struct {
    const char *icon;
    const char *label;
    const char *description;
} input_mode_status[] = {
    {"", "\xe3\x81\x82", N_("Hiragana")},
    {"", "\xe3\x82\xa2", N_("Katakana")},
    {"", "\xef\xbd\xb1", N_("Half width Katakana")},
    {"", "A", N_("Latin")},
    {"", "\xef\xbc\xa1", N_("Wide latin")},
};

auto inputModeStatus(KkcEngine *engine, InputContext *ic) {
    auto state = engine->state(ic);
    auto mode = kkc_context_get_input_mode(state->context_.get());
    return (mode >= 0 || mode < FCITX_ARRAY_SIZE(input_mode_status))
               ? &input_mode_status[mode]
               : nullptr;
}

class KkcModeAction : public Action {
public:
    KkcModeAction(KkcEngine *engine) : engine_(engine) {}

    std::string shortText(InputContext *ic) const override {
        if (auto status = inputModeStatus(engine_, ic)) {
            return status->label;
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
        setShortText(input_mode_status[mode].label);
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
            GObjectUniquePtr<KkcCandidate> kkcCandidate =
                makeGObjectUnique(kkc_candidate_list_get(kkcCandidates, i));
            Text text;
            text.append(kkc_candidate_get_text(kkcCandidate.get()));
            if (*engine->config().showAnnotation &&
                kkc_candidate_get_annotation(kkcCandidate.get())) {

                text.append(stringutils::concat(
                    " [", kkc_candidate_get_annotation(kkcCandidate.get()),
                    "]"));
            }
            if (i == cursor_pos) {
                cursorIndex_ = i - pageFirst;
            }

            labels_.push_back(Text(std::to_string(i - pageFirst + 1) + ". "));
            words_.push_back(std::make_shared<KkcCandidateWord>(
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

    std::shared_ptr<const CandidateWord> candidate(int idx) const override {
        return words_[idx];
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
    std::vector<std::shared_ptr<KkcCandidateWord>> words_;
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
            GObjectUniquePtr<KkcSegment> segment =
                makeGObjectUnique(kkc_segment_list_get(segments, i));
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
            preedit.append(str);
            preedit.setCursor(strlen(str));
        }
        g_free(str);
    }
    return preedit;
}

} // namespace

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
    auto &statusArea = event.inputContext()->statusArea();
    statusArea.clearGroup(StatusGroup::InputMethod);
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

std::string KkcEngine::subMode(const InputMethodEntry &, InputContext &ic) {
    if (auto status = inputModeStatus(this, &ic)) {
        return _(status->description);
    }
    return "";
}

KkcState *KkcEngine::state(InputContext *ic) {
    return ic->propertyFor(&factory_);
}
} // namespace fcitx

FCITX_ADDON_FACTORY(fcitx::KkcFactory);
