/*
 * SPDX-FileCopyrightText: 2013~2017 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 */
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
