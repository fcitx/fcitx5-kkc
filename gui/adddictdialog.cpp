/*
 * SPDX-FileCopyrightText: 2013~2017 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 */

#include "adddictdialog.h"
#include "config.h"
#include <QFileDialog>
#include <array>
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
