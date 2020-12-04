/*
 * SPDX-FileCopyrightText: 2013~2017 CSSlayer <wengxt@gmail.com>
 * SPDX-FileCopyrightText: 2017~2017 luren <byljcron@gmail.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 */
#ifndef _FCITX_KKC_H_
#define _FCITX_KKC_H_

#include "config.h"

#include <fcitx-config/configuration.h>
#include <fcitx-config/enum.h>
#include <fcitx-config/iniparser.h>
#include <fcitx-utils/i18n.h>
#include <fcitx/action.h>
#include <fcitx/addonfactory.h>
#include <fcitx/addonmanager.h>
#include <fcitx/candidatelist.h>
#include <fcitx/inputcontextproperty.h>
#include <fcitx/inputmethodengine.h>
#include <fcitx/instance.h>
#include <libkkc/libkkc.h>
#include <memory>
namespace fcitx {

FCITX_CONFIG_ENUM_NAME_WITH_I18N(CandidateLayoutHint, N_("Not set"),
                                 N_("Vertical"), N_("Horizontal"));

FCITX_CONFIG_ENUM_NAME_WITH_I18N(KkcPunctuationStyle, N_("Japanese"),
                                 N_("Latin"), N_("Wide latin"),
                                 N_("Wide latin Japanese"));

FCITX_CONFIG_ENUM_NAME_WITH_I18N(KkcInputMode, N_("Hiragana"), N_("Katakana"),
                                 N_("Half width Katakana"), N_("Latin"),
                                 N_("Wide latin"), N_("Direct input"));

struct NotEmpty {
    bool check(const std::string &value) const { return !value.empty(); }
    void dumpDescription(RawConfig &) const {}
};

struct RuleAnnotation : public EnumAnnotation {
    void dumpDescription(RawConfig &config) const {
        EnumAnnotation::dumpDescription(config);
        int length;
        auto rules = kkc_rule_list(&length);
        FCITX_INFO() << length;
        int total = 0;
        for (int i = 0; i < length; i++) {
            int priority;
            g_object_get(G_OBJECT(rules[i]), "priority", &priority, nullptr);
            if (priority < 70) {
                continue;
            }
            gchar *name, *label;
            g_object_get(G_OBJECT(rules[i]), "label", &label, "name", &name,
                         nullptr);
            config.setValueByPath("Enum/" + std::to_string(total), name);
            config.setValueByPath("EnumI18n/" + std::to_string(total), label);
            g_object_unref(rules[i]);
            g_free(name);
            g_free(label);
            total += 1;
        }
        g_free(rules);
    }
};

FCITX_CONFIGURATION(
    KkcConfig, Option<std::string, NotEmpty, DefaultMarshaller<std::string>,
                      RuleAnnotation>
                   rule{this, "Rule", _("Rule"), "default"};
    OptionWithAnnotation<KkcPunctuationStyle, KkcPunctuationStyleI18NAnnotation>
        punctuationStyle{this, "PunctuationStyle", _("Punctuation Style"),
                         KKC_PUNCTUATION_STYLE_JA_JA};
    OptionWithAnnotation<KkcInputMode, KkcInputModeI18NAnnotation> inputMode{
        this, "InitialInputMode", _("Initial Input Mode"),
        KKC_INPUT_MODE_HIRAGANA};
    Option<int, IntConstrain> pageSize{this, "PageSize", _("Page size"), 10,
                                       IntConstrain(1, 10)};
    OptionWithAnnotation<CandidateLayoutHint, CandidateLayoutHintI18NAnnotation>
        candidateLayout{this, "Candidate Layout", _("Candidate Layout"),
                        CandidateLayoutHint::Vertical};
    Option<bool> autoCorrect{this, "AutoCorrect", _("Auto Correct"), true};
    KeyListOption prevPageKey{
        this,
        "CandidatesPageUpKey",
        _("Candidates Page Up"),
        {Key(FcitxKey_Page_Up)},
        KeyListConstrain({KeyConstrainFlag::AllowModifierLess})};
    KeyListOption nextPageKey{
        this,
        "CandidatesPageDownKey",
        _("Candidates Page Down"),
        {Key(FcitxKey_Page_Down)},
        KeyListConstrain({KeyConstrainFlag::AllowModifierLess})};
    KeyListOption cursorUpKey{
        this,
        "CursorUp",
        _("Cursor Up"),
        {Key(FcitxKey_Up)},
        KeyListConstrain({KeyConstrainFlag::AllowModifierLess})};
    KeyListOption cursorDownKey{
        this,
        "CursorDown",
        _("Cursor Down"),
        {Key(FcitxKey_Down)},
        KeyListConstrain({KeyConstrainFlag::AllowModifierLess})};
    Option<bool> showAnnotation{this, "ShowAnnotation", _("Show Annotation"),
                                true};
    Option<int, IntConstrain> nTriggersToShowCandWin{
        this, "NTriggersToShowCandWin",
        _("Number candidate of Triggers To Show Candidate Window"), 0,
        IntConstrain(0, 7)};
    ExternalOption ruleEditor{this, "Rule Editor", _("Rule Editor"),
                              "fcitx://config/addon/kkc/rule"};
    ExternalOption dictionary{this, "Dict", _("Dictionary"),
                              "fcitx://config/addon/kkc/dictionary_list"};);

class KkcState;

template <typename T>
using GObjectUniquePtr = UniqueCPtr<T, g_object_unref>;

class KkcEngine final : public InputMethodEngine {
public:
    KkcEngine(Instance *instance);
    ~KkcEngine();
    Instance *instance() { return instance_; }

    const Configuration *getConfig() const override { return &config_; }
    void setConfig(const RawConfig &config) override {
        config_.load(config, true);
        safeSaveAsIni(config_, "conf/kkc.conf");
        reloadConfig();
    }

    void setSubConfig(const std::string &path,
                      const fcitx::RawConfig &) override {
        if (path == "dictionary_list") {
            reloadConfig();
        }
    }

    void activate(const InputMethodEntry &entry,
                  InputContextEvent &event) override;
    void keyEvent(const InputMethodEntry &entry, KeyEvent &keyEvent) override;
    void reloadConfig() override;
    void reset(const InputMethodEntry &entry,
               InputContextEvent &event) override;
    void deactivate(const fcitx::InputMethodEntry &,
                    fcitx::InputContextEvent &event) override;
    void save() override;
    auto &factory() { return factory_; }
    auto dictionaries() { return dictionaries_.get(); }
    auto dummyEmptyDictionaries() { return dummyEmptyDictionaries_.get(); }
    auto &config() { return config_; }
    auto model() { return model_.get(); }
    auto rule() { return userRule_.get(); }

    void updateUI(InputContext *inputContext);

    std::string subMode(const InputMethodEntry &, InputContext &) override;

    KkcState *state(InputContext *ic);

    void updateInputMode(InputContext *ic);

private:
    void loadDictionary();
    void loadRule();

    KkcConfig config_;
    Instance *instance_;
    FactoryFor<KkcState> factory_;
    GObjectUniquePtr<KkcLanguageModel> model_;
    GObjectUniquePtr<KkcDictionaryList> dictionaries_;
    GObjectUniquePtr<KkcDictionaryList> dummyEmptyDictionaries_;
    GObjectUniquePtr<KkcUserRule> userRule_;

    std::unique_ptr<Action> modeAction_;
    std::unique_ptr<Menu> menu_;
    std::vector<std::unique_ptr<Action>> subModeActions_;
};

class KkcFactory : public AddonFactory {
public:
    AddonInstance *create(AddonManager *manager) override {
        registerDomain("fcitx5-kkc", FCITX_INSTALL_LOCALEDIR);
        return new KkcEngine(manager->instance());
    }
};
} // namespace fcitx

#endif // _FCITX_KKC_H_
