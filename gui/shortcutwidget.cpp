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

#include "shortcutwidget.h"
#include "addshortcutdialog.h"
#include "rulemodel.h"
#include "shortcutmodel.h"
#include <QDebug>
#include <QFile>
#include <QMessageBox>
#include <fcitx-utils/i18n.h>
#include <fcitx-utils/standardpath.h>
#include <fcntl.h>

namespace fcitx {

KkcShortcutWidget::KkcShortcutWidget(QWidget *parent)
    : FcitxQtConfigUIWidget(parent), ruleModel_(new RuleModel(this)),
      shortcutModel_(new ShortcutModel(this)) {
    setupUi(this);
    ruleComboBox_->setModel(ruleModel_);
    shortcutView_->setModel(shortcutModel_);
    shortcutView_->sortByColumn(3);

    connect(ruleComboBox_, qOverload<int>(&QComboBox::currentIndexChanged),
            this, &KkcShortcutWidget::ruleChanged);
    connect(addShortcutButton_, &QPushButton::clicked, this,
            &KkcShortcutWidget::addShortcutClicked);
    connect(removeShortCutButton_, &QPushButton::clicked, this,
            &KkcShortcutWidget::removeShortcutClicked);
    connect(shortcutModel_, &ShortcutModel::needSaveChanged, this,
            &KkcShortcutWidget::shortcutNeedSaveChanged);
    connect(shortcutView_->selectionModel(),
            &QItemSelectionModel::currentChanged, this,
            &KkcShortcutWidget::currentShortcutChanged);

    load();
    currentShortcutChanged();
}
void KkcShortcutWidget::load() {
    auto fd = StandardPath::global().open(StandardPath::Type::PkgConfig,
                                          "kkc/rule", O_RDONLY);
    QString sline;
    do {
        if (fd.fd() >= 0) {
            break;
        }

        QFile f;
        QByteArray line;
        if (f.open(fd.fd(), QIODevice::ReadOnly)) {
            line = f.readLine();
            f.close();
        }

        sline = QString::fromUtf8(line).trimmed();

        if (sline.isEmpty()) {
            sline = "default";
        }
    } while (0);
    ruleModel_->load();
    int idx = ruleModel_->findRule(sline);
    idx = idx < 0 ? 0 : idx;
    ruleComboBox_->setCurrentIndex(idx);

    Q_EMIT changed(false);
}

void KkcShortcutWidget::save() {
    shortcutModel_->save();

    QString name =
        ruleModel_
            ->data(ruleModel_->index(ruleComboBox_->currentIndex(), 0),
                   Qt::UserRole)
            .toString();

    StandardPath::global().safeSave(StandardPath::Type::PkgData, "kkc/rule",
                                    [name](int fd) {
                                        QFile f;
                                        if (f.open(fd, QIODevice::WriteOnly)) {
                                            f.write(name.toUtf8());
                                            f.close();
                                        } else {
                                            return false;
                                        }
                                        return true;
                                    });

    Q_EMIT changed(false);
}

QString KkcShortcutWidget::title() { return _("Shortcut Manager"); }

QString KkcShortcutWidget::icon() { return "fcitx-kkc"; }

void KkcShortcutWidget::ruleChanged(int rule) {
    QString name =
        ruleModel_->data(ruleModel_->index(rule, 0), Qt::UserRole).toString();
    if (shortcutModel_->needSave()) {
        int ret = QMessageBox::question(
            this, _("Save Changes"),
            _("The content has changed.\n"
              "Do you want to save the changes or discard them?"),
            QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
        if (ret == QMessageBox::Save) {
            shortcutModel_->save();
        } else if (ret == QMessageBox::Cancel) {
            int idx = ruleModel_->findRule(name_);
            if (idx < 0) {
                idx = 0;
            }
            ruleComboBox_->setCurrentIndex(idx);
            return;
        }
    }
    shortcutModel_->load(name);
    name_ = name;
    Q_EMIT changed(true);
}

void KkcShortcutWidget::addShortcutClicked() {
    AddShortcutDialog dialog;
    if (dialog.exec() == QDialog::Accepted) {
        if (!shortcutModel_->add(dialog.shortcut())) {
            QMessageBox::critical(
                this, _("Key Conflict"),
                _("Key to add is conflict with existing shortcut."));
        }
    }
}

void KkcShortcutWidget::removeShortcutClicked() {
    QModelIndex idx = shortcutView_->currentIndex();
    if (idx.isValid()) {
        shortcutModel_->remove(idx);
    }
}

void KkcShortcutWidget::shortcutNeedSaveChanged(bool needSave) {
    if (needSave) {
        Q_EMIT changed(true);
    }
}

void KkcShortcutWidget::currentShortcutChanged() {
    removeShortCutButton_->setEnabled(shortcutView_->currentIndex().isValid());
}

} // namespace fcitx
