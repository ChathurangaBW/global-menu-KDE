// SPDX-License-Identifier: GPL-2.0-or-later
#include "appmenuapplet.h"
#include "appmenumodel.h"

#include <KPluginMetaData>
#include <QApplication>
#include <QKeyEvent>
#include <QMenu>
#include <QQuickItem>
#include <QQuickWindow>
#include <QSignalSpy>
#include <QtTest>

class MenuModel final : public QAbstractListModel
{
public:
    QMenu root;
    int rowCount(const QModelIndex &parent = {}) const override
    {
        return parent.isValid() ? 0 : root.actions().size();
    }
    QVariant data(const QModelIndex &index, int role) const override
    {
        if (role != AppMenuModel::ActionRole) {
            return {};
        }
        if (!index.isValid()) {
            return QVariant::fromValue(root.menuAction());
        }
        return QVariant::fromValue(root.actions().value(index.row()));
    }
};

class TestApplet final : public AppMenuApplet
{
public:
    TestApplet() : AppMenuApplet(nullptr, KPluginMetaData::fromJsonFile(QStringLiteral(METADATA_FILE)), {}) {}
    using AppMenuApplet::eventFilter;
};

class AppMenuAppletTest final : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void representationsRestoreActions()
    {
        MenuModel model;
        auto *file = model.root.addMenu(QStringLiteral("File"));
        QAction *action = file->addAction(QStringLiteral("Open"));
        TestApplet applet;
        applet.setModel(&model);
        QQuickWindow window;
        QQuickItem context(window.contentItem());
        context.setSize(QSizeF(80, 30));
        for (int i = 0; i < 3; ++i) {
            applet.setView(AppMenuApplet::FullView);
            applet.trigger(&context, 0);
            QCOMPARE(applet.currentIndex(), 0);
            auto *popup = qobject_cast<QMenu *>(QApplication::activePopupWidget());
            QVERIFY(popup);
            QCOMPARE(popup->actions(), QList<QAction *>({action}));
            popup->hide();
            QCOMPARE(file->actions(), QList<QAction *>({action}));
            QCOMPARE(file->menuAction()->menu(), file);
            applet.setView(AppMenuApplet::CompactView);
            applet.trigger(&context, 0);
            QCOMPARE(file->actions(), QList<QAction *>({action}));
            model.root.hide();
        }
        applet.setView(AppMenuApplet::FullView);
        applet.trigger(&context, 0);
        // Changing representation with the popup still open must also restore it.
        applet.setView(AppMenuApplet::CompactView);
        QCOMPARE(file->actions(), QList<QAction *>({action}));
    }

    void navigationNeverExecutesCommands()
    {
        MenuModel model;
        QAction *command = model.root.addAction(QStringLiteral("Command"));
        QSignalSpy triggered(command, &QAction::triggered);
        TestApplet applet;
        applet.setModel(&model);
        QQuickWindow window;
        QQuickItem context(window.contentItem());
        applet.navigate(&context, 0);
        QCOMPARE(triggered.count(), 0);
        applet.trigger(&context, 0);
        QCOMPARE(triggered.count(), 1);
        command->setEnabled(false);
        applet.trigger(&context, 0);
        QCOMPARE(triggered.count(), 1);
        applet.setModel(nullptr);
        applet.trigger(&context, 0);
        applet.navigate(&context, -1);
    }

    void keyboardWrapAndRtl_data()
    {
        QTest::addColumn<bool>("rtl");
        QTest::newRow("ltr") << false;
        QTest::newRow("rtl") << true;
    }
    void keyboardWrapAndRtl()
    {
        QFETCH(bool, rtl);
        QGuiApplication::setLayoutDirection(rtl ? Qt::RightToLeft : Qt::LeftToRight);
        MenuModel model;
        model.root.addMenu(QStringLiteral("First"))->addAction(QStringLiteral("One"));
        model.root.addAction(QStringLiteral("Direct"));
        auto *disabled = model.root.addMenu(QStringLiteral("Disabled"));
        disabled->menuAction()->setEnabled(false);
        auto *hidden = model.root.addMenu(QStringLiteral("Hidden"));
        hidden->menuAction()->setVisible(false);
        model.root.addMenu(QStringLiteral("Last"))->addAction(QStringLiteral("Two"));
        TestApplet applet;
        applet.setModel(&model);
        QQuickWindow window;
        QQuickItem context(window.contentItem());
        applet.trigger(&context, 0);
        auto *popup = qobject_cast<QMenu *>(QApplication::activePopupWidget());
        QVERIFY(popup);
        QSignalSpy navigate(&applet, &AppMenuApplet::requestActivateIndex);
        QKeyEvent previous(QEvent::KeyPress, rtl ? Qt::Key_Right : Qt::Key_Left, Qt::NoModifier);
        QVERIFY(applet.eventFilter(popup, &previous));
        QCOMPARE(navigate.takeFirst().at(0).toInt(), 4);
        QKeyEvent next(QEvent::KeyPress, rtl ? Qt::Key_Left : Qt::Key_Right, Qt::NoModifier);
        QVERIFY(applet.eventFilter(popup, &next));
        QCOMPARE(navigate.takeFirst().at(0).toInt(), 4);
        popup->hide();
        QGuiApplication::setLayoutDirection(Qt::LeftToRight);
    }

    void sourceCanDisappearWhileOpen()
    {
        MenuModel model;
        auto *file = model.root.addMenu(QStringLiteral("File"));
        file->addAction(QStringLiteral("Open"));
        TestApplet applet;
        applet.setModel(&model);
        QQuickWindow window;
        QQuickItem context(window.contentItem());
        applet.trigger(&context, 0);
        delete file;
        applet.setView(AppMenuApplet::CompactView);
        QCOMPARE(applet.currentIndex(), -1);
    }
};
QTEST_MAIN(AppMenuAppletTest)
#include "appmenuapplettest.moc"
