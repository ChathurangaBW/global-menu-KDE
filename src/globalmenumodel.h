// SPDX-FileCopyrightText: 2016 Kai Uwe Broulik <kde@privat.broulik.de>
// SPDX-FileCopyrightText: 2016 Chinmoy Ranjan Pradhan <chinmoyrp65@gmail.com>
// SPDX-FileCopyrightText: 2026 ChathurangaBW
// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include "dbusmenutypes.h"

#include <QAbstractListModel>
#include <QPointer>
#include <QString>
#include <QTimer>

class QAction;
class QDBusInterface;
class QDBusServiceWatcher;
class QMenu;

namespace TaskManager
{
class TasksModel;
}

class GlobalMenuModel final : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(bool menuAvailable READ menuAvailable NOTIFY menuAvailableChanged)

public:
    enum Role {
        LabelRole = Qt::UserRole + 1,
        ActionRole,
    };
    Q_ENUM(Role)

    explicit GlobalMenuModel(QObject *parent = nullptr);
    ~GlobalMenuModel() override;

    [[nodiscard]] int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    [[nodiscard]] QVariant data(const QModelIndex &index, int role) const override;
    [[nodiscard]] QHash<int, QByteArray> roleNames() const override;
    [[nodiscard]] bool menuAvailable() const;

    void aboutToShow(int index);
    void menuOpened(int index);
    void menuClosed(int index);

Q_SIGNALS:
    void menuAvailableChanged();

private Q_SLOTS:
    void onActiveWindowChanged();
    void onLayoutUpdated(uint revision, int parentId);
    void onItemsPropertiesUpdated(const DBusMenuItemList &updated, const DBusMenuItemKeysList &removed);
    void scheduleRefresh();
    void requestLayout();

private:
    [[nodiscard]] static QVariant property(const DBusMenuLayoutItem &item, const QString &name);
    void setSource(const QString &serviceName, const QString &objectPath);
    void clearActions();
    void rebuildActions(const DBusMenuLayoutItem &root);
    QAction *buildAction(const DBusMenuLayoutItem &item, QObject *owner);
    void sendEvent(int id, const QString &eventName);
    void sendClicked(int id);
    void sendAboutToShow(int id);
    void setMenuAvailable(bool available);

    TaskManager::TasksModel *m_tasksModel = nullptr;
    QDBusServiceWatcher *m_serviceWatcher = nullptr;
    QDBusInterface *m_interface = nullptr;

    QString m_serviceName;
    QString m_objectPath;

    QList<QAction *> m_topLevelActions;
    QList<int> m_topLevelIds;
    QList<QMenu *> m_ownedMenus;

    QTimer m_refreshTimer;
    quint64 m_sourceGeneration = 0;
    bool m_menuAvailable = false;
    bool m_requestInFlight = false;
    bool m_refreshQueued = false;
};
