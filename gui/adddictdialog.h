/*
 * SPDX-FileCopyrightText: 2013~2017 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 */
#ifndef _GUI_ADDDICTDIALOG_H_
#define _GUI_ADDDICTDIALOG_H_

#include "ui_adddictdialog.h"
#include <QDialog>
#include <QMap>

namespace fcitx {

class AddDictDialog : public QDialog, public Ui::AddDictDialog {
    Q_OBJECT
public:
    explicit AddDictDialog(QWidget *parent = 0);
    QMap<QString, QString> dictionary();

public Q_SLOTS:
    void browseClicked();
};

} // namespace fcitx

#endif // _GUI_ADDDICTDIALOG_H_
