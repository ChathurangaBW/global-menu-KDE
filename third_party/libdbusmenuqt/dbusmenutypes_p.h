/* This file is part of the dbusmenu-qt library
    SPDX-FileCopyrightText: 2009 Canonical
    SPDX-FileContributor: Aurelien Gateau <aurelien.gateau@canonical.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/
#pragma once

#include <QList>
#include <QStringList>
#include <QVariant>

class QDBusArgument;

struct DBusMenuItem {
    int id;
    QVariantMap properties;
};

QDBusArgument &operator<<(QDBusArgument &argument, const DBusMenuItem &item);
const QDBusArgument &operator>>(const QDBusArgument &argument, DBusMenuItem &item);
typedef QList<DBusMenuItem> DBusMenuItemList;

struct DBusMenuItemKeys {
    int id;
    QStringList properties;
};

QDBusArgument &operator<<(QDBusArgument &argument, const DBusMenuItemKeys &);
const QDBusArgument &operator>>(const QDBusArgument &argument, DBusMenuItemKeys &);
typedef QList<DBusMenuItemKeys> DBusMenuItemKeysList;

struct DBusMenuLayoutItem {
    int id;
    QVariantMap properties;
    QList<DBusMenuLayoutItem> children;
};

QDBusArgument &operator<<(QDBusArgument &argument, const DBusMenuLayoutItem &);
const QDBusArgument &operator>>(const QDBusArgument &argument, DBusMenuLayoutItem &);
typedef QList<DBusMenuLayoutItem> DBusMenuLayoutItemList;

class DBusMenuShortcut;
QDBusArgument &operator<<(QDBusArgument &argument, const DBusMenuShortcut &);
const QDBusArgument &operator>>(const QDBusArgument &argument, DBusMenuShortcut &);

void DBusMenuTypes_register();
