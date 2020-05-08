/*
 * SPDX-FileCopyrightText: 2013~2017 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 */
#ifndef _GUI_SHORTCUTWIDGET_H_
#define _GUI_SHORTCUTWIDGET_H_

#include "ui_shortcutwidget.h"
#include <fcitxqtconfiguiwidget.h>
#include <libkkc/libkkc.h>

namespace fcitx {

class RuleModel;
class ShortcutModel;

class KkcShortcutWidget : public FcitxQtConfigUIWidget, Ui::KkcShortcutWidget {
    Q_OBJECT
public:
    explicit KkcShortcutWidget(QWidget *parent = 0);

    void load() override;
    void save() override;
    QString title() override;
    QString icon() override;
public Q_SLOTS:
    void ruleChanged(int);
    void addShortcutClicked();
    void removeShortcutClicked();
    void shortcutNeedSaveChanged(bool);
    void currentShortcutChanged();

private:
    RuleModel *ruleModel_;
    ShortcutModel *shortcutModel_;
    QString name_;
};
} // namespace fcitx

#endif // _GUI_SHORTCUTWIDGET_H_
