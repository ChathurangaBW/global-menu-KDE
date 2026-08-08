/* This file is part of the dbusmenu-qt library
    SPDX-FileCopyrightText: 2009 Canonical
    SPDX-FileContributor: Aurelien Gateau <aurelien.gateau@canonical.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/
#pragma once

#include <QObject>

class QAction;
class QDBusPendingCallWatcher;
class QIcon;
class QMenu;

class DBusMenuImporterPrivate;

class DBusMenuImporter : public QObject
{
    Q_OBJECT
public:
    DBusMenuImporter(const QString &service, const QString &path, QObject *parent = nullptr);
    ~DBusMenuImporter() override;

    QAction *actionForId(int id) const;
    QMenu *menu() const;

public Q_SLOTS:
    void updateMenu();
    void updateMenu(QMenu *menu);

Q_SIGNALS:
    void menuUpdated(QMenu *);
    void actionActivationRequested(QAction *);

protected:
    virtual QMenu *createMenu(QWidget *parent);
    virtual QIcon iconForName(const QString &);
    virtual void actionActivated(int id);
    void sendClickedEvent(int id);

private Q_SLOTS:
    void slotMenuAboutToShow();
    void slotMenuAboutToHide();
    void slotAboutToShowDBusCallFinished(QDBusPendingCallWatcher *);
    void slotItemActivationRequested(int id, uint timestamp);
    void processPendingLayoutUpdates();
    void slotLayoutUpdated(uint revision, int parentId);
    void slotGetLayoutFinished(QDBusPendingCallWatcher *);

private:
    Q_DISABLE_COPY(DBusMenuImporter)
    DBusMenuImporterPrivate *const d;
    friend class DBusMenuImporterPrivate;

    Q_PRIVATE_SLOT(d, void slotItemsPropertiesUpdated(const DBusMenuItemList &updatedList, const DBusMenuItemKeysList &removedList))
};
