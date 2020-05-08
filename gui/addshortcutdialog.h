/*
 * SPDX-FileCopyrightText: 2013~2017 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 */
#ifndef _GUI_ADDSHORTCUTDIALOG_H_
#define _GUI_ADDSHORTCUTDIALOG_H_

#include "shortcutmodel.h"
#include "ui_addshortcutdialog.h"
#include <QDialog>
#include <QMap>

namespace fcitx {

class AddShortcutDialog : public QDialog, public Ui::AddShortcutDialog {
    Q_OBJECT
public:
    explicit AddShortcutDialog(QWidget *parent = 0);
    virtual ~AddShortcutDialog();

    bool valid();
    ShortcutEntry shortcut();
public Q_SLOTS:
    void keyChanged();

private:
    int length_;
    gchar **commands_;
};
} // namespace fcitx

#endif // _GUI_ADDSHORTCUTDIALOG_H_
