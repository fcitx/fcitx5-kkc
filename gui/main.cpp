/*
 * SPDX-FileCopyrightText: 2013~2017 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 */

#include "main.h"
#include "dictwidget.h"
#include "shortcutwidget.h"
#include <QApplication>
#include <fcitx-utils/i18n.h>
#include <libkkc/libkkc.h>
#include <qplugin.h>

namespace fcitx {

KkcConfigPlugin::KkcConfigPlugin(QObject *parent)
    : FcitxQtConfigUIPlugin(parent) {
#if !GLIB_CHECK_VERSION(2, 36, 0)
    g_type_init();
#endif
    kkc_init();
    registerDomain("fcitx5-kkc", FCITX_INSTALL_LOCALEDIR);
}

FcitxQtConfigUIWidget *KkcConfigPlugin::create(const QString &key) {
    if (key == "dictionary_list") {
        return new KkcDictWidget;
    } else if (key == "rule") {
        return new KkcShortcutWidget;
    }
    return nullptr;
}

} // namespace fcitx
