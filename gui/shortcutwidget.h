//
// Copyright (C) 2013~2017 by CSSlayer
// wengxt@gmail.com
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
#ifndef _GUI_SHORTCUTWIDGET_H_
#define _GUI_SHORTCUTWIDGET_H_

#include <fcitxqtconfiguiwidget.h>
#include <libkkc/libkkc.h>

namespace Ui {
class KkcShortcutWidget;
}

namespace fcitx {

class RuleModel;
class ShortcutModel;

class KkcShortcutWidget : public FcitxQtConfigUIWidget {
    Q_OBJECT
public:
    explicit KkcShortcutWidget(QWidget *parent = 0);
    virtual ~KkcShortcutWidget();

    virtual void load();
    virtual void save();
    virtual QString title();
    virtual QString addon();
    virtual QString icon();
public Q_SLOTS:
    void ruleChanged(int);
    void addShortcutClicked();
    void removeShortcutClicked();
    void shortcutNeedSaveChanged(bool);
    void currentShortcutChanged();

private:
    Ui::KkcShortcutWidget *m_ui;
    ShortcutModel *m_shortcutModel;
    RuleModel *m_ruleModel;
    QString m_name;
};
} // namespace fcitx

#endif // _GUI_SHORTCUTWIDGET_H_
