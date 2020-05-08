/*
 * SPDX-FileCopyrightText: 2013~2017 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 */
#include "addshortcutdialog.h"
#include <fcitx-utils/i18n.h>

namespace fcitx {

AddShortcutDialog::AddShortcutDialog(QWidget *parent)
    : QDialog(parent), length_(0) {
    setupUi(this);
    keyButton_->setModifierlessAllowed(true);
    keyButton_->setMultiKeyShortcutsAllowed(false);
    for (int i = 0; i <= KKC_INPUT_MODE_DIRECT; i++) {
        inputModeComboBox_->addItem(_(modeName[i]));
    }

    commands_ = kkc_keymap_commands(&length_);

    for (int i = 0; i < length_; i++) {
        gchar *label = kkc_keymap_get_command_label(commands_[i]);
        commandComboBox_->addItem(QString::fromUtf8(label));
        g_free(label);
    }

    connect(keyButton_, &FcitxQtKeySequenceWidget::keySequenceChanged, this,
            &AddShortcutDialog::keyChanged);

    keyChanged();
}

AddShortcutDialog::~AddShortcutDialog() {
    for (int i = 0; i < length_; i++) {
        g_free(commands_[i]);
    }
    g_free(commands_);
}

void AddShortcutDialog::keyChanged() {
    buttonBox_->button(QDialogButtonBox::Ok)
        ->setEnabled(keyButton_->keySequence().count() > 0);
}

ShortcutEntry AddShortcutDialog::shortcut() {
    KkcInputMode mode =
        static_cast<KkcInputMode>(inputModeComboBox_->currentIndex());
    const QString command =
        QString::fromUtf8(commands_[commandComboBox_->currentIndex()]);
    auto key = keyButton_->keySequence()[0];

    auto event = makeGObjectUnique(kkc_key_event_new_from_x_event(
        key.sym(), 0,
        static_cast<KkcModifierType>(static_cast<uint32_t>(key.states()))));
    return ShortcutEntry(command, event.get(), commandComboBox_->currentText(),
                         mode);
}

} // namespace fcitx
