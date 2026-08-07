// SPDX-FileCopyrightText: 2009 Canonical
// SPDX-FileCopyrightText: 2026 ChathurangaBW
// SPDX-License-Identifier: LGPL-2.0-or-later
#include "dbusmenutypes.h"

#include <QDBusArgument>
#include <QDBusMetaType>
#include <QDBusVariant>
#include <QKeySequence>

namespace
{
void translateShortcutTokens(QStringList &tokens)
{
    tokens.replaceInStrings(QStringLiteral("Super"), QStringLiteral("Meta"));
    tokens.replaceInStrings(QStringLiteral("Control"), QStringLiteral("Ctrl"));
    tokens.replaceInStrings(QStringLiteral("plus"), QStringLiteral("+"));
    tokens.replaceInStrings(QStringLiteral("minus"), QStringLiteral("-"));
}
}

QDBusArgument &operator<<(QDBusArgument &argument, const DBusMenuItem &item)
{
    argument.beginStructure();
    argument << item.id << item.properties;
    argument.endStructure();
    return argument;
}

const QDBusArgument &operator>>(const QDBusArgument &argument, DBusMenuItem &item)
{
    argument.beginStructure();
    argument >> item.id >> item.properties;
    argument.endStructure();
    return argument;
}

QDBusArgument &operator<<(QDBusArgument &argument, const DBusMenuItemKeys &item)
{
    argument.beginStructure();
    argument << item.id << item.properties;
    argument.endStructure();
    return argument;
}

const QDBusArgument &operator>>(const QDBusArgument &argument, DBusMenuItemKeys &item)
{
    argument.beginStructure();
    argument >> item.id >> item.properties;
    argument.endStructure();
    return argument;
}

QDBusArgument &operator<<(QDBusArgument &argument, const DBusMenuLayoutItem &item)
{
    argument.beginStructure();
    argument << item.id << item.properties;
    argument.beginArray(qMetaTypeId<QDBusVariant>());
    for (const DBusMenuLayoutItem &child : item.children) {
        argument << QDBusVariant(QVariant::fromValue(child));
    }
    argument.endArray();
    argument.endStructure();
    return argument;
}

const QDBusArgument &operator>>(const QDBusArgument &argument, DBusMenuLayoutItem &item)
{
    argument.beginStructure();
    argument >> item.id >> item.properties;
    item.children.clear();
    argument.beginArray();
    while (!argument.atEnd()) {
        QDBusVariant wrappedChild;
        argument >> wrappedChild;
        const QDBusArgument childArgument = wrappedChild.variant().value<QDBusArgument>();
        DBusMenuLayoutItem child;
        childArgument >> child;
        item.children.append(child);
    }
    argument.endArray();
    argument.endStructure();
    return argument;
}

QDBusArgument &operator<<(QDBusArgument &argument, const DBusMenuShortcut &shortcut)
{
    argument.beginArray(qMetaTypeId<QStringList>());
    for (const QStringList &keyTokens : shortcut) {
        argument << keyTokens;
    }
    argument.endArray();
    return argument;
}

const QDBusArgument &operator>>(const QDBusArgument &argument, DBusMenuShortcut &shortcut)
{
    shortcut.clear();
    argument.beginArray();
    while (!argument.atEnd()) {
        QStringList keyTokens;
        argument >> keyTokens;
        shortcut.append(keyTokens);
    }
    argument.endArray();
    return argument;
}

QKeySequence DBusMenuShortcut::toKeySequence() const
{
    QStringList sequences;
    for (const QStringList &exportedTokens : *this) {
        QStringList tokens = exportedTokens;
        translateShortcutTokens(tokens);
        sequences.append(tokens.join(QLatin1Char('+')));
    }
    return QKeySequence::fromString(sequences.join(QStringLiteral(", ")));
}

void registerDBusMenuTypes()
{
    static bool registered = false;
    if (registered) {
        return;
    }

    qDBusRegisterMetaType<DBusMenuItem>();
    qDBusRegisterMetaType<DBusMenuItemList>();
    qDBusRegisterMetaType<DBusMenuItemKeys>();
    qDBusRegisterMetaType<DBusMenuItemKeysList>();
    qDBusRegisterMetaType<DBusMenuLayoutItem>();
    qDBusRegisterMetaType<QList<DBusMenuLayoutItem>>();
    qDBusRegisterMetaType<DBusMenuShortcut>();
    registered = true;
}
