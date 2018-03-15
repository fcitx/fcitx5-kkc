//
// Copyright (C) 2013~2017 by CSSlayer
// wengxt@gmail.com
//
// This library is free software; you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as
// published by the Free Software Foundation; either version 2.1 of the
// License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; see the file COPYING. If not,
// see <http://www.gnu.org/licenses/>.
//
#include "addshortcutdialog.h"
#include "ui_addshortcutdialog.h"
#include <fcitx-utils/i18n.h>

namespace fcitx {

AddShortcutDialog::AddShortcutDialog(QWidget *parent)
    : QDialog(parent), m_ui(new Ui::AddShortcutDialog), m_length(0) {
    m_ui->setupUi(this);
    m_ui->inputModeLabel->setText(_("&Input Mode"));
    m_ui->commandLabel->setText(_("&Command"));
    m_ui->keyLabel->setText(_("&Key"));
    m_ui->keyButton->setModifierlessAllowed(true);
    m_ui->keyButton->setMultiKeyShortcutsAllowed(false);
    for (int i = 0; i <= KKC_INPUT_MODE_DIRECT; i++) {
        m_ui->inputModeComboBox->addItem(_(modeName[i]));
    }

    m_commands = kkc_keymap_commands(&m_length);

    for (int i = 0; i < m_length; i++) {
        gchar *label = kkc_keymap_get_command_label(m_commands[i]);
        m_ui->commandComboBox->addItem(QString::fromUtf8(label));
        g_free(label);
    }

    connect(m_ui->keyButton, &FcitxQtKeySequenceWidget::keySequenceChanged,
            this, &AddShortcutDialog::keyChanged);

    keyChanged();
}

AddShortcutDialog::~AddShortcutDialog() {
    for (int i = 0; i < m_length; i++) {
        g_free(m_commands[i]);
    }
    g_free(m_commands);
    delete m_ui;
}

void AddShortcutDialog::keyChanged() {
    m_ui->buttonBox->button(QDialogButtonBox::Ok)
        ->setEnabled(m_ui->keyButton->keySequence().count() > 0);
}

ShortcutEntry AddShortcutDialog::shortcut() {
    KkcInputMode mode = (KkcInputMode)m_ui->inputModeComboBox->currentIndex();
    const QString command =
        QString::fromUtf8(m_commands[m_ui->commandComboBox->currentIndex()]);
    auto key = m_ui->keyButton->keySequence()[0];

    auto event = makeGObjectUnique(kkc_key_event_new_from_x_event(
        key.sym(), 0,
        static_cast<KkcModifierType>(static_cast<uint32_t>(key.states()))));
    return ShortcutEntry(command, event.get(),
                         m_ui->commandComboBox->currentText(), mode);
}

} // namespace fcitx
