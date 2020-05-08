/*
 * SPDX-FileCopyrightText: 2013~2017 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 */

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
