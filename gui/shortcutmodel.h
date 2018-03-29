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
#ifndef _GUI_SHORTCUTMODEL_H_
#define _GUI_SHORTCUTMODEL_H_

#include "common.h"
#include <QAbstractTableModel>
#include <libkkc/libkkc.h>
#include <memory>

namespace fcitx {

class ShortcutEntry {
public:
    ShortcutEntry(const QString &command, KkcKeyEvent *event,
                  const QString &label, KkcInputMode mode)
        : command_(command),
          event_(makeGObjectUnique(KKC_KEY_EVENT(g_object_ref(event)))),
          label_(label), mode_(mode) {
        gchar *keystr = kkc_key_event_to_string(event_.get());
        keyString_ = QString::fromUtf8(keystr);
        g_free(keystr);
    }

    ShortcutEntry(const ShortcutEntry &other)
        : ShortcutEntry(other.command_, other.event_.get(), other.label_,
                        other.mode_) {}

    ShortcutEntry &operator=(const ShortcutEntry &other) {
        label_ = other.label_;
        command_ = other.command_;
        event_.reset(KKC_KEY_EVENT(g_object_ref(other.event())));
        mode_ = other.mode_;
        keyString_ = other.keyString_;
        return *this;
    }

    const QString &label() const { return label_; }

    const QString &command() const { return command_; }

    KkcInputMode mode() const { return mode_; }

    KkcKeyEvent *event() const { return event_.get(); }

    const QString &keyString() const { return keyString_; }

private:
    QString command_;
    GObjectUniquePtr<KkcKeyEvent> event_;
    QString label_;
    KkcInputMode mode_;
    QString keyString_;
};

class ShortcutModel : public QAbstractTableModel {
    Q_OBJECT
public:
    explicit ShortcutModel(QObject *parent = 0);
    virtual ~ShortcutModel();
    virtual int rowCount(const QModelIndex &parent = QModelIndex()) const;
    virtual int columnCount(const QModelIndex &parent = QModelIndex()) const;
    virtual QVariant data(const QModelIndex &index,
                          int role = Qt::DisplayRole) const;
    virtual QVariant headerData(int section, Qt::Orientation orientation,
                                int role = Qt::DisplayRole) const;

    bool add(const ShortcutEntry &entry);
    void remove(const QModelIndex &index);
    void load(const QString &name);
    void save();
    bool needSave();

Q_SIGNALS:
    void needSaveChanged(bool needSave);

private:
    void setNeedSave(bool arg1);

private:
    QList<ShortcutEntry> entries_;
    GObjectUniquePtr<KkcUserRule> userRule_;
    bool needSave_;
};

extern const char *modeName[];
} // namespace fcitx

#endif // _GUI_SHORTCUTMODEL_H_
