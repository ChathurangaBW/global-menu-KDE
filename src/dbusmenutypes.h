// SPDX-FileCopyrightText: 2009 Canonical
// SPDX-FileCopyrightText: 2026 ChathurangaBW
// SPDX-License-Identifier: LGPL-2.0-or-later
#pragma once

#include <QList>
#include <QMetaType>
#include <QStringList>
#include <QVariantMap>

class QDBusArgument;
class QKeySequence;

struct DBusMenuItem {
    int id = -1;
    QVariantMap properties;
};

struct DBusMenuItemKeys {
    int id = -1;
    QStringList properties;
};

using DBusMenuItemList = QList<DBusMenuItem>;
using DBusMenuItemKeysList = QList<DBusMenuItemKeys>;

struct DBusMenuLayoutItem {
    int id = -1;
    QVariantMap properties;
    QList<DBusMenuLayoutItem> children;
};

class DBusMenuShortcut : public QList<QStringList>
{
public:
    [[nodiscard]] QKeySequence toKeySequence() const;
};

Q_DECLARE_METATYPE(DBusMenuItem)
Q_DECLARE_METATYPE(DBusMenuItemList)
Q_DECLARE_METATYPE(DBusMenuItemKeys)
Q_DECLARE_METATYPE(DBusMenuItemKeysList)
Q_DECLARE_METATYPE(DBusMenuLayoutItem)
Q_DECLARE_METATYPE(QList<DBusMenuLayoutItem>)
Q_DECLARE_METATYPE(DBusMenuShortcut)

QDBusArgument &operator<<(QDBusArgument &argument, const DBusMenuItem &item);
const QDBusArgument &operator>>(const QDBusArgument &argument, DBusMenuItem &item);
QDBusArgument &operator<<(QDBusArgument &argument, const DBusMenuItemKeys &item);
const QDBusArgument &operator>>(const QDBusArgument &argument, DBusMenuItemKeys &item);
QDBusArgument &operator<<(QDBusArgument &argument, const DBusMenuLayoutItem &item);
const QDBusArgument &operator>>(const QDBusArgument &argument, DBusMenuLayoutItem &item);
QDBusArgument &operator<<(QDBusArgument &argument, const DBusMenuShortcut &shortcut);
const QDBusArgument &operator>>(const QDBusArgument &argument, DBusMenuShortcut &shortcut);

void registerDBusMenuTypes();
