/*
 * SPDX-FileCopyrightText: 2013~2017 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 */
#ifndef _GUI_DICTMODEL_H_
#define _GUI_DICTMODEL_H_

#include <QAbstractItemModel>
#include <QFile>
#include <QSet>

namespace fcitx {

class DictModel : public QAbstractListModel {
    Q_OBJECT
public:
    explicit DictModel(QObject *parent = 0);
    virtual ~DictModel();
    virtual int rowCount(const QModelIndex &parent = QModelIndex()) const;
    virtual QVariant data(const QModelIndex &index,
                          int role = Qt::DisplayRole) const;
    virtual bool removeRows(int row, int count,
                            const QModelIndex &parent = QModelIndex());

    void load();
    void load(QFile &file);
    void defaults();
    bool save();
    void add(const QMap<QString, QString> &dict);
    bool moveDown(const QModelIndex &currentIndex);
    bool moveUp(const QModelIndex &currentIndex);

private:
    QSet<QString> requiredKeys_;
    QList<QMap<QString, QString>> dicts_;
};

} // namespace fcitx

#endif // _GUI_DICTMODEL_H_
