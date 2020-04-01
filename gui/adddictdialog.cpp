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
#include <QDebug>
#include <QFileDialog>
#include <fcitx-utils/i18n.h>
#include <fcitx-utils/standardpath.h>
#include <fcitx-utils/stringutils.h>

#define FCITX_CONFIG_DIR_STR "$FCITX_CONFIG_DIR"

namespace fcitx {

constexpr std::array<const char *, 2> type = {"readonly", "readwrite"};

AddDictDialog::AddDictDialog(QWidget *parent) : QDialog(parent) {
    setupUi(this);
    typeComboBox_->addItem(_("System"));
    typeComboBox_->addItem(_("User"));

    connect(browseButton_, &QPushButton::clicked, this,
            &AddDictDialog::browseClicked);
}

QMap<QString, QString> AddDictDialog::dictionary() {
    int idx = typeComboBox_->currentIndex();
    idx = idx < 0 ? 0 : idx;
    idx = idx >= static_cast<int>(type.size()) ? 0 : idx;
    QMap<QString, QString> dict;
    dict["type"] = "file";
    dict["file"] = urlLineEdit_->text();
    dict["mode"] = type[idx];

    return dict;
}

void AddDictDialog::browseClicked() {
    QString path = urlLineEdit_->text();
    if (typeComboBox_->currentIndex() == 0) {
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
        urlLineEdit_->setText(path);
    }
}

} // namespace fcitx
