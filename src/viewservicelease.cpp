// SPDX-FileCopyrightText: 2026 ChathurangaBW
// SPDX-License-Identifier: GPL-2.0-or-later
#include "viewservicelease.h"

#include <Plasma/Applet>

#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDBusServiceWatcher>
#include <QObject>

namespace
{
int activeLeaseCount = 0;

QString viewService()
{
    return QStringLiteral("org.kde.kappmenuview");
}

void registerViewService()
{
    if (QDBusConnectionInterface *interface = QDBusConnection::sessionBus().interface()) {
        interface->registerService(
            viewService(),
            QDBusConnectionInterface::QueueService,
            QDBusConnectionInterface::DontAllowReplacement);
    }
}

bool hasActiveStockApplet(Plasma::Applet *applet)
{
    QObject *root = applet;
    while (root && root->parent()) {
        root = root->parent();
    }
    if (!root) {
        return false;
    }

    const auto applets = root->findChildren<Plasma::Applet *>();
    for (Plasma::Applet *candidate : applets) {
        if (candidate == applet) {
            continue;
        }
        if (candidate->pluginMetaData().pluginId() != QLatin1String("org.kde.plasma.appmenu")) {
            continue;
        }
        if (!candidate->property("destroyed").toBool()) {
            return true;
        }
    }
    return false;
}

void setWatcherSignalsBlocked(Plasma::Applet *applet, bool blocked)
{
    const auto watchers = applet->findChildren<QDBusServiceWatcher *>();
    for (QDBusServiceWatcher *watcher : watchers) {
        watcher->blockSignals(blocked);
    }
}
}

ViewServiceLease::ViewServiceLease(Plasma::Applet *applet)
    : m_applet(applet)
{
    setActive(true);
    m_destroyedConnection = QObject::connect(
        applet,
        &Plasma::Applet::destroyedChanged,
        applet,
        [this](bool destroyed) {
            setActive(!destroyed);
        });
}

ViewServiceLease::~ViewServiceLease()
{
    QObject::disconnect(m_destroyedConnection);
    setActive(false);
}

void ViewServiceLease::setActive(bool active)
{
    if (m_active == active || !m_applet) {
        return;
    }

    m_active = active;
    if (active) {
        setWatcherSignalsBlocked(m_applet, false);
        if (++activeLeaseCount == 1) {
            registerViewService();
        }
        return;
    }

    setWatcherSignalsBlocked(m_applet, true);
    if (activeLeaseCount > 0 && --activeLeaseCount == 0 && !hasActiveStockApplet(m_applet)) {
        if (QDBusConnectionInterface *interface = QDBusConnection::sessionBus().interface()) {
            interface->unregisterService(viewService());
        }
    }
}
