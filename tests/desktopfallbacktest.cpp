// SPDX-FileCopyrightText: 2026 ChathurangaBW
// SPDX-License-Identifier: GPL-2.0-or-later

#include "desktopfallback.h"

#include <QAction>
#include <QMenu>
#include <QSet>
#include <QtTest>

class DesktopFallbackTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void hasExpectedTopLevelMenus();
    void everyTopLevelEntryHasActions();
    void fileAndGoMenusDoNotDuplicateLocations();
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
        QString text = action->text();
        text.remove(QLatin1Char('&'));
        labels << text;
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

void DesktopFallbackTest::everyTopLevelEntryHasActions()
{
    DesktopFallback fallback;
    for (QAction *action : fallback.menu()->actions()) {
        QVERIFY2(action->menu(), qPrintable(action->text()));
        QVERIFY2(!action->menu()->actions().isEmpty(), qPrintable(action->text()));
    }
}

void DesktopFallbackTest::fileAndGoMenusDoNotDuplicateLocations()
{
    DesktopFallback fallback;
    QMenu *menu = fallback.menu();
    QVERIFY(menu);

    QMenu *fileMenu = menu->actions().at(0)->menu();
    QMenu *goMenu = menu->actions().at(3)->menu();
    QVERIFY(fileMenu);
    QVERIFY(goMenu);

    QSet<QString> fileLabels;
    for (QAction *action : fileMenu->actions()) {
        if (!action->isSeparator()) {
            QString text = action->text();
            text.remove(QLatin1Char('&'));
            fileLabels.insert(text);
        }
    }

    QStringList goLabels;
    for (QAction *action : goMenu->actions()) {
        if (!action->isSeparator()) {
            QString text = action->text();
            text.remove(QLatin1Char('&'));
            goLabels << text;
            QVERIFY2(!fileLabels.contains(text), qPrintable(text));
        }
    }

    QCOMPARE(goLabels,
             QStringList({QStringLiteral("Root Filesystem"),
                          QStringLiteral("Network"),
                          QStringLiteral("Recent Locations")}));
}

QTEST_MAIN(DesktopFallbackTest)
#include "desktopfallbacktest.moc"
