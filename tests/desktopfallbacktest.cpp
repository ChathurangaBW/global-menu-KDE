// SPDX-FileCopyrightText: 2026 ChathurangaBW
// SPDX-License-Identifier: GPL-2.0-or-later

#include "desktopfallback.h"

#include <QAction>
#include <QFileInfo>
#include <QMenu>
#include <QSet>
#include <QTemporaryDir>
#include <QtTest>

namespace
{
QString label(QAction *action)
{
    QString text = action->text();
    text.remove(QLatin1Char('&'));
    return text;
}

QStringList labels(QMenu *menu)
{
    QStringList result;
    for (QAction *action : menu->actions()) {
        if (!action->isSeparator()) {
            result << label(action);
        }
    }
    return result;
}

QAction *findAction(QMenu *menu, const QString &wanted)
{
    for (QAction *action : menu->actions()) {
        if (!action->isSeparator() && label(action) == wanted) {
            return action;
        }
        if (action->menu()) {
            if (QAction *nested = findAction(action->menu(), wanted)) {
                return nested;
            }
        }
    }
    return nullptr;
}

QMenu *findMenu(QMenu *menu, const QString &wanted)
{
    QAction *action = findAction(menu, wanted);
    return action ? action->menu() : nullptr;
}

struct ProgramCall {
    QString program;
    QStringList arguments;
};

struct BusCall {
    QString service;
    QString path;
    QString interface;
    QString method;
    QVariantList arguments;
};

class FakeRunner final : public DesktopFallbackActionRunner
{
public:
    bool programAvailable(const QString &program) const override
    {
        return availablePrograms.contains(program);
    }

    void openUrl(const QUrl &url) override
    {
        openedUrls << url;
    }

    void startProgram(const QString &program, const QStringList &arguments) override
    {
        programCalls.push_back({program, arguments});
    }

    bool restartShellAvailable() const override
    {
        return restartAvailable;
    }

    void restartShell() override
    {
        ++restartCalls;
    }

    void callSessionBus(const QString &service,
                        const QString &path,
                        const QString &interface,
                        const QString &method,
                        const QVariantList &arguments) override
    {
        busCalls.push_back({service, path, interface, method, arguments});
    }

    bool confirm(QWidget *, const QString &title, const QString &text) override
    {
        confirmationTitles << title;
        confirmationTexts << text;
        return confirmationResult;
    }

    void createDesktopItem(DesktopCreateKind kind, const QString &desktopDirectory, QWidget *) override
    {
        createKinds << kind;
        createDirectories << desktopDirectory;
    }

    QSet<QString> availablePrograms;
    QList<QUrl> openedUrls;
    QList<ProgramCall> programCalls;
    QList<BusCall> busCalls;
    QStringList confirmationTitles;
    QStringList confirmationTexts;
    QList<DesktopCreateKind> createKinds;
    QStringList createDirectories;
    bool confirmationResult = true;
    bool restartAvailable = false;
    int restartCalls = 0;
};
}

class DesktopFallbackTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void menuContractMatchesIssue14();
    void fileAndGoMenusDoNotDuplicateLocations();
    void unavailableProgramsAreDisabled();
    void dispatchesProgramsUrlsAndSessionBusCalls();
    void restartRequiresConfirmation();
    void activeWindowActionsDispatchSafely();
    void createNewDispatchesSafeKinds();
    void validatesDesktopItemNames();
    void createsEmptyDesktopItemsSafely();
};

void DesktopFallbackTest::menuContractMatchesIssue14()
{
    FakeRunner runner;
    DesktopFallback fallback(&runner, QStringLiteral("/desktop"));
    QMenu *menu = fallback.menu();
    QVERIFY(menu);

    QCOMPARE(labels(menu),
             QStringList({QStringLiteral("File"),
                          QStringLiteral("Edit"),
                          QStringLiteral("View"),
                          QStringLiteral("Go"),
                          QStringLiteral("Tools"),
                          QStringLiteral("Settings"),
                          QStringLiteral("Help")}));

    QCOMPARE(labels(findMenu(menu, QStringLiteral("Create New"))),
             QStringList({QStringLiteral("Folder…"),
                          QStringLiteral("Text File…"),
                          QStringLiteral("HTML File…"),
                          QStringLiteral("Link to Location (URL)…"),
                          QStringLiteral("Basic Link to File or Directory…"),
                          QStringLiteral("Link to Application…")}));
    QCOMPARE(labels(findMenu(menu, QStringLiteral("File"))),
             QStringList({QStringLiteral("Create New"),
                          QStringLiteral("Restart Plasma Shell…"),
                          QStringLiteral("Close Window"),
                          QStringLiteral("Force Quit Window…"),
                          QStringLiteral("Lock Screen"),
                          QStringLiteral("Show Logout Screen…"),
                          QStringLiteral("Open Default Browser")}));
    QCOMPARE(labels(findMenu(menu, QStringLiteral("Edit"))),
             QStringList({QStringLiteral("Clipboard History"),
                          QStringLiteral("Desktop and Wallpaper…"),
                          QStringLiteral("Display Configuration…")}));
    QCOMPARE(labels(findMenu(menu, QStringLiteral("View"))),
             QStringList({QStringLiteral("Peek at Desktop"),
                          QStringLiteral("Restore Windows"),
                          QStringLiteral("Overview"),
                          QStringLiteral("Activities")}));
    QCOMPARE(labels(findMenu(menu, QStringLiteral("Go"))),
             QStringList({QStringLiteral("Home Folder"),
                          QStringLiteral("Documents"),
                          QStringLiteral("Downloads"),
                          QStringLiteral("Trash"),
                          QStringLiteral("Root Filesystem"),
                          QStringLiteral("Network"),
                          QStringLiteral("Recent Locations")}));
    QCOMPARE(labels(findMenu(menu, QStringLiteral("Tools"))),
             QStringList({QStringLiteral("Find Files…"),
                          QStringLiteral("Run Command…"),
                          QStringLiteral("Terminal"),
                          QStringLiteral("System Monitor"),
                          QStringLiteral("Manage Disk Usage"),
                          QStringLiteral("Partition Manager")}));
    QCOMPARE(labels(findMenu(menu, QStringLiteral("Settings"))),
             QStringList({QStringLiteral("System Settings"),
                          QStringLiteral("Power Management…"),
                          QStringLiteral("Date and Time…"),
                          QStringLiteral("Region and Language…"),
                          QStringLiteral("Bluetooth…")}));
    QCOMPARE(labels(findMenu(menu, QStringLiteral("Help"))),
             QStringList({QStringLiteral("KDE Help Center"),
                          QStringLiteral("KDE Global Menu Documentation"),
                          QStringLiteral("KDE Global Menu Issues"),
                          QStringLiteral("KDE Community")}));
}

void DesktopFallbackTest::fileAndGoMenusDoNotDuplicateLocations()
{
    FakeRunner runner;
    DesktopFallback fallback(&runner, QStringLiteral("/desktop"));
    const QStringList fileMenuLabels = labels(findMenu(fallback.menu(), QStringLiteral("File")));
    const QSet<QString> fileLabels(fileMenuLabels.cbegin(), fileMenuLabels.cend());

    const QStringList goLabels = labels(findMenu(fallback.menu(), QStringLiteral("Go")));
    for (const QString &goLabel : goLabels) {
        QVERIFY2(!fileLabels.contains(goLabel), qPrintable(goLabel));
    }
    QCOMPARE(goLabels.size(), 7);
}

void DesktopFallbackTest::unavailableProgramsAreDisabled()
{
    FakeRunner runner;
    runner.availablePrograms = {QStringLiteral("krunner"), QStringLiteral("konsole"), QStringLiteral("kcmshell6")};
    DesktopFallback fallback(&runner, QStringLiteral("/desktop"));

    QVERIFY(!findAction(fallback.menu(), QStringLiteral("Find Files…"))->isEnabled());
    QVERIFY(findAction(fallback.menu(), QStringLiteral("Run Command…"))->isEnabled());
    QVERIFY(findAction(fallback.menu(), QStringLiteral("Terminal"))->isEnabled());
    QVERIFY(!findAction(fallback.menu(), QStringLiteral("System Monitor"))->isEnabled());
    QVERIFY(!findAction(fallback.menu(), QStringLiteral("Manage Disk Usage"))->isEnabled());
    QVERIFY(!findAction(fallback.menu(), QStringLiteral("Partition Manager"))->isEnabled());
    QVERIFY(!findAction(fallback.menu(), QStringLiteral("System Settings"))->isEnabled());
    QVERIFY(findAction(fallback.menu(), QStringLiteral("Power Management…"))->isEnabled());
    QVERIFY(!findAction(fallback.menu(), QStringLiteral("Restart Plasma Shell…"))->isEnabled());
}

void DesktopFallbackTest::dispatchesProgramsUrlsAndSessionBusCalls()
{
    FakeRunner runner;
    runner.availablePrograms = {QStringLiteral("kcmshell6"),
                                QStringLiteral("krunner"),
                                QStringLiteral("konsole"),
                                QStringLiteral("plasma-systemmonitor"),
                                QStringLiteral("partitionmanager")};
    DesktopFallback fallback(&runner, QStringLiteral("/desktop"));

    findAction(fallback.menu(), QStringLiteral("Display Configuration…"))->trigger();
    findAction(fallback.menu(), QStringLiteral("Run Command…"))->trigger();
    QCOMPARE(runner.programCalls.size(), 2);
    QCOMPARE(runner.programCalls.at(0).program, QStringLiteral("kcmshell6"));
    QCOMPARE(runner.programCalls.at(0).arguments, QStringList({QStringLiteral("kcm_kscreen")}));
    QCOMPARE(runner.programCalls.at(1).program, QStringLiteral("krunner"));
    QVERIFY(runner.programCalls.at(1).arguments.isEmpty());

    findAction(fallback.menu(), QStringLiteral("KDE Global Menu Issues"))->trigger();
    findAction(fallback.menu(), QStringLiteral("Network"))->trigger();
    QCOMPARE(runner.openedUrls,
             QList<QUrl>({QUrl(QStringLiteral("https://github.com/ChathurangaBW/global-menu-KDE/issues")),
                          QUrl(QStringLiteral("network:/"))}));

    findAction(fallback.menu(), QStringLiteral("Lock Screen"))->trigger();
    findAction(fallback.menu(), QStringLiteral("Show Logout Screen…"))->trigger();
    findAction(fallback.menu(), QStringLiteral("Peek at Desktop"))->trigger();
    findAction(fallback.menu(), QStringLiteral("Overview"))->trigger();
    findAction(fallback.menu(), QStringLiteral("Activities"))->trigger();
    QCOMPARE(runner.busCalls.size(), 5);
    QCOMPARE(runner.busCalls.at(0).service, QStringLiteral("org.freedesktop.ScreenSaver"));
    QCOMPARE(runner.busCalls.at(0).method, QStringLiteral("Lock"));
    QCOMPARE(runner.busCalls.at(1).service, QStringLiteral("org.kde.LogoutPrompt"));
    QCOMPARE(runner.busCalls.at(1).method, QStringLiteral("promptAll"));
    QCOMPARE(runner.busCalls.at(2).method, QStringLiteral("showDesktop"));
    QCOMPARE(runner.busCalls.at(2).arguments, QVariantList({true}));
    QCOMPARE(runner.busCalls.at(3).path, QStringLiteral("/component/kwin"));
    QCOMPARE(runner.busCalls.at(3).arguments, QVariantList({QStringLiteral("Overview")}));
    QCOMPARE(runner.busCalls.at(4).path, QStringLiteral("/component/plasmashell"));
    QCOMPARE(runner.busCalls.at(4).arguments, QVariantList({QStringLiteral("manage activities")}));
}

void DesktopFallbackTest::restartRequiresConfirmation()
{
    FakeRunner runner;
    runner.restartAvailable = true;
    DesktopFallback fallback(&runner, QStringLiteral("/desktop"));

    runner.confirmationResult = false;
    findAction(fallback.menu(), QStringLiteral("Restart Plasma Shell…"))->trigger();
    QCOMPARE(runner.confirmationTitles.size(), 1);
    QVERIFY(runner.programCalls.isEmpty());

    runner.confirmationResult = true;
    findAction(fallback.menu(), QStringLiteral("Restart Plasma Shell…"))->trigger();
    QCOMPARE(runner.confirmationTitles.size(), 2);
    QCOMPARE(runner.restartCalls, 1);
}

void DesktopFallbackTest::activeWindowActionsDispatchSafely()
{
    FakeRunner runner;
    DesktopFallback fallback(&runner, QStringLiteral("/desktop"));
    bool closeRequested = false;
    bool forceQuitRequested = false;

    fallback.setActiveWindowActions(true,
                                    [&closeRequested] { closeRequested = true; },
                                    true,
                                    [&forceQuitRequested] { forceQuitRequested = true; });

    QAction *closeAction = findAction(fallback.menu(), QStringLiteral("Close Window"));
    QAction *forceQuitAction = findAction(fallback.menu(), QStringLiteral("Force Quit Window…"));
    QVERIFY(closeAction);
    QVERIFY(forceQuitAction);
    QVERIFY(closeAction->isEnabled());
    QVERIFY(forceQuitAction->isEnabled());

    closeAction->trigger();
    QVERIFY(closeRequested);

    runner.confirmationResult = false;
    forceQuitAction->trigger();
    QVERIFY(!forceQuitRequested);
    runner.confirmationResult = true;
    forceQuitAction->trigger();
    QVERIFY(forceQuitRequested);

    fallback.setActiveWindowActions(false, {}, false, {});
    QVERIFY(!closeAction->isEnabled());
    QVERIFY(!forceQuitAction->isEnabled());
}

void DesktopFallbackTest::createNewDispatchesSafeKinds()
{
    FakeRunner runner;
    DesktopFallback fallback(&runner, QStringLiteral("/chosen/desktop"));
    QMenu *createMenu = findMenu(fallback.menu(), QStringLiteral("Create New"));
    QVERIFY(createMenu);

    for (QAction *action : createMenu->actions()) {
        if (!action->isSeparator()) {
            action->trigger();
        }
    }
    QCOMPARE(runner.createKinds,
             QList<DesktopCreateKind>({DesktopCreateKind::Folder,
                                       DesktopCreateKind::TextFile,
                                       DesktopCreateKind::HtmlFile,
                                       DesktopCreateKind::UrlLink,
                                       DesktopCreateKind::FileOrDirectoryLink,
                                       DesktopCreateKind::ApplicationLink}));
    QCOMPARE(runner.createDirectories, QStringList(6, QStringLiteral("/chosen/desktop")));
}

void DesktopFallbackTest::validatesDesktopItemNames()
{
    QVERIFY(isSafeDesktopItemName(QStringLiteral("New Folder")));
    QVERIFY(isSafeDesktopItemName(QStringLiteral(" file.txt ")));
    QVERIFY(!isSafeDesktopItemName(QString()));
    QVERIFY(!isSafeDesktopItemName(QStringLiteral(".")));
    QVERIFY(!isSafeDesktopItemName(QStringLiteral("..")));
    QVERIFY(!isSafeDesktopItemName(QStringLiteral("../escape")));
    QVERIFY(!isSafeDesktopItemName(QStringLiteral("folder/file")));
    QVERIFY(!isSafeDesktopItemName(QStringLiteral("folder\\file")));
    QVERIFY(!isSafeDesktopItemName(QStringLiteral("line\nbreak")));
}

void DesktopFallbackTest::createsEmptyDesktopItemsSafely()
{
    QTemporaryDir temporaryDirectory;
    QVERIFY(temporaryDirectory.isValid());
    const QString desktop = temporaryDirectory.filePath(QStringLiteral("Desktop"));
    QString error;

    QVERIFY(createEmptyDesktopItem(desktop, QStringLiteral("Folder"), true, &error));
    QVERIFY(QFileInfo(temporaryDirectory.filePath(QStringLiteral("Desktop/Folder"))).isDir());
    QVERIFY(createEmptyDesktopItem(desktop, QStringLiteral("Note.txt"), false, &error));
    const QFileInfo note(temporaryDirectory.filePath(QStringLiteral("Desktop/Note.txt")));
    QVERIFY(note.isFile());
    QCOMPARE(note.size(), 0);

    QVERIFY(!createEmptyDesktopItem(desktop, QStringLiteral("Folder"), true, &error));
    QVERIFY(!error.isEmpty());
    QVERIFY(!createEmptyDesktopItem(desktop, QStringLiteral("../escape"), false, &error));
    QVERIFY(!QFileInfo::exists(temporaryDirectory.filePath(QStringLiteral("escape"))));
}

QTEST_MAIN(DesktopFallbackTest)
#include "desktopfallbacktest.moc"
