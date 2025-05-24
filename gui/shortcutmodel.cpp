/*
 * SPDX-FileCopyrightText: 2013~2017 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 */

#include "shortcutmodel.h"
#include "common.h"
#include <QAbstractTableModel>
#include <QObject>
#include <QVariant>
#include <Qt>
#include <fcitx-utils/i18n.h>
#include <fcitx-utils/standardpaths.h>
#include <fcitx-utils/stringutils.h>
#include <glib.h>
#include <libkkc/libkkc.h>
#include <utility>

namespace fcitx {

ShortcutModel::ShortcutModel(QObject *parent)
    : QAbstractTableModel(parent), needSave_(false) {}

ShortcutModel::~ShortcutModel() {}

int ShortcutModel::rowCount(const QModelIndex &parent) const {
    return parent.isValid() ? 0 : entries_.length();
}

int ShortcutModel::columnCount(const QModelIndex &parent) const {
    return parent.isValid() ? 0 : 3;
}

const char *modeName[] = {
    N_("Hiragana"), N_("Katakana"),   N_("Half width Katakana"),
    N_("Latin"),    N_("Wide latin"), N_("Direct input"),
};

QVariant ShortcutModel::data(const QModelIndex &index, int role) const {
    if (!index.isValid()) {
        return {};
    }

    if (index.row() >= entries_.size() || index.column() >= 3) {
        return {};
    }

    switch (role) {
    case Qt::DisplayRole:
        switch (index.column()) {
        case 0:
            return _(modeName[entries_[index.row()].mode()]);
        case 1:
            return entries_[index.row()].keyString();
        case 2:
            return entries_[index.row()].label();
        default:
            break;
        }
        break;
    default:
        break;
    }
    return {};
}

QVariant ShortcutModel::headerData(int section, Qt::Orientation orientation,
                                   int role) const {
    if (orientation == Qt::Vertical) {
        return QAbstractItemModel::headerData(section, orientation, role);
    }

    switch (role) {
    case Qt::DisplayRole:
        switch (section) {
        case 0:
            return _("Input Mode");
        case 1:
            return _("Key");
        case 2:
            return _("Function");
        default:
            break;
        }
        break;
    default:
        break;
    }
    return QAbstractItemModel::headerData(section, orientation, role);
}

void ShortcutModel::load(const QString &name) {
    setNeedSave(false);
    beginResetModel();

    do {
        userRule_.reset();
        entries_.clear();

        KkcRuleMetadata *ruleMeta =
            kkc_rule_metadata_find(name.toUtf8().constData());
        if (!ruleMeta) {
            return;
        }

        const auto basePath =
            StandardPaths::global().userDirectory(StandardPathsType::PkgData) /
            "kkc/rules";

        auto userRule = makeGObjectUnique(kkc_user_rule_new(
            ruleMeta, basePath.string().c_str(), "fcitx-kkc", nullptr));
        if (!userRule) {
            break;
        }

        for (int mode = 0; mode <= KKC_INPUT_MODE_DIRECT; mode++) {
            auto keymap = makeGObjectUnique(kkc_rule_get_keymap(
                KKC_RULE(userRule.get()), (KkcInputMode)mode));
            int length;
            KkcKeymapEntry *entries = kkc_keymap_entries(keymap.get(), &length);

            for (int i = 0; i < length; i++) {
                if (entries[i].command) {
                    gchar *label =
                        kkc_keymap_get_command_label(entries[i].command);
                    entries_ << ShortcutEntry(
                        QString::fromUtf8(entries[i].command), entries[i].key,
                        QString::fromUtf8(label),
                        static_cast<KkcInputMode>(mode));
                    g_free(label);
                }
            }

            for (int i = 0; i < length; i++) {
                kkc_keymap_entry_destroy(&entries[i]);
            }
            g_free(entries);
        }

        userRule_ = std::move(userRule);
    } while (0);

    endResetModel();
}

void ShortcutModel::save() {
    if (userRule_ && needSave_) {
        for (int mode = 0; mode <= KKC_INPUT_MODE_DIRECT; mode++) {
            kkc_user_rule_write(userRule_.get(), (KkcInputMode)mode, nullptr);
        }
    }

    setNeedSave(false);
}

bool ShortcutModel::add(const ShortcutEntry &entry) {
    auto map = makeGObjectUnique(
        kkc_rule_get_keymap(KKC_RULE(userRule_.get()), entry.mode()));
    bool result = true;
    if (kkc_keymap_lookup_key(map.get(), entry.event())) {
        result = false;
    }

    if (result) {
        beginInsertRows(QModelIndex(), entries_.size(), entries_.size());
        entries_ << entry;
        kkc_keymap_set(map.get(), entry.event(),
                       entry.command().toUtf8().constData());
        endInsertRows();
        setNeedSave(true);
    }

    return result;
}

void ShortcutModel::remove(const QModelIndex &index) {
    if (!userRule_) {
        return;
    }

    if (!index.isValid()) {
        return;
    }

    if (index.row() >= entries_.size()) {
        return;
    }

    beginRemoveRows(QModelIndex(), index.row(), index.row());
    auto map = makeGObjectUnique(kkc_rule_get_keymap(
        KKC_RULE(userRule_.get()), entries_[index.row()].mode()));
    kkc_keymap_set(map.get(), entries_[index.row()].event(), nullptr);

    entries_.removeAt(index.row());
    endRemoveRows();

    setNeedSave(true);
}

void ShortcutModel::setNeedSave(bool needSave) {
    if (needSave_ != needSave) {
        needSave_ = needSave;
        Q_EMIT needSaveChanged(needSave_);
    }
}

bool ShortcutModel::needSave() const { return needSave_; }

} // namespace fcitx
