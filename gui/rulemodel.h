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
#ifndef _GUI_RULEMODEL_H_
#define _GUI_RULEMODEL_H_

#include <QAbstractListModel>

#include <libkkc/libkkc.h>

namespace fcitx {

class Rule {
public:
    Rule(const QString &name, const QString &label)
        : name_(name), label_(label) {}

    const QString &name() const { return name_; }

    const QString &label() const { return label_; }

private:
    QString name_;
    QString label_;
};

class RuleModel : public QAbstractListModel {
    Q_OBJECT
public:
    explicit RuleModel(QObject *parent = 0);

    virtual ~RuleModel();

    virtual int rowCount(const QModelIndex &parent = QModelIndex()) const;
    virtual QVariant data(const QModelIndex &index,
                          int role = Qt::DisplayRole) const;
    void load();
    int findRule(const QString &name);

private:
    QList<Rule> rules_;
};

} // namespace fcitx

#endif // _GUI_RULEMODEL_H_
