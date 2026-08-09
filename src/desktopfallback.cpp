// SPDX-FileCopyrightText: 2026 ChathurangaBW
// SPDX-License-Identifier: GPL-2.0-or-later

#include "desktopfallback.h"

#include <KLocalizedString>

#include <QAction>
#include <QDBusConnection>
#include <QDBusMessage>
#include <QDesktopServices>
#include <QDir>
#include <QMenu>
#include <QProcess>
#include <QStandardPaths>
#include <QUrl>

namespace
{
void addUrlAction(QMenu *menu, const QString &text, const QUrl &url)
{
    QAction *action = menu->addAction(text);
    action->setEnabled(url.isValid() && !url.isEmpty());
    QObject::connect(action, &QAction::triggered, action, [url] {
        QDesktopServices::openUrl(url);
    });
}

void addLocationAction(QMenu *menu, const QString &text, QStandardPaths::StandardLocation location)
{
    const QString path = QStandardPaths::writableLocation(location);
    QAction *action = menu->addAction(text);
    action->setEnabled(!path.isEmpty());
    QObject::connect(action, &QAction::triggered, action, [path] {
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
    QObject::connect(action, &QAction::triggered, action, [executable, arguments] {
        if (!executable.isEmpty()) {
            QProcess::startDetached(executable, arguments);
        }
    });
}

void callSessionBus(const QString &service,
                    const QString &path,
                    const QString &interface,
                    const QString &method,
                    const QVariantList &arguments = {})
{
    QDBusMessage message = QDBusMessage::createMethodCall(service, path, interface, method);
    message.setArguments(arguments);
    QDBusConnection::sessionBus().asyncCall(message);
}
}

DesktopFallback::DesktopFallback(QObject *parent)
    : QObject(parent)
    , m_menu(std::make_unique<QMenu>())
{
    QMenu *fileMenu = m_menu->addMenu(i18n("&File"));
    addUrlAction(fileMenu, i18n("Home Folder"), QUrl::fromLocalFile(QDir::homePath()));
    addLocationAction(fileMenu, i18n("Documents"), QStandardPaths::DocumentsLocation);
    addLocationAction(fileMenu, i18n("Downloads"), QStandardPaths::DownloadLocation);
    fileMenu->addSeparator();
    addUrlAction(fileMenu, i18n("Trash"), QUrl(QStringLiteral("trash:/")));

    QMenu *editMenu = m_menu->addMenu(i18n("&Edit"));
    QAction *clipboardAction = editMenu->addAction(i18n("Clipboard History"));
    QObject::connect(clipboardAction, &QAction::triggered, clipboardAction, [] {
        callSessionBus(QStringLiteral("org.kde.klipper"),
                       QStringLiteral("/klipper"),
                       QStringLiteral("org.kde.klipper.klipper"),
                       QStringLiteral("showKlipperPopupMenu"));
    });

    QMenu *viewMenu = m_menu->addMenu(i18n("&View"));
    QAction *showDesktopAction = viewMenu->addAction(i18n("Show Desktop"));
    QObject::connect(showDesktopAction, &QAction::triggered, showDesktopAction, [] {
        callSessionBus(QStringLiteral("org.kde.KWin"),
                       QStringLiteral("/KWin"),
                       QStringLiteral("org.kde.KWin"),
                       QStringLiteral("showDesktop"),
                       {true});
    });
    QAction *restoreWindowsAction = viewMenu->addAction(i18n("Restore Windows"));
    QObject::connect(restoreWindowsAction, &QAction::triggered, restoreWindowsAction, [] {
        callSessionBus(QStringLiteral("org.kde.KWin"),
                       QStringLiteral("/KWin"),
                       QStringLiteral("org.kde.KWin"),
                       QStringLiteral("showDesktop"),
                       {false});
    });

    QMenu *goMenu = m_menu->addMenu(i18n("&Go"));
    // File owns the user's common folders. Keep Go for navigation targets so
    // the fallback does not present the same locations twice.
    addUrlAction(goMenu, i18n("Root Filesystem"), QUrl(QStringLiteral("file:///")));
    addUrlAction(goMenu, i18n("Network"), QUrl(QStringLiteral("network:/")));
    addUrlAction(goMenu, i18n("Recent Locations"), QUrl(QStringLiteral("recentlyused:/")));

    QMenu *toolsMenu = m_menu->addMenu(i18n("&Tools"));
    addProgramAction(toolsMenu, i18n("Run Command…"), QStringLiteral("krunner"));
    addProgramAction(toolsMenu, i18n("Konsole"), QStringLiteral("konsole"));
    addProgramAction(toolsMenu, i18n("System Monitor"), QStringLiteral("plasma-systemmonitor"));

    QMenu *settingsMenu = m_menu->addMenu(i18n("&Settings"));
    addProgramAction(settingsMenu, i18n("System Settings"), QStringLiteral("systemsettings"));

    QMenu *helpMenu = m_menu->addMenu(i18n("&Help"));
    addProgramAction(helpMenu, i18n("KDE Help Center"), QStringLiteral("khelpcenter"));
}

DesktopFallback::~DesktopFallback() = default;

QMenu *DesktopFallback::menu() const
{
    return m_menu.get();
}
