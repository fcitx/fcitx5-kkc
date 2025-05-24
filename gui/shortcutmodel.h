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
#include <QList>
#include <QObject>
#include <QString>
#include <QVariant>
#include <Qt>
#include <glib-object.h>
#include <glib.h>
#include <libkkc/libkkc.h>
#include <utility>

namespace fcitx {

class ShortcutEntry {
public:
    ShortcutEntry(QString command, KkcKeyEvent *event, QString label,
                  KkcInputMode mode)
        : command_(std::move(command)),
          event_(KKC_KEY_EVENT(g_object_ref(event))), label_(std::move(label)),
          mode_(mode) {
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
    bool needSave() const;

Q_SIGNALS:
    void needSaveChanged(bool needSave);

private:
    void setNeedSave(bool needSave);

private:
    QList<ShortcutEntry> entries_;
    GObjectUniquePtr<KkcUserRule> userRule_;
    bool needSave_;
};

extern const char *modeName[];
} // namespace fcitx

#endif // _GUI_SHORTCUTMODEL_H_
