// SPDX-FileCopyrightText: 2026 ChathurangaBW
// SPDX-License-Identifier: GPL-2.0-or-later
#include "displaymenumodel.h"

#include "globalmenumodel.h"

#include <KLocalizedString>

#include <QAction>
#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusPendingCall>
#include <QDesktopServices>
#include <QDir>
#include <QMenu>
#include <QProcess>
#include <QStandardPaths>
#include <QUrl>

namespace
{
QString displayText(QString text)
{
    text.remove(QLatin1Char('&'));
    return text.trimmed();
}

void addUrlAction(QMenu *menu, const QString &text, const QUrl &url)
{
    QAction *action = menu->addAction(text);
    action->setEnabled(url.isValid() && !url.isEmpty());
    QObject::connect(action, &QAction::triggered, menu, [url] {
        QDesktopServices::openUrl(url);
    });
}

void addLocationAction(QMenu *menu, const QString &text, QStandardPaths::StandardLocation location)
{
    const QString path = QStandardPaths::writableLocation(location);
    QAction *action = menu->addAction(text);
    action->setEnabled(!path.isEmpty());
    QObject::connect(action, &QAction::triggered, menu, [path] {
        if (!path.isEmpty()) {
            QDesktopServices::openUrl(QUrl::fromLocalFile(path));
        }
    });
}

void addProgramAction(QMenu *menu, const QString &text, const QString &program, const QStringList &arguments = {})
{
    const QString executable = QStandardPaths::findExecutable(program);
    QAction *action = menu->addAction(text);
    action->setEnabled(!executable.isEmpty());
    QObject::connect(action, &QAction::triggered, menu, [executable, arguments] {
        if (!executable.isEmpty()) {
            QProcess::startDetached(executable, arguments);
        }
    });
}

void callSessionBus(const QString &service, const QString &path, const QString &interface, const QString &method, const QVariantList &arguments = {})
{
    QDBusMessage message = QDBusMessage::createMethodCall(service, path, interface, method);
    message.setArguments(arguments);
    QDBusConnection::sessionBus().asyncCall(message);
}
}

DisplayMenuModel::DisplayMenuModel(GlobalMenuModel *applicationModel, QObject *parent)
    : QAbstractListModel(parent)
    , m_applicationModel(applicationModel)
{
    buildDesktopFallback();

    connect(m_applicationModel, &QAbstractItemModel::modelReset, this, &DisplayMenuModel::rebuildView);
    connect(m_applicationModel, &GlobalMenuModel::menuAvailableChanged, this, &DisplayMenuModel::rebuildView);
    connect(m_applicationModel,
            &QAbstractItemModel::dataChanged,
            this,
            [this](const QModelIndex &topLeft, const QModelIndex &bottomRight, const QList<int> &roles) {
                if (!usingApplicationMenu()) {
                    return;
                }
                Q_EMIT dataChanged(index(topLeft.row(), 0), index(bottomRight.row(), 0), roles);
            });
}

DisplayMenuModel::~DisplayMenuModel()
{
    qDeleteAll(m_fallbackActions);
    qDeleteAll(m_fallbackMenus);
}

int DisplayMenuModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) {
        return 0;
    }
    return usingApplicationMenu() ? m_applicationModel->rowCount() : m_fallbackActions.size();
}

QVariant DisplayMenuModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= rowCount()) {
        return {};
    }

    if (usingApplicationMenu()) {
        const QModelIndex sourceIndex = m_applicationModel->index(index.row(), 0);
        switch (role) {
        case LabelRole:
            return m_applicationModel->data(sourceIndex, GlobalMenuModel::LabelRole);
        case ActionRole:
            return m_applicationModel->data(sourceIndex, GlobalMenuModel::ActionRole);
        default:
            return {};
        }
    }

    QAction *action = m_fallbackActions.at(index.row());
    switch (role) {
    case LabelRole:
        return displayText(action->text());
    case ActionRole:
        return QVariant::fromValue(action);
    default:
        return {};
    }
}

QHash<int, QByteArray> DisplayMenuModel::roleNames() const
{
    return {
        {LabelRole, QByteArrayLiteral("label")},
        {ActionRole, QByteArrayLiteral("activeAction")},
    };
}

QAction *DisplayMenuModel::actionForIndex(int index) const
{
    if (index < 0 || index >= rowCount()) {
        return nullptr;
    }

    if (usingApplicationMenu()) {
        return m_applicationModel
            ->data(m_applicationModel->index(index, 0), GlobalMenuModel::ActionRole)
            .value<QAction *>();
    }

    return m_fallbackActions.at(index);
}

bool DisplayMenuModel::usingApplicationMenu() const
{
    return m_applicationModel && m_applicationModel->menuAvailable() && m_applicationModel->rowCount() > 0;
}

void DisplayMenuModel::aboutToShow(int index)
{
    if (usingApplicationMenu()) {
        m_applicationModel->aboutToShow(index);
    }
}

void DisplayMenuModel::menuOpened(int index)
{
    if (usingApplicationMenu()) {
        m_applicationModel->menuOpened(index);
    }
}

void DisplayMenuModel::menuClosed(int index)
{
    if (usingApplicationMenu()) {
        m_applicationModel->menuClosed(index);
    }
}

void DisplayMenuModel::rebuildView()
{
    beginResetModel();
    endResetModel();
}

void DisplayMenuModel::buildDesktopFallback()
{
    const auto addTopLevel = [this](const QString &text) {
        auto *action = new QAction(text, this);
        auto *menu = new QMenu;
        action->setMenu(menu);
        m_fallbackActions.append(action);
        m_fallbackMenus.append(menu);
        return menu;
    };

    QMenu *fileMenu = addTopLevel(i18n("&File"));
    addUrlAction(fileMenu, i18n("Home Folder"), QUrl::fromLocalFile(QDir::homePath()));
    addLocationAction(fileMenu, i18n("Documents"), QStandardPaths::DocumentsLocation);
    addLocationAction(fileMenu, i18n("Downloads"), QStandardPaths::DownloadLocation);
    fileMenu->addSeparator();
    addUrlAction(fileMenu, i18n("Trash"), QUrl(QStringLiteral("trash:/")));

    QMenu *editMenu = addTopLevel(i18n("&Edit"));
    QAction *clipboardAction = editMenu->addAction(i18n("Clipboard History"));
    QObject::connect(clipboardAction, &QAction::triggered, editMenu, [] {
        callSessionBus(
            QStringLiteral("org.kde.klipper"),
            QStringLiteral("/klipper"),
            QStringLiteral("org.kde.klipper.klipper"),
            QStringLiteral("showKlipperPopupMenu"));
    });

    QMenu *viewMenu = addTopLevel(i18n("&View"));
    QAction *showDesktopAction = viewMenu->addAction(i18n("Show Desktop"));
    QObject::connect(showDesktopAction, &QAction::triggered, viewMenu, [] {
        callSessionBus(
            QStringLiteral("org.kde.KWin"),
            QStringLiteral("/KWin"),
            QStringLiteral("org.kde.KWin"),
            QStringLiteral("showDesktop"),
            {true});
    });
    QAction *restoreWindowsAction = viewMenu->addAction(i18n("Restore Windows"));
    QObject::connect(restoreWindowsAction, &QAction::triggered, viewMenu, [] {
        callSessionBus(
            QStringLiteral("org.kde.KWin"),
            QStringLiteral("/KWin"),
            QStringLiteral("org.kde.KWin"),
            QStringLiteral("showDesktop"),
            {false});
    });

    QMenu *goMenu = addTopLevel(i18n("&Go"));
    addUrlAction(goMenu, i18n("Home"), QUrl::fromLocalFile(QDir::homePath()));
    addLocationAction(goMenu, i18n("Documents"), QStandardPaths::DocumentsLocation);
    addLocationAction(goMenu, i18n("Downloads"), QStandardPaths::DownloadLocation);
    addUrlAction(goMenu, i18n("Trash"), QUrl(QStringLiteral("trash:/")));

    QMenu *toolsMenu = addTopLevel(i18n("&Tools"));
    addProgramAction(toolsMenu, i18n("Run Command…"), QStringLiteral("krunner"));
    addProgramAction(toolsMenu, i18n("Konsole"), QStringLiteral("konsole"));
    addProgramAction(toolsMenu, i18n("System Monitor"), QStringLiteral("plasma-systemmonitor"));

    QMenu *settingsMenu = addTopLevel(i18n("&Settings"));
    addProgramAction(settingsMenu, i18n("System Settings"), QStringLiteral("systemsettings"));

    QMenu *helpMenu = addTopLevel(i18n("&Help"));
    addProgramAction(helpMenu, i18n("KDE Help Center"), QStringLiteral("khelpcenter"));
}
