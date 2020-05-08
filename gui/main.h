/*
 * SPDX-FileCopyrightText: 2013~2017 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 */
#ifndef _GUI_MAIN_H_
#define _GUI_MAIN_H_

#include <fcitxqtconfiguiplugin.h>

namespace fcitx {

class KkcConfigPlugin : public FcitxQtConfigUIPlugin {
    Q_OBJECT
public:
    Q_PLUGIN_METADATA(IID FcitxQtConfigUIFactoryInterface_iid FILE
                      "kkc-config.json")
    explicit KkcConfigPlugin(QObject *parent = 0);
    FcitxQtConfigUIWidget *create(const QString &key) override;
};
} // namespace fcitx

#endif // _GUI_MAIN_H_
