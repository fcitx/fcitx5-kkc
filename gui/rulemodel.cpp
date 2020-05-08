/*
 * SPDX-FileCopyrightText: 2013~2017 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 */

#include "rulemodel.h"
#include <libkkc/libkkc.h>
namespace fcitx {
RuleModel::RuleModel(QObject *parent) : QAbstractListModel(parent) {}

RuleModel::~RuleModel() {}

void RuleModel::load() {
    beginResetModel();
    int length;
    KkcRuleMetadata **rules = kkc_rule_list(&length);
    for (int i = 0; i < length; i++) {
        int priority;
        g_object_get(G_OBJECT(rules[i]), "priority", &priority, nullptr);
        if (priority < 70) {
            continue;
        }
        gchar *name, *label;
        g_object_get(G_OBJECT(rules[i]), "label", &label, "name", &name,
                     nullptr);
        rules_ << Rule(QString::fromUtf8(name), QString::fromUtf8(label));
        g_object_unref(rules[i]);
        g_free(name);
        g_free(label);
    }
    g_free(rules);
    endResetModel();
}

int RuleModel::rowCount(const QModelIndex &parent) const {
    return parent.isValid() ? 0 : rules_.size();
}

QVariant RuleModel::data(const QModelIndex &index, int role) const {
    if (!index.isValid()) {
        return QVariant();
    }

    if (index.row() >= rules_.size() || index.column() != 0) {
        return QVariant();
    }

    switch (role) {
    case Qt::DisplayRole:
        return rules_[index.row()].label();
    case Qt::UserRole:
        return rules_[index.row()].name();
    }
    return QVariant();
}

int RuleModel::findRule(const QString &name) {
    int i = 0;
    Q_FOREACH (const Rule &rule, rules_) {
        if (rule.name() == name) {
            return i;
        }
        i++;
    }
    return -1;
}

} // namespace fcitx
