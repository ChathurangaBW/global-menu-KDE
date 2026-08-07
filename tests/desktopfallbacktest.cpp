// SPDX-FileCopyrightText: 2026 ChathurangaBW
// SPDX-License-Identifier: GPL-2.0-or-later

#include "desktopfallback.h"

#include <QAction>
#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusVirtualObject>
#include <QDesktopServices>
#include <QDir>
#include <QFile>
#include <QMenu>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QUrl>
#include <QtTest>

namespace
{
QString plainLabel(QString text)
{
    text.remove(QLatin1Char('&'));
    return text;
}

QMenu *menuByLabel(QMenu *root, const QString &label)
{
    for (QAction *action : root->actions()) {
        if (plainLabel(action->text()) == label) {
            return action->menu();
        }
    }
    return nullptr;
}

QAction *actionByLabel(QMenu *menu, const QString &label)
{
    if (!menu) {
        return nullptr;
    }
    for (QAction *action : menu->actions()) {
        if (!action->isSeparator() && plainLabel(action->text()) == label) {
            return action;
        }
    }
    return nullptr;
}

QStringList actionLabels(QMenu *menu)
{
    QStringList labels;
    if (!menu) {
        return labels;
    }
    for (QAction *action : menu->actions()) {
        if (!action->isSeparator()) {
            labels << plainLabel(action->text());
        }
    }
    return labels;
}

class UrlSink final : public QObject
{
    Q_OBJECT

public:
    QList<QUrl> urls;

public Q_SLOTS:
    void capture(const QUrl &url)
    {
        urls << url;
    }
};

class KWinObject final : public QDBusVirtualObject
{
public:
    QString introspect(const QString &path) const override
    {
        Q_UNUSED(path)
        return QStringLiteral(R"XML(
  <interface name="org.kde.KWin">
    <method name="showDesktop">
      <arg direction="in" type="b" name="show"/>
    </method>
  </interface>
)XML");
    }

    bool handleMessage(const QDBusMessage &message, const QDBusConnection &connection) override
    {
        if (message.interface() != QLatin1String("org.kde.KWin") || message.member() != QLatin1String("showDesktop")) {
            return false;
        }
        states << message.arguments().constFirst().toBool();
        connection.send(message.createReply());
        return true;
    }

    QList<bool> states;
};

class KlipperObject final : public QDBusVirtualObject
{
public:
    QString introspect(const QString &path) const override
    {
        Q_UNUSED(path)
        return QStringLiteral(R"XML(
  <interface name="org.kde.klipper.klipper">
    <method name="showKlipperPopupMenu"/>
  </interface>
)XML");
    }

    bool handleMessage(const QDBusMessage &message, const QDBusConnection &connection) override
    {
        if (message.interface() != QLatin1String("org.kde.klipper.klipper")
            || message.member() != QLatin1String("showKlipperPopupMenu")) {
            return false;
        }
        ++calls;
        connection.send(message.createReply());
        return true;
    }

    int calls = 0;
};

bool writeLauncher(const QString &path, const QString &marker)
{
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
        return false;
    }
    const QByteArray script = QByteArrayLiteral("#!/bin/sh\nprintf '%s\\n' '") + marker.toUtf8()
        + QByteArrayLiteral("' >> \"$GLOBAL_MENU_KDE_TEST_PROGRAM_LOG\"\n");
    if (file.write(script) != script.size()) {
        return false;
    }
    file.close();
    return file.setPermissions(QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ExeOwner | QFileDevice::ReadGroup
                               | QFileDevice::ExeGroup | QFileDevice::ReadOther | QFileDevice::ExeOther);
}
}

class DesktopFallbackTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void hasExpectedTopLevelMenus();
    void hasExpectedActionStructure();
    void urlActionsDispatch();
    void dbusActionsDispatch();
    void programActionsDispatch();
};

void DesktopFallbackTest::hasExpectedTopLevelMenus()
{
    DesktopFallback fallback;
    QMenu *menu = fallback.menu();
    QVERIFY(menu);

    const QList<QAction *> actions = menu->actions();
    QCOMPARE(actions.size(), 7);

    QStringList labels;
    for (QAction *action : actions) {
        labels << plainLabel(action->text());
    }

    QCOMPARE(labels,
             QStringList({QStringLiteral("File"),
                          QStringLiteral("Edit"),
                          QStringLiteral("View"),
                          QStringLiteral("Go"),
                          QStringLiteral("Tools"),
                          QStringLiteral("Settings"),
                          QStringLiteral("Help")}));
}

void DesktopFallbackTest::hasExpectedActionStructure()
{
    DesktopFallback fallback;
    QMenu *root = fallback.menu();

    QCOMPARE(actionLabels(menuByLabel(root, QStringLiteral("File"))),
             QStringList({QStringLiteral("Home Folder"), QStringLiteral("Documents"), QStringLiteral("Downloads"), QStringLiteral("Trash")}));
    QCOMPARE(actionLabels(menuByLabel(root, QStringLiteral("Edit"))), QStringList({QStringLiteral("Clipboard History")}));
    QCOMPARE(actionLabels(menuByLabel(root, QStringLiteral("View"))),
             QStringList({QStringLiteral("Show Desktop"), QStringLiteral("Restore Windows")}));
    QCOMPARE(actionLabels(menuByLabel(root, QStringLiteral("Go"))),
             QStringList({QStringLiteral("Home"), QStringLiteral("Documents"), QStringLiteral("Downloads"), QStringLiteral("Trash")}));
    QCOMPARE(actionLabels(menuByLabel(root, QStringLiteral("Tools"))),
             QStringList({QStringLiteral("Run Command…"), QStringLiteral("Konsole"), QStringLiteral("System Monitor")}));
    QCOMPARE(actionLabels(menuByLabel(root, QStringLiteral("Settings"))), QStringList({QStringLiteral("System Settings")}));
    QCOMPARE(actionLabels(menuByLabel(root, QStringLiteral("Help"))), QStringList({QStringLiteral("KDE Help Center")}));
}

void DesktopFallbackTest::urlActionsDispatch()
{
    UrlSink sink;
    QDesktopServices::setUrlHandler(QStringLiteral("file"), &sink, "capture");
    QDesktopServices::setUrlHandler(QStringLiteral("trash"), &sink, "capture");

    DesktopFallback fallback;
    QMenu *root = fallback.menu();
    QMenu *file = menuByLabel(root, QStringLiteral("File"));
    QMenu *go = menuByLabel(root, QStringLiteral("Go"));
    QVERIFY(file);
    QVERIFY(go);

    const QStringList fileActions = {QStringLiteral("Home Folder"), QStringLiteral("Documents"), QStringLiteral("Downloads"), QStringLiteral("Trash")};
    for (const QString &label : fileActions) {
        QAction *action = actionByLabel(file, label);
        QVERIFY2(action, qPrintable(label));
        QVERIFY2(action->isEnabled(), qPrintable(label));
        action->trigger();
    }

    const QStringList goActions = {QStringLiteral("Home"), QStringLiteral("Documents"), QStringLiteral("Downloads"), QStringLiteral("Trash")};
    for (const QString &label : goActions) {
        QAction *action = actionByLabel(go, label);
        QVERIFY2(action, qPrintable(label));
        QVERIFY2(action->isEnabled(), qPrintable(label));
        action->trigger();
    }

    QTRY_COMPARE_WITH_TIMEOUT(sink.urls.size(), 8, 3000);
    QCOMPARE(sink.urls.at(0), QUrl::fromLocalFile(QDir::homePath()));
    QCOMPARE(sink.urls.at(1), QUrl::fromLocalFile(QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)));
    QCOMPARE(sink.urls.at(2), QUrl::fromLocalFile(QStandardPaths::writableLocation(QStandardPaths::DownloadLocation)));
    QCOMPARE(sink.urls.at(3), QUrl(QStringLiteral("trash:/")));
    QCOMPARE(sink.urls.at(4), QUrl::fromLocalFile(QDir::homePath()));
    QCOMPARE(sink.urls.at(5), QUrl::fromLocalFile(QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)));
    QCOMPARE(sink.urls.at(6), QUrl::fromLocalFile(QStandardPaths::writableLocation(QStandardPaths::DownloadLocation)));
    QCOMPARE(sink.urls.at(7), QUrl(QStringLiteral("trash:/")));

    QDesktopServices::unsetUrlHandler(QStringLiteral("file"));
    QDesktopServices::unsetUrlHandler(QStringLiteral("trash"));
}

void DesktopFallbackTest::dbusActionsDispatch()
{
    QDBusConnection connection = QDBusConnection::sessionBus();
    QVERIFY(connection.isConnected());

    KWinObject kwin;
    KlipperObject klipper;
    QVERIFY(connection.registerService(QStringLiteral("org.kde.KWin")));
    QVERIFY(connection.registerVirtualObject(QStringLiteral("/KWin"), &kwin, QDBusConnection::SingleNode));
    QVERIFY(connection.registerService(QStringLiteral("org.kde.klipper")));
    QVERIFY(connection.registerVirtualObject(QStringLiteral("/klipper"), &klipper, QDBusConnection::SingleNode));

    DesktopFallback fallback;
    QMenu *root = fallback.menu();

    QAction *clipboard = actionByLabel(menuByLabel(root, QStringLiteral("Edit")), QStringLiteral("Clipboard History"));
    QAction *showDesktop = actionByLabel(menuByLabel(root, QStringLiteral("View")), QStringLiteral("Show Desktop"));
    QAction *restoreWindows = actionByLabel(menuByLabel(root, QStringLiteral("View")), QStringLiteral("Restore Windows"));
    QVERIFY(clipboard);
    QVERIFY(showDesktop);
    QVERIFY(restoreWindows);

    clipboard->trigger();
    showDesktop->trigger();
    restoreWindows->trigger();

    QTRY_COMPARE_WITH_TIMEOUT(klipper.calls, 1, 3000);
    QTRY_COMPARE_WITH_TIMEOUT(kwin.states.size(), 2, 3000);
    QCOMPARE(kwin.states.at(0), true);
    QCOMPARE(kwin.states.at(1), false);

    connection.unregisterObject(QStringLiteral("/klipper"), QDBusConnection::UnregisterNode);
    connection.unregisterService(QStringLiteral("org.kde.klipper"));
    connection.unregisterObject(QStringLiteral("/KWin"), QDBusConnection::UnregisterNode);
    connection.unregisterService(QStringLiteral("org.kde.KWin"));
}

void DesktopFallbackTest::programActionsDispatch()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    const QString logPath = dir.filePath(QStringLiteral("launch.log"));
    const QByteArray oldPath = qgetenv("PATH");
    const QByteArray oldLog = qgetenv("GLOBAL_MENU_KDE_TEST_PROGRAM_LOG");

    struct Launcher {
        const char *program;
        const char *marker;
    };
    const QList<Launcher> launchers = {
        {"krunner", "krunner"},
        {"konsole", "konsole"},
        {"plasma-systemmonitor", "plasma-systemmonitor"},
        {"systemsettings", "systemsettings"},
        {"khelpcenter", "khelpcenter"},
    };

    for (const Launcher &launcher : launchers) {
        QVERIFY(writeLauncher(dir.filePath(QString::fromLatin1(launcher.program)), QString::fromLatin1(launcher.marker)));
    }

    QByteArray testPath = dir.path().toUtf8();
    testPath.append(':');
    testPath.append(oldPath);
    qputenv("PATH", testPath);
    qputenv("GLOBAL_MENU_KDE_TEST_PROGRAM_LOG", logPath.toUtf8());

    DesktopFallback fallback;
    QMenu *root = fallback.menu();
    const QList<QPair<QString, QString>> actions = {
        {QStringLiteral("Tools"), QStringLiteral("Run Command…")},
        {QStringLiteral("Tools"), QStringLiteral("Konsole")},
        {QStringLiteral("Tools"), QStringLiteral("System Monitor")},
        {QStringLiteral("Settings"), QStringLiteral("System Settings")},
        {QStringLiteral("Help"), QStringLiteral("KDE Help Center")},
    };

    for (const auto &[menuLabel, actionLabel] : actions) {
        QAction *action = actionByLabel(menuByLabel(root, menuLabel), actionLabel);
        QVERIFY2(action, qPrintable(actionLabel));
        QVERIFY2(action->isEnabled(), qPrintable(actionLabel));
        action->trigger();
    }

    auto launched = [&]() {
        QFile log(logPath);
        if (!log.open(QIODevice::ReadOnly | QIODevice::Text)) {
            return QStringList();
        }
        QStringList lines = QString::fromUtf8(log.readAll()).split(QLatin1Char('\n'), Qt::SkipEmptyParts);
        lines.sort();
        return lines;
    };

    QStringList expected = {QStringLiteral("khelpcenter"),
                            QStringLiteral("konsole"),
                            QStringLiteral("krunner"),
                            QStringLiteral("plasma-systemmonitor"),
                            QStringLiteral("systemsettings")};
    expected.sort();
    QTRY_COMPARE_WITH_TIMEOUT(launched(), expected, 5000);

    if (oldPath.isNull()) {
        qunsetenv("PATH");
    } else {
        qputenv("PATH", oldPath);
    }
    if (oldLog.isNull()) {
        qunsetenv("GLOBAL_MENU_KDE_TEST_PROGRAM_LOG");
    } else {
        qputenv("GLOBAL_MENU_KDE_TEST_PROGRAM_LOG", oldLog);
    }
}

QTEST_MAIN(DesktopFallbackTest)
#include "desktopfallbacktest.moc"
