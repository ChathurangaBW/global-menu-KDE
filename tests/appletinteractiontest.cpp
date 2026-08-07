// SPDX-FileCopyrightText: 2026 ChathurangaBW
// SPDX-License-Identifier: GPL-2.0-or-later

#include "displaymenumodel.h"
#include "globalmenuapplet.h"

#include <QAction>
#include <QDesktopServices>
#include <QFile>
#include <QFileInfo>
#include <QMenu>
#include <QQuickItem>
#include <QQuickWindow>
#include <QSignalSpy>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QTest>
#include <QUrl>

class UrlCapture final : public QObject
{
    Q_OBJECT

public Q_SLOTS:
    void capture(const QUrl &url)
    {
        urls.append(url);
    }

public:
    QList<QUrl> urls;
};

class GlobalMenuAppletTest final : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void fallbackSubmenuActionsReallyExecute();
};

void GlobalMenuAppletTest::fallbackSubmenuActionsReallyExecute()
{
    QTemporaryDir temporaryDirectory;
    QVERIFY(temporaryDirectory.isValid());

    const QString markerPath = temporaryDirectory.filePath(QStringLiteral("konsole-started"));
    const QString fakeKonsolePath = temporaryDirectory.filePath(QStringLiteral("konsole"));

    QFile fakeKonsole(fakeKonsolePath);
    QVERIFY(fakeKonsole.open(QIODevice::WriteOnly | QIODevice::Text));
    const QByteArray script = QByteArrayLiteral("#!/bin/sh\n: > ")
        + QByteArrayLiteral("\"")
        + markerPath.toUtf8()
        + QByteArrayLiteral("\"\n");
    QCOMPARE(fakeKonsole.write(script), script.size());
    fakeKonsole.close();
    QVERIFY(QFile::setPermissions(
        fakeKonsolePath,
        QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ExeOwner |
            QFileDevice::ReadGroup | QFileDevice::ExeGroup |
            QFileDevice::ReadOther | QFileDevice::ExeOther));

    const QByteArray oldPath = qgetenv("PATH");
    qputenv("PATH", temporaryDirectory.path().toUtf8() + ':' + oldPath);
    QCOMPARE(QStandardPaths::findExecutable(QStringLiteral("konsole")), fakeKonsolePath);

    UrlCapture urlCapture;
    QDesktopServices::setUrlHandler(QStringLiteral("file"), &urlCapture, "capture");

    {
        const KPluginMetaData metadata;
        GlobalMenuApplet applet(nullptr, metadata, {});

        QQuickWindow window;
        window.setGeometry(0, 0, 640, 480);
        window.show();
        QTest::qWait(50);
        QVERIFY(window.screen());

        QQuickItem contextItem(window.contentItem());
        contextItem.setWidth(64);
        contextItem.setHeight(24);

        QCOMPARE(applet.m_model->rowCount(), 7);

        QAction *fileAction = applet.m_model->actionForIndex(0);
        QVERIFY(fileAction);
        QVERIFY(fileAction->menu());
        QVERIFY(!fileAction->menu()->isEmpty());
        QAction *homeAction = fileAction->menu()->actions().constFirst();
        QVERIFY(homeAction);
        QCOMPARE(homeAction->text(), QStringLiteral("Home Folder"));
        QVERIFY(homeAction->isEnabled());
        QSignalSpy homeTriggered(homeAction, &QAction::triggered);

        QAction *toolsAction = applet.m_model->actionForIndex(4);
        QVERIFY(toolsAction);
        QVERIFY(toolsAction->menu());
        QVERIFY(toolsAction->menu()->actions().size() >= 2);
        QAction *konsoleAction = toolsAction->menu()->actions().at(1);
        QVERIFY(konsoleAction);
        QCOMPARE(konsoleAction->text(), QStringLiteral("Konsole"));
        QVERIFY(konsoleAction->isEnabled());
        QSignalSpy konsoleTriggered(konsoleAction, &QAction::triggered);

        // Open File through the real applet popup path and click Home Folder as
        // a user would. This specifically guards against changing the QMenu
        // action list from aboutToHide and swallowing QAction activation.
        applet.trigger(&contextItem, 0);
        QTRY_VERIFY(applet.m_popupMenu);
        QTRY_VERIFY(applet.m_popupMenu->isVisible());
        QVERIFY(applet.m_popupMenu->actions().contains(homeAction));
        const QRect homeGeometry = applet.m_popupMenu->actionGeometry(homeAction);
        QVERIFY(homeGeometry.isValid());
        QTest::mouseClick(applet.m_popupMenu, Qt::LeftButton, Qt::NoModifier, homeGeometry.center());

        QTRY_COMPARE(homeTriggered.count(), 1);
        QTRY_COMPARE(urlCapture.urls.size(), 1);
        QCOMPARE(urlCapture.urls.constFirst(), QUrl::fromLocalFile(QDir::homePath()));
        QTRY_VERIFY(!applet.m_popupMenu->isVisible());

        // Reopening the same top-level menu must still work even though its
        // actions remain parked in the reusable popup after the previous hide.
        applet.trigger(&contextItem, 0);
        QTRY_VERIFY(applet.m_popupMenu->isVisible());
        QVERIFY(applet.m_popupMenu->actions().contains(homeAction));
        applet.m_popupMenu->hide();
        QTRY_VERIFY(!applet.m_popupMenu->isVisible());

        // Switch menus and click Konsole. The fake executable gives us a real
        // process-launch assertion without starting an actual terminal in CI.
        applet.trigger(&contextItem, 4);
        QTRY_VERIFY(applet.m_popupMenu->isVisible());
        QVERIFY(applet.m_popupMenu->actions().contains(konsoleAction));
        const QRect konsoleGeometry = applet.m_popupMenu->actionGeometry(konsoleAction);
        QVERIFY(konsoleGeometry.isValid());
        QTest::mouseClick(applet.m_popupMenu, Qt::LeftButton, Qt::NoModifier, konsoleGeometry.center());

        QTRY_COMPARE(konsoleTriggered.count(), 1);
        QTRY_VERIFY_WITH_TIMEOUT(QFileInfo::exists(markerPath), 5000);
        QTRY_VERIFY(!applet.m_popupMenu->isVisible());
    }

    QDesktopServices::unsetUrlHandler(QStringLiteral("file"));
    qputenv("PATH", oldPath);
}

QTEST_MAIN(GlobalMenuAppletTest)

#include "appletinteractiontest.moc"
