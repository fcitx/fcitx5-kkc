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
#ifndef _FCITX_KKC_H_
#define _FCITX_KKC_H_

#include "config.h"

#include <fcitx-config/configuration.h>
#include <fcitx/addonfactory.h>
#include <fcitx/addonmanager.h>
#include <fcitx/inputcontextproperty.h>
#include <fcitx/inputmethodengine.h>
#include <fcitx/instance.h>
#include <libkkc/libkkc.h>
#include <memory>

namespace fcitx {

class KkcState;

template <typename T>
using GObjectUniquePtr = std::unique_ptr<T, decltype(&g_object_unref)>;

class KkcEngine final : public InputMethodEngine {
public:
    KkcEngine(Instance *instance);
    ~KkcEngine();
    Instance *instance() { return instance_; }
    void activate(const InputMethodEntry &entry,
                  InputContextEvent &event) override;
    void keyEvent(const InputMethodEntry &entry, KeyEvent &keyEvent) override;
    void reloadConfig() override;
    void reset(const InputMethodEntry &entry,
               InputContextEvent &event) override;
    void save() override;
    auto &factory() { return factory_; }
    auto dictionaries() { return dictionaries_.get(); }
    auto model() { return model_.get(); }
    auto rule() { return userRule_.get(); }

    void updateUI(InputContext *inputContext);

    std::string subMode(const InputMethodEntry &, InputContext &) override;

    KkcState *state(InputContext *ic);

private:
    void loadDictionary();
    void loadRule();

    Instance *instance_;
    FactoryFor<KkcState> factory_;
    GObjectUniquePtr<KkcLanguageModel> model_;
    GObjectUniquePtr<KkcDictionaryList> dictionaries_;
    GObjectUniquePtr<KkcUserRule> userRule_;
};

class KkcFactory : public AddonFactory {
public:
    AddonInstance *create(AddonManager *manager) override {
        return new KkcEngine(manager->instance());
    }
};
} // namespace fcitx

#endif // _FCITX_KKC_H_
