// SPDX-FileCopyrightText: 2016 Kai Uwe Broulik <kde@privat.broulik.de>
// SPDX-FileCopyrightText: 2016 Chinmoy Ranjan Pradhan <chinmoyrp65@gmail.com>
// SPDX-FileCopyrightText: 2026 ChathurangaBW
// SPDX-License-Identifier: GPL-2.0-or-later
#include "globalmenumodel.h"

#include <abstracttasksmodel.h>
#include <tasksmodel.h>

#include <QAction>
#include <QActionGroup>
#include <QDBusArgument>
#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusMessage>
#include <QDBusPendingCallWatcher>
#include <QDBusPendingReply>
#include <QDBusServiceWatcher>
#include <QDBusVariant>
#include <QIcon>
#include <QKeySequence>
#include <QMenu>
#include <QPixmap>
#include <QVariant>

namespace
{
constexpr auto dbusMenuInterface = "com.canonical.dbusmenu";

QVariant unwrapped(const QVariant &value)
{
    if (value.metaType() == QMetaType::fromType<QDBusVariant>()) {
        return value.value<QDBusVariant>().variant();
    }
    return value;
}

QVariant property(const DBusMenuLayoutItem &item, const QString &name)
{
    return unwrapped(item.properties.value(name));
}

bool boolProperty(const DBusMenuLayoutItem &item, const QString &name, bool defaultValue)
{
    if (!item.properties.contains(name)) {
        return defaultValue;
    }
    return property(item, name).toBool();
}

DBusMenuShortcut shortcutProperty(const DBusMenuLayoutItem &item)
{
    const QVariant value = property(item, QStringLiteral("shortcut"));
    if (value.metaType() == QMetaType::fromType<DBusMenuShortcut>()) {
        return value.value<DBusMenuShortcut>();
    }
    if (value.metaType() == QMetaType::fromType<QDBusArgument>()) {
        DBusMenuShortcut shortcut;
        value.value<QDBusArgument>() >> shortcut;
        return shortcut;
    }
    return {};
}

QString actionText(QString label)
{
    label.replace(QLatin1Char('&'), QStringLiteral("&&"));
    label.replace(QLatin1Char('_'), QLatin1Char('&'));
    return label;
}

QString displayText(QString label)
{
    label.remove(QLatin1Char('_'));
    label.remove(QLatin1Char('&'));
    return label.trimmed();
}
}

GlobalMenuModel::GlobalMenuModel(QObject *parent)
    : QAbstractListModel(parent)
    , m_tasksModel(new TaskManager::TasksModel(this))
    , m_serviceWatcher(new QDBusServiceWatcher(this))
{
    registerDBusMenuTypes();

    m_tasksModel->setFilterByScreen(false);
    connect(m_tasksModel, &TaskManager::TasksModel::activeTaskChanged, this, &GlobalMenuModel::onActiveWindowChanged);
    connect(m_tasksModel,
            &TaskManager::TasksModel::dataChanged,
            this,
            [this](const QModelIndex &, const QModelIndex &, const QList<int> &roles) {
                if (roles.isEmpty()
                    || roles.contains(TaskManager::AbstractTasksModel::ApplicationMenuObjectPath)
                    || roles.contains(TaskManager::AbstractTasksModel::ApplicationMenuServiceName)) {
                    onActiveWindowChanged();
                }
            });
    connect(m_tasksModel, &TaskManager::TasksModel::activityChanged, this, &GlobalMenuModel::onActiveWindowChanged);
    connect(m_tasksModel, &TaskManager::TasksModel::virtualDesktopChanged, this, &GlobalMenuModel::onActiveWindowChanged);
    connect(m_tasksModel, &TaskManager::TasksModel::countChanged, this, &GlobalMenuModel::onActiveWindowChanged);

    m_serviceWatcher->setConnection(QDBusConnection::sessionBus());
    m_serviceWatcher->setWatchMode(QDBusServiceWatcher::WatchForUnregistration);
    connect(m_serviceWatcher, &QDBusServiceWatcher::serviceUnregistered, this, [this](const QString &service) {
        if (service == m_serviceName) {
            setSource({}, {});
        }
    });

    m_refreshTimer.setSingleShot(true);
    m_refreshTimer.setInterval(40);
    connect(&m_refreshTimer, &QTimer::timeout, this, &GlobalMenuModel::requestLayout);

    onActiveWindowChanged();
}

GlobalMenuModel::~GlobalMenuModel()
{
    clearActions();
}

int GlobalMenuModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) {
        return 0;
    }
    return m_topLevelActions.size();
}

QVariant GlobalMenuModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_topLevelActions.size()) {
        return {};
    }

    QAction *action = m_topLevelActions.at(index.row());
    switch (role) {
    case LabelRole:
        return displayText(action->text());
    case ActionRole:
        return QVariant::fromValue(action);
    default:
        return {};
    }
}

QHash<int, QByteArray> GlobalMenuModel::roleNames() const
{
    return {
        {LabelRole, QByteArrayLiteral("label")},
        {ActionRole, QByteArrayLiteral("activeAction")},
    };
}

bool GlobalMenuModel::menuAvailable() const
{
    return m_menuAvailable;
}

void GlobalMenuModel::aboutToShow(int index)
{
    if (index < 0 || index >= m_topLevelIds.size()) {
        return;
    }
    sendAboutToShow(m_topLevelIds.at(index));
}

void GlobalMenuModel::menuOpened(int index)
{
    if (index < 0 || index >= m_topLevelIds.size()) {
        return;
    }
    sendEvent(m_topLevelIds.at(index), QStringLiteral("opened"));
}

void GlobalMenuModel::menuClosed(int index)
{
    if (index < 0 || index >= m_topLevelIds.size()) {
        return;
    }
    sendEvent(m_topLevelIds.at(index), QStringLiteral("closed"));
}

void GlobalMenuModel::onActiveWindowChanged()
{
    const QModelIndex activeTask = m_tasksModel->activeTask();
    if (!activeTask.isValid()) {
        setSource({}, {});
        return;
    }

    const QString serviceName = m_tasksModel
                                    ->data(activeTask, TaskManager::AbstractTasksModel::ApplicationMenuServiceName)
                                    .toString();
    const QString objectPath = m_tasksModel
                                   ->data(activeTask, TaskManager::AbstractTasksModel::ApplicationMenuObjectPath)
                                   .toString();
    setSource(serviceName, objectPath);
}

void GlobalMenuModel::onLayoutUpdated(uint revision, int parentId)
{
    Q_UNUSED(revision)
    Q_UNUSED(parentId)
    scheduleRefresh();
}

void GlobalMenuModel::onItemsPropertiesUpdated(const DBusMenuItemList &updated, const DBusMenuItemKeysList &removed)
{
    Q_UNUSED(updated)
    Q_UNUSED(removed)
    scheduleRefresh();
}

void GlobalMenuModel::scheduleRefresh()
{
    if (m_serviceName.isEmpty() || m_objectPath.isEmpty()) {
        return;
    }
    if (m_requestInFlight) {
        m_refreshQueued = true;
        return;
    }
    m_refreshTimer.start();
}

void GlobalMenuModel::requestLayout()
{
    if (!m_interface || !m_interface->isValid() || m_requestInFlight) {
        return;
    }

    m_requestInFlight = true;
    const quint64 sourceGeneration = m_sourceGeneration;
    const QString requestedService = m_serviceName;
    const QString requestedPath = m_objectPath;
    const QStringList properties = {
        QStringLiteral("label"),
        QStringLiteral("type"),
        QStringLiteral("visible"),
        QStringLiteral("enabled"),
        QStringLiteral("icon-name"),
        QStringLiteral("icon-data"),
        QStringLiteral("toggle-type"),
        QStringLiteral("toggle-state"),
        QStringLiteral("children-display"),
        QStringLiteral("shortcut"),
    };

    QDBusPendingCall call = m_interface->asyncCall(
        QStringLiteral("GetLayout"),
        0,
        -1,
        properties);
    auto *watcher = new QDBusPendingCallWatcher(call, this);

    connect(watcher, &QDBusPendingCallWatcher::finished, this, [this, watcher, sourceGeneration, requestedService, requestedPath] {
        const QDBusPendingReply<uint, DBusMenuLayoutItem> reply = *watcher;
        watcher->deleteLater();

        if (sourceGeneration != m_sourceGeneration
            || requestedService != m_serviceName
            || requestedPath != m_objectPath) {
            return;
        }

        m_requestInFlight = false;
        if (reply.isError()) {
            clearActions();
        } else {
            rebuildActions(reply.argumentAt<1>());
        }

        if (m_refreshQueued) {
            m_refreshQueued = false;
            scheduleRefresh();
        }
    });
}

void GlobalMenuModel::setSource(const QString &serviceName, const QString &objectPath)
{
    if (m_serviceName == serviceName && m_objectPath == objectPath) {
        if (!serviceName.isEmpty()) {
            scheduleRefresh();
        }
        return;
    }

    ++m_sourceGeneration;

    if (!m_serviceName.isEmpty() && !m_objectPath.isEmpty()) {
        QDBusConnection::sessionBus().disconnect(
            m_serviceName,
            m_objectPath,
            QString::fromLatin1(dbusMenuInterface),
            QStringLiteral("LayoutUpdated"),
            this,
            SLOT(onLayoutUpdated(uint,int)));
        QDBusConnection::sessionBus().disconnect(
            m_serviceName,
            m_objectPath,
            QString::fromLatin1(dbusMenuInterface),
            QStringLiteral("ItemsPropertiesUpdated"),
            this,
            SLOT(onItemsPropertiesUpdated(DBusMenuItemList,DBusMenuItemKeysList)));
    }

    m_refreshTimer.stop();
    m_requestInFlight = false;
    m_refreshQueued = false;
    delete m_interface;
    m_interface = nullptr;

    // Reset while the previous service and path are still available so an open
    // popup can emit its final "closed" event to the correct application.
    clearActions();

    m_serviceName = serviceName;
    m_objectPath = objectPath;
    m_serviceWatcher->setWatchedServices(serviceName.isEmpty() ? QStringList{} : QStringList{serviceName});

    if (serviceName.isEmpty() || objectPath.isEmpty()) {
        return;
    }

    m_interface = new QDBusInterface(
        serviceName,
        objectPath,
        QString::fromLatin1(dbusMenuInterface),
        QDBusConnection::sessionBus(),
        this);

    if (!m_interface->isValid()) {
        delete m_interface;
        m_interface = nullptr;
        return;
    }

    QDBusConnection::sessionBus().connect(
        serviceName,
        objectPath,
        QString::fromLatin1(dbusMenuInterface),
        QStringLiteral("LayoutUpdated"),
        this,
        SLOT(onLayoutUpdated(uint,int)));
    QDBusConnection::sessionBus().connect(
        serviceName,
        objectPath,
        QString::fromLatin1(dbusMenuInterface),
        QStringLiteral("ItemsPropertiesUpdated"),
        this,
        SLOT(onItemsPropertiesUpdated(DBusMenuItemList,DBusMenuItemKeysList)));

    scheduleRefresh();
}

void GlobalMenuModel::clearActions()
{
    const bool hadRows = !m_topLevelActions.isEmpty();
    if (hadRows) {
        beginResetModel();
    }

    qDeleteAll(m_topLevelActions);
    m_topLevelActions.clear();
    m_topLevelIds.clear();
    qDeleteAll(m_ownedMenus);
    m_ownedMenus.clear();

    if (hadRows) {
        endResetModel();
    }
    setMenuAvailable(false);
}

void GlobalMenuModel::rebuildActions(const DBusMenuLayoutItem &root)
{
    beginResetModel();
    qDeleteAll(m_topLevelActions);
    m_topLevelActions.clear();
    m_topLevelIds.clear();
    qDeleteAll(m_ownedMenus);
    m_ownedMenus.clear();

    for (const DBusMenuLayoutItem &item : root.children) {
        if (!boolProperty(item, QStringLiteral("visible"), true)) {
            continue;
        }
        if (property(item, QStringLiteral("type")).toString() == QLatin1String("separator")) {
            continue;
        }
        if (displayText(property(item, QStringLiteral("label")).toString()).isEmpty()) {
            continue;
        }

        if (QAction *action = buildAction(item, this)) {
            m_topLevelActions.append(action);
            m_topLevelIds.append(item.id);
        }
    }

    endResetModel();
    setMenuAvailable(!m_topLevelActions.isEmpty());
}

QAction *GlobalMenuModel::buildAction(const DBusMenuLayoutItem &item, QObject *owner)
{
    auto *action = new QAction(actionText(property(item, QStringLiteral("label")).toString()), owner);
    action->setVisible(boolProperty(item, QStringLiteral("visible"), true));
    action->setEnabled(boolProperty(item, QStringLiteral("enabled"), true));

    const QString type = property(item, QStringLiteral("type")).toString();
    if (type == QLatin1String("separator")) {
        action->setSeparator(true);
        return action;
    }

    const QString iconName = property(item, QStringLiteral("icon-name")).toString();
    if (!iconName.isEmpty()) {
        action->setIcon(QIcon::fromTheme(iconName));
    }

    const QByteArray iconData = property(item, QStringLiteral("icon-data")).toByteArray();
    if (!iconData.isEmpty()) {
        QPixmap pixmap;
        if (pixmap.loadFromData(iconData)) {
            action->setIcon(QIcon(pixmap));
        }
    }

    const DBusMenuShortcut shortcut = shortcutProperty(item);
    if (!shortcut.isEmpty()) {
        action->setShortcut(shortcut.toKeySequence());
    }

    const QString toggleType = property(item, QStringLiteral("toggle-type")).toString();
    if (!toggleType.isEmpty()) {
        action->setCheckable(true);
        action->setChecked(property(item, QStringLiteral("toggle-state")).toInt() == 1);
    }

    const bool isSubmenu = !item.children.isEmpty()
        || property(item, QStringLiteral("children-display")).toString() == QLatin1String("submenu");
    if (isSubmenu) {
        auto *menu = new QMenu;
        m_ownedMenus.append(menu);
        QActionGroup *radioGroup = nullptr;

        for (const DBusMenuLayoutItem &child : item.children) {
            QAction *childAction = buildAction(child, menu);
            if (!childAction) {
                continue;
            }

            if (childAction->isSeparator()) {
                radioGroup = nullptr;
            } else if (property(child, QStringLiteral("toggle-type")).toString() == QLatin1String("radio")) {
                if (!radioGroup) {
                    radioGroup = new QActionGroup(menu);
                    radioGroup->setExclusive(true);
                }
                radioGroup->addAction(childAction);
            } else {
                radioGroup = nullptr;
            }

            menu->addAction(childAction);
        }
        connect(menu, &QMenu::aboutToShow, this, [this, id = item.id] {
            sendAboutToShow(id);
            sendEvent(id, QStringLiteral("opened"));
        });
        connect(menu, &QMenu::aboutToHide, this, [this, id = item.id] {
            sendEvent(id, QStringLiteral("closed"));
        });
        action->setMenu(menu);
    } else {
        connect(action, &QAction::triggered, this, [this, id = item.id] {
            sendClicked(id);
        });
    }

    return action;
}

void GlobalMenuModel::sendEvent(int id, const QString &eventName)
{
    if (m_serviceName.isEmpty() || m_objectPath.isEmpty()) {
        return;
    }

    QDBusMessage message = QDBusMessage::createMethodCall(
        m_serviceName,
        m_objectPath,
        QString::fromLatin1(dbusMenuInterface),
        QStringLiteral("Event"));
    message << id
            << eventName
            << QVariant::fromValue(QDBusVariant(QString()))
            << 0u;
    QDBusConnection::sessionBus().asyncCall(message);
}

void GlobalMenuModel::sendClicked(int id)
{
    sendEvent(id, QStringLiteral("clicked"));
}

void GlobalMenuModel::sendAboutToShow(int id)
{
    if (!m_interface || !m_interface->isValid()) {
        return;
    }

    const quint64 sourceGeneration = m_sourceGeneration;
    QDBusPendingCall call = m_interface->asyncCall(QStringLiteral("AboutToShow"), id);
    auto *watcher = new QDBusPendingCallWatcher(call, this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this, [this, watcher, sourceGeneration] {
        const QDBusPendingReply<bool> reply = *watcher;
        watcher->deleteLater();
        if (sourceGeneration == m_sourceGeneration && !reply.isError() && reply.value()) {
            scheduleRefresh();
        }
    });
}

void GlobalMenuModel::setMenuAvailable(bool available)
{
    if (m_menuAvailable == available) {
        return;
    }
    m_menuAvailable = available;
    Q_EMIT menuAvailableChanged();
}
