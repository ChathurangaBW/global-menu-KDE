// SPDX-FileCopyrightText: 2026 ChathurangaBW
// SPDX-License-Identifier: GPL-2.0-or-later
#include "globalmenumodel.h"

#include <QDBusVariant>
#include <QMetaType>
#include <QVariant>

QVariant GlobalMenuModel::property(const DBusMenuLayoutItem &item, const QString &name)
{
    const QVariant value = item.properties.value(name);
    if (value.metaType() == QMetaType::fromType<QDBusVariant>()) {
        return value.value<QDBusVariant>().variant();
    }
    return value;
}
