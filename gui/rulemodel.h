/*
 * SPDX-FileCopyrightText: 2013~2017 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 */
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
