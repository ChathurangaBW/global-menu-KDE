// SPDX-FileCopyrightText: 2026 ChathurangaBW
// SPDX-License-Identifier: GPL-2.0-or-later

#include "desktopfallback.h"

#include <QAction>
#include <QMenu>
#include <QtTest>

class DesktopFallbackTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void hasExpectedTopLevelMenus();
    void everyTopLevelEntryHasActions();
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

QTEST_MAIN(DesktopFallbackTest)
#include "desktopfallbacktest.moc"
