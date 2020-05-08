/*
 * SPDX-FileCopyrightText: 2013~2017 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 */
#ifndef _GUI_DICTWIDGET_H_
#define _GUI_DICTWIDGET_H_

#include "ui_dictwidget.h"
#include <fcitxqtconfiguiwidget.h>

namespace fcitx {
class DictModel;

class KkcDictWidget : public FcitxQtConfigUIWidget, public Ui::KkcDictWidget {
    Q_OBJECT
public:
    explicit KkcDictWidget(QWidget *parent = 0);

    void load() override;
    void save() override;
    QString title() override;
    QString icon() override;

private Q_SLOTS:
    void addDictClicked();
    void defaultDictClicked();
    void removeDictClicked();
    void moveUpDictClicked();
    void moveDownClicked();

private:
    DictModel *dictModel_;
};

} // namespace fcitx

#endif // _GUI_DICTWIDGET_H_
