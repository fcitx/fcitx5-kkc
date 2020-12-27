/*
 * SPDX-FileCopyrightText: 2013~2017 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 */

#include "dictmodel.h"
#include <QFile>
#include <QSet>
#include <QStringList>
#include <QtGlobal>
#include <fcitx-utils/standardpath.h>
#include <fcntl.h>
namespace fcitx {

DictModel::DictModel(QObject *parent) : QAbstractListModel(parent) {
    requiredKeys_ << "file"
                  << "type"
                  << "mode";
}

DictModel::~DictModel() {}

void DictModel::defaults() {
    auto path =
        StandardPath::global().fcitxPath("pkgdatadir", "kkc/dictionary_list");
    QFile f(path.data());
    if (f.open(QIODevice::ReadOnly)) {
        load(f);
    }
}

void DictModel::load() {
    auto file = StandardPath::global().open(StandardPath::Type::PkgData,
                                            "kkc/dictionary_list", O_RDONLY);
    if (file.fd() < 0) {
        return;
    }
    QFile f;
    if (!f.open(file.fd(), QIODevice::ReadOnly)) {
        return;
    }

    load(f);
    f.close();
}

void DictModel::load(QFile &file) {
    beginResetModel();
    dicts_.clear();

    QByteArray bytes;
    while (!(bytes = file.readLine()).isEmpty()) {
        QString line = QString::fromUtf8(bytes).trimmed();
        QStringList items = line.split(",");
        if (items.size() < requiredKeys_.size()) {
            continue;
        }

        bool failed = false;
        QMap<QString, QString> dict;
        Q_FOREACH (const QString &item, items) {
            if (!item.contains('=')) {
                failed = true;
                break;
            }
            QString key = item.section('=', 0, 0);
            QString value = item.section('=', 1, -1);

            if (!requiredKeys_.contains(key)) {
                continue;
            }

            dict[key] = value;
        }

        if (!failed && requiredKeys_.size() == dict.size()) {
            dicts_ << dict;
        }
    }
    endResetModel();
}

bool DictModel::save() {
    return StandardPath::global().safeSave(
        StandardPath::Type::PkgData, "kkc/dictionary_list", [this](int fd) {
            QFile tempFile;
            if (!tempFile.open(fd, QIODevice::WriteOnly)) {
                return false;
            }

            typedef QMap<QString, QString> DictType;

            Q_FOREACH (const DictType &dict, dicts_) {
                bool first = true;
                Q_FOREACH (const QString &key, dict.keys()) {
                    if (first) {
                        first = false;
                    } else {
                        tempFile.write(",");
                    }
                    tempFile.write(key.toUtf8());
                    tempFile.write("=");
                    tempFile.write(dict[key].toUtf8());
                }
                tempFile.write("\n");
            }
            return true;
        });
}

int DictModel::rowCount(const QModelIndex &parent) const {
    if (parent.isValid()) {
        return 0;
    }
    return dicts_.size();
}

bool DictModel::removeRows(int row, int count, const QModelIndex &parent) {
    if (parent.isValid()) {
        return false;
    }

    if (count == 0 || row >= dicts_.size() || row + count > dicts_.size()) {
        return false;
    }

    beginRemoveRows(parent, row, row + count - 1);
    dicts_.erase(dicts_.begin() + row, dicts_.begin() + row + count);
    endRemoveRows();

    return true;
}

QVariant DictModel::data(const QModelIndex &index, int role) const {
    if (!index.isValid()) {
        return QVariant();
    }

    if (index.row() >= dicts_.size() || index.column() != 0) {
        return QVariant();
    }

    switch (role) {
    case Qt::DisplayRole:
        return dicts_[index.row()]["file"];
    }
    return QVariant();
}

bool DictModel::moveUp(const QModelIndex &currentIndex) {
    if (currentIndex.row() > 0 && currentIndex.row() < dicts_.size()) {
        beginResetModel();
#if (QT_VERSION < QT_VERSION_CHECK(5, 13, 0))
        dicts_.swap(currentIndex.row() - 1, currentIndex.row());
#else
        dicts_.swapItemsAt(currentIndex.row() - 1, currentIndex.row());
#endif
        endResetModel();
        return true;
    }
    return false;
}

bool DictModel::moveDown(const QModelIndex &currentIndex) {
    if (currentIndex.row() >= 0 && currentIndex.row() + 1 < dicts_.size()) {
        beginResetModel();
#if (QT_VERSION < QT_VERSION_CHECK(5, 13, 0))
        dicts_.swap(currentIndex.row() + 1, currentIndex.row());
#else
        dicts_.swapItemsAt(currentIndex.row() + 1, currentIndex.row());
#endif
        endResetModel();
        return true;
    }

    return false;
}

void DictModel::add(const QMap<QString, QString> &dict) {
    beginInsertRows(QModelIndex(), dicts_.size(), dicts_.size());
    dicts_ << dict;
    endInsertRows();
}

} // namespace fcitx
