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

#include "dictwidget.h"
#include "adddictdialog.h"
#include "dictmodel.h"
#include "shortcutmodel.h"
#include <fcitx-utils/i18n.h>

namespace fcitx {

KkcDictWidget::KkcDictWidget(QWidget *parent)
    : FcitxQtConfigUIWidget(parent), dictModel_(new DictModel(this)) {
    setupUi(this);

    dictionaryView_->setModel(dictModel_);

    connect(addDictButton_, &QPushButton::clicked, this,
            &KkcDictWidget::addDictClicked);
    connect(defaultDictButton_, &QPushButton::clicked, this,
            &KkcDictWidget::defaultDictClicked);
    connect(removeDictButton_, &QPushButton::clicked, this,
            &KkcDictWidget::removeDictClicked);
    connect(moveUpDictButton_, &QPushButton::clicked, this,
            &KkcDictWidget::moveUpDictClicked);
    connect(moveDownDictButton_, &QPushButton::clicked, this,
            &KkcDictWidget::moveDownClicked);

    load();
}

QString KkcDictWidget::title() { return _("Dictionary Manager"); }

QString KkcDictWidget::icon() { return "fcitx-kkc"; }

void KkcDictWidget::load() {
    dictModel_->load();
    Q_EMIT changed(false);
}

void KkcDictWidget::save() {
    dictModel_->save();
    Q_EMIT changed(false);
}

void KkcDictWidget::addDictClicked() {
    AddDictDialog dialog;
    int result = dialog.exec();
    if (result == QDialog::Accepted) {
        dictModel_->add(dialog.dictionary());
        Q_EMIT changed(true);
    }
}

void KkcDictWidget::defaultDictClicked() {
    dictModel_->defaults();
    Q_EMIT changed(true);
}

void KkcDictWidget::removeDictClicked() {
    if (dictionaryView_->currentIndex().isValid()) {
        dictModel_->removeRow(dictionaryView_->currentIndex().row());
        Q_EMIT changed(true);
    }
}

void KkcDictWidget::moveUpDictClicked() {
    int row = dictionaryView_->currentIndex().row();
    if (dictModel_->moveUp(dictionaryView_->currentIndex())) {
        dictionaryView_->selectionModel()->setCurrentIndex(
            dictModel_->index(row - 1), QItemSelectionModel::ClearAndSelect);
        Q_EMIT changed(true);
    }
}

void KkcDictWidget::moveDownClicked() {
    int row = dictionaryView_->currentIndex().row();
    if (dictModel_->moveDown(dictionaryView_->currentIndex())) {
        dictionaryView_->selectionModel()->setCurrentIndex(
            dictModel_->index(row + 1), QItemSelectionModel::ClearAndSelect);
        Q_EMIT changed(true);
    }
}

} // namespace fcitx
