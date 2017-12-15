/*
 * Copyright (C) 2017~2017 by CSSlayer
 * wengxt@gmail.com
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2.1 of the
 * License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; see the file COPYING. If not,
 * see <http://www.gnu.org/licenses/>.
 */
#ifndef _FCITX_KKC_H_
#define _FCITX_KKC_H_

#include <fcitx-config/configuration.h>
#include <fcitx/addonfactory.h>
#include <fcitx/addonmanager.h>
#include <fcitx/inputcontextproperty.h>
#include <fcitx/inputmethodengine.h>
#include <fcitx/instance.h>
#include <libkkc/libkkc.h>
#include <memory>

namespace fcitx {

class KKCState;

class KKC final : public InputMethodEngine {
public:
    KKC(Instance *instance);
    ~KKC();
    Instance *instance() { return instance_; }
    void activate(const InputMethodEntry &entry,
                  InputContextEvent &event) override;
    void keyEvent(const InputMethodEntry &entry, KeyEvent &keyEvent) override;
    void reloadConfig() override;
    void reset(const InputMethodEntry &entry,
               InputContextEvent &event) override;
    void save() override;
    auto &factory() { return factory_; }
    auto model() { return model_.get(); }

    void updateUI(InputContext *inputContext);

    std::string subMode(const InputMethodEntry &, InputContext &) override;

    KKCState *state(InputContext *ic);

private:
    void loadDictionary();
    void loadRule();

    Instance *instance_;
    FactoryFor<KKCState> factory_;
    std::unique_ptr<KkcLanguageModel, decltype(&g_object_unref)> model_;
};

class KKCFactory : public AddonFactory {
public:
    AddonInstance *create(AddonManager *manager) override {
        return new KKC(manager->instance());
    }
};
} // namespace fcitx

#endif // _FCITX_KKC_H_
