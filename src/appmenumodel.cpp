/*
    SPDX-FileCopyrightText: 2016 Kai Uwe Broulik <kde@privat.broulik.de>
    SPDX-FileCopyrightText: 2016 Chinmoy Ranjan Pradhan <chinmoyrp65@gmail.com>
    SPDX-FileCopyrightText: 2026 ChathurangaBW

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#include "appmenumodel.h"
#include "desktopfallback.h"

#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDBusServiceWatcher>
#include <QGuiApplication>
#include <QMenu>

#include <KLocalizedString>
#include <QLineEdit>
#include <QListView>
#include <QWidgetAction>
#include <algorithm>

#include <abstracttasksmodel.h>
#include <dbusmenuimporter.h>

class KDBusMenuImporter : public DBusMenuImporter
{
public:
    KDBusMenuImporter(const QString &service, const QString &path)
        : DBusMenuImporter(service, path)
    {
    }

protected:
    QIcon iconForName(const QString &name) override
    {
        return QIcon::fromTheme(name);
    }
};

AppMenuModel::AppMenuModel(QObject *parent)
    : QAbstractListModel(parent)
    , m_tasksModel(new TaskManager::TasksModel(this))
    , m_desktopFallback(std::make_unique<DesktopFallback>())
    , m_serviceWatcher(new QDBusServiceWatcher(this))
{
    m_tasksModel->setFilterByScreen(!m_allScreens);
    connect(m_tasksModel, &TaskManager::TasksModel::activeTaskChanged, this, &AppMenuModel::onActiveWindowChanged);
    connect(m_tasksModel,
            &TaskManager::TasksModel::dataChanged,
            this,
            [this](const QModelIndex &topLeft, const QModelIndex &bottomRight, const QList<int> &roles = QList<int>()) {
                Q_UNUSED(topLeft)
                Q_UNUSED(bottomRight)
                if (roles.contains(TaskManager::AbstractTasksModel::ApplicationMenuObjectPath)
                    || roles.contains(TaskManager::AbstractTasksModel::ApplicationMenuServiceName) || roles.isEmpty()) {
                    onActiveWindowChanged();
                }
            });
    connect(m_tasksModel, &TaskManager::TasksModel::activityChanged, this, &AppMenuModel::onActiveWindowChanged);
    connect(m_tasksModel, &TaskManager::TasksModel::virtualDesktopChanged, this, &AppMenuModel::onActiveWindowChanged);
    connect(m_tasksModel, &TaskManager::TasksModel::countChanged, this, &AppMenuModel::onActiveWindowChanged);
    connect(m_tasksModel, &TaskManager::TasksModel::screenGeometryChanged, this, &AppMenuModel::screenGeometryChanged);

    connect(this, &AppMenuModel::modelNeedsUpdate, this, [this] {
        if (!m_updatePending) {
            m_updatePending = true;
            QMetaObject::invokeMethod(this, "update", Qt::QueuedConnection);
        }
    });

    showDesktopFallback();
    onActiveWindowChanged();

    m_serviceWatcher->setConnection(QDBusConnection::sessionBus());
    connect(m_serviceWatcher, &QDBusServiceWatcher::serviceUnregistered, this, [this](const QString &serviceName) {
        if (serviceName == m_serviceName) {
            updateApplicationMenu(QString(), QString());
        }
    });

    // Preserve KDE's native Wayland menu-search behavior for real application
    // menus. The desktop fallback itself remains exactly seven top-level menus.
    if (KWindowSystem::isPlatformWayland()) {
        m_searchAction = new QAction(this);
        m_searchAction->setText(i18n("Search"));
        m_searchAction->setObjectName(QStringLiteral("appmenu"));

        m_searchMenu.reset(new QMenu);
        auto *searchAction = new QWidgetAction(this);
        auto *searchBar = new QLineEdit;
        searchBar->setClearButtonEnabled(true);
        searchBar->setPlaceholderText(i18n("Search…"));
        searchBar->setMinimumWidth(200);
        searchBar->setContentsMargins(4, 4, 4, 4);
        connect(m_tasksModel, &TaskManager::TasksModel::activeTaskChanged, searchBar, [searchBar]() {
            searchBar->setText(QString());
        });
        connect(searchBar, &QLineEdit::textChanged, this, [searchBar, this]() mutable {
            insertSearchActionsIntoMenu(searchBar->text());
        });
        connect(searchBar, &QLineEdit::returnPressed, this, [this]() mutable {
            if (!m_currentSearchActions.empty()) {
                m_currentSearchActions.constFirst()->trigger();
            }
        });
        connect(this, &AppMenuModel::modelNeedsUpdate, searchBar, [this, searchBar]() mutable {
            insertSearchActionsIntoMenu(searchBar->text());
        });
        searchAction->setDefaultWidget(searchBar);
        m_searchMenu->addAction(searchAction);
        m_searchMenu->addSeparator();
        m_searchAction->setMenu(m_searchMenu.get());
    }
}

AppMenuModel::~AppMenuModel() = default;

bool AppMenuModel::menuAvailable() const
{
    return m_menuAvailable;
}

void AppMenuModel::setMenuAvailable(bool set)
{
    if (m_menuAvailable != set) {
        m_menuAvailable = set;
        setVisible(true);
        Q_EMIT menuAvailableChanged();
    }
}

bool AppMenuModel::allScreens() const
{
    return m_allScreens;
}

void AppMenuModel::setallScreens(bool allScreens)
{
    if (m_allScreens == allScreens) {
        return;
    }

    m_allScreens = allScreens;
    m_tasksModel->setFilterByScreen(!m_allScreens);
    Q_EMIT allScreensChanged();
}

bool AppMenuModel::visible() const
{
    return m_visible;
}

void AppMenuModel::setVisible(bool visible)
{
    if (m_visible != visible) {
        m_visible = visible;
        Q_EMIT visibleChanged();
    }
}

QRect AppMenuModel::screenGeometry() const
{
    return m_tasksModel->screenGeometry();
}

void AppMenuModel::setScreenGeometry(QRect geometry)
{
    m_tasksModel->setScreenGeometry(geometry);
}

bool AppMenuModel::usingDesktopFallback() const
{
    return m_desktopFallback && m_menu == m_desktopFallback->menu();
}

int AppMenuModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    if (!m_menuAvailable || !m_menu) {
        return 0;
    }

    const bool includeSearch = m_searchAction && !usingDesktopFallback();
    return m_menu->actions().count() + (includeSearch ? 1 : 0);
}

void AppMenuModel::removeSearchActionsFromMenu()
{
    if (!m_searchAction || !m_searchAction->menu()) {
        m_currentSearchActions.clear();
        return;
    }

    for (auto action : std::as_const(m_currentSearchActions)) {
        m_searchAction->menu()->removeAction(action);
    }
    m_currentSearchActions.clear();
}

void AppMenuModel::insertSearchActionsIntoMenu(const QString &filter)
{
    removeSearchActionsFromMenu();
    if (filter.isEmpty() || usingDesktopFallback()) {
        return;
    }

    const auto actions = flatActionList();
    for (const auto &action : actions) {
        if (action->text().contains(filter, Qt::CaseInsensitive)) {
            m_searchAction->menu()->addAction(action);
            m_currentSearchActions << action;
        }
    }
}

void AppMenuModel::update()
{
    beginResetModel();
    endResetModel();
    m_updatePending = false;
}

void AppMenuModel::showDesktopFallback()
{
    QMenu *fallback = m_desktopFallback->menu();
    const bool modelChanged = m_menu != fallback;
    m_menu = fallback;
    setMenuAvailable(true);
    setVisible(true);
    if (modelChanged) {
        Q_EMIT modelNeedsUpdate();
    }
}

void AppMenuModel::onActiveWindowChanged()
{
    // Keep KDE's native rule: panel focus itself must not replace the active
    // application's menu selection.
    if (m_containmentStatus == Plasma::Types::AcceptingInputStatus) {
        return;
    }

    const QModelIndex activeTaskIndex = m_tasksModel->activeTask();
    const QString objectPath = m_tasksModel->data(activeTaskIndex, TaskManager::AbstractTasksModel::ApplicationMenuObjectPath).toString();
    const QString serviceName = m_tasksModel->data(activeTaskIndex, TaskManager::AbstractTasksModel::ApplicationMenuServiceName).toString();
    updateApplicationMenu(serviceName, objectPath);
}

QHash<int, QByteArray> AppMenuModel::roleNames() const
{
    QHash<int, QByteArray> roleNames;
    roleNames[MenuRole] = QByteArrayLiteral("activeMenu");
    roleNames[ActionRole] = QByteArrayLiteral("activeActions");
    return roleNames;
}

QList<QAction *> AppMenuModel::flatActionList()
{
    QList<QAction *> ret;
    if (!m_menuAvailable || !m_menu) {
        return ret;
    }
    const auto actions = m_menu->findChildren<QAction *>();
    for (auto &action : actions) {
        if (action->menu() == nullptr) {
            ret << action;
        }
    }
    return ret;
}

QVariant AppMenuModel::data(const QModelIndex &index, int role) const
{
    if (!m_menuAvailable || !m_menu) {
        return {};
    }

    if (!index.isValid()) {
        if (role == MenuRole) {
            return QString();
        } else if (role == ActionRole) {
            return QVariant::fromValue(m_menu->menuAction());
        }
    }

    const auto actions = m_menu->actions();
    const int row = index.row();
    const bool includeSearch = m_searchAction && !usingDesktopFallback();
    if (row == actions.count() && includeSearch) {
        if (role == MenuRole) {
            return m_searchAction->text();
        } else if (role == ActionRole) {
            return QVariant::fromValue(m_searchAction.data());
        }
    }
    if (row < 0 || row >= actions.count()) {
        return {};
    }

    if (role == MenuRole) {
        return actions.at(row)->text();
    } else if (role == ActionRole) {
        return QVariant::fromValue(actions.at(row));
    }

    return {};
}

void AppMenuModel::connectApplicationActions()
{
    if (!m_importer || !m_menu || usingDesktopFallback()) {
        return;
    }

    const auto actions = m_menu->actions();
    for (QAction *action : actions) {
        connect(action, &QAction::changed, this, [this, action] {
            if (m_menuAvailable && m_menu && !usingDesktopFallback()) {
                const int actionIdx = m_menu->actions().indexOf(action);
                if (actionIdx > -1) {
                    const QModelIndex modelIdx = index(actionIdx, 0);
                    Q_EMIT dataChanged(modelIdx, modelIdx);
                }
            }
        });

        connect(action, &QAction::destroyed, this, &AppMenuModel::modelNeedsUpdate);

        if (action->menu()) {
            m_importer->updateMenu(action->menu());
        }
    }
}

void AppMenuModel::updateApplicationMenu(const QString &serviceName, const QString &menuObjectPath)
{
    if (m_serviceName == serviceName && m_menuObjectPath == menuObjectPath) {
        if (m_importer) {
            QMetaObject::invokeMethod(m_importer.get(), "updateMenu", Qt::QueuedConnection);
        }
        return;
    }

    // Leave no pointer into a menu owned by the previous importer while it is
    // being destroyed. The desktop menu is also what the user should see while
    // a newly focused application's menu is loading.
    showDesktopFallback();
    m_importer.reset();
    removeSearchActionsFromMenu();

    if (serviceName.isEmpty() || menuObjectPath.isEmpty()) {
        m_serviceName.clear();
        m_menuObjectPath.clear();
        m_serviceWatcher->setWatchedServices({});
        return;
    }

    m_serviceName = serviceName;
    m_menuObjectPath = menuObjectPath;
    m_serviceWatcher->setWatchedServices(QStringList({m_serviceName}));

    m_importer = std::make_unique<KDBusMenuImporter>(serviceName, menuObjectPath);
    QMetaObject::invokeMethod(m_importer.get(), "updateMenu", Qt::QueuedConnection);

    connect(m_importer.get(), &DBusMenuImporter::menuUpdated, this, [this](QMenu *menu) {
        if (!m_importer) {
            return;
        }

        QMenu *applicationMenu = m_importer->menu();
        if (!applicationMenu || menu != applicationMenu) {
            return;
        }

        if (applicationMenu->isEmpty()) {
            // Apps that export an empty menu should behave like the desktop:
            // keep the fallback until a usable menu arrives.
            showDesktopFallback();
            return;
        }

        m_menu = applicationMenu;
        connectApplicationActions();
        setMenuAvailable(true);
        setVisible(true);
        Q_EMIT modelNeedsUpdate();
    });

    connect(m_importer.get(), &DBusMenuImporter::actionActivationRequested, this, [this](QAction *action) {
        if (!m_menuAvailable || !m_menu || usingDesktopFallback()) {
            return;
        }

        const auto actions = m_menu->actions();
        auto it = std::ranges::find(actions, action);
        if (it != actions.end()) {
            Q_EMIT requestActivateIndex(it - actions.begin());
        }
    });
}

#include "moc_appmenumodel.cpp"
