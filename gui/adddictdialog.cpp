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

#include "adddictdialog.h"
#include "config.h"
#include "ui_adddictdialog.h"
#include <QDebug>
#include <QFileDialog>
#include <fcitx-utils/i18n.h>
#include <fcitx-utils/standardpath.h>
#include <fcitx-utils/stringutils.h>

#define FCITX_CONFIG_DIR_STR "$FCITX_CONFIG_DIR"

namespace fcitx {

AddDictDialog::AddDictDialog(QWidget *parent)
    : QDialog(parent), m_ui(new Ui::AddDictDialog) {
    m_ui->setupUi(this);
    m_ui->typeLabel->setText(_("&Type:"));
    m_ui->pathLabel->setText(_("&Path:"));
    m_ui->typeComboBox->addItem(_("System"));
    m_ui->typeComboBox->addItem(_("User"));

    connect(m_ui->browseButton, SIGNAL(clicked(bool)), this,
            SLOT(browseClicked()));
}

AddDictDialog::~AddDictDialog() { delete m_ui; }

QMap<QString, QString> AddDictDialog::dictionary() {
    int idx = m_ui->typeComboBox->currentIndex();
    idx = idx < 0 ? 0 : idx;
    idx = idx > 2 ? 0 : idx;

    const char *type[] = {"readonly", "readwrite"};

    QMap<QString, QString> dict;
    dict["type"] = "file";
    dict["file"] = m_ui->urlLineEdit->text();
    dict["mode"] = type[idx];

    return dict;
}

void AddDictDialog::browseClicked() {
    QString path = m_ui->urlLineEdit->text();
    if (m_ui->typeComboBox->currentIndex() == 0) {
        QString dir;
        if (path.isEmpty()) {
            path = SKK_DEFAULT_PATH;
        }
        QFileInfo info(path);
        path = QFileDialog::getOpenFileName(this, _("Select Dictionary File"),
                                            info.path());
    } else {
        constexpr char configDir[] = FCITX_CONFIG_DIR_STR "/";
        auto fcitxBasePath = stringutils::joinPath(
            StandardPath::global().userDirectory(StandardPath::Type::PkgData));
        QString basePath =
            QDir::cleanPath(QString::fromLocal8Bit(fcitxBasePath.data()));
        if (path.isEmpty()) {
            path = basePath;
        } else if (path.startsWith(configDir)) {
            QDir dir(basePath);
            path = dir.filePath(path.mid(sizeof(configDir) - 1));
        }
        qDebug() << path;
        path = QFileDialog::getExistingDirectory(
            this, _("Select Dictionary Directory"), path);
        if (path.startsWith(basePath + "/")) {
            path = FCITX_CONFIG_DIR_STR + path.mid(basePath.length(), -1);
        }
    }

    if (!path.isEmpty()) {
        m_ui->urlLineEdit->setText(path);
    }
}

} // namespace fcitx
