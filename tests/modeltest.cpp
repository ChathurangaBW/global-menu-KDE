// SPDX-FileCopyrightText: 2026 ChathurangaBW
// SPDX-License-Identifier: GPL-2.0-or-later

#include "globalmenumodel.h"

#include <tasksmodel.h>

#include <QAction>
#include <QCoreApplication>
#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusVirtualObject>
#include <QEventLoop>
#include <QMenu>
#include <QObject>
#include <QTest>
#include <QVariant>

namespace
{
constexpr auto menuPath = "/com/example/TestMenu";
constexpr auto menuInterface = "com.canonical.dbusmenu";

DBusMenuLayoutItem menuItem(int id, const QString &label, QList<DBusMenuLayoutItem> children = {})
{
    DBusMenuLayoutItem item;
    item.id = id;
    item.properties.insert(QStringLiteral("label"), label);
    item.properties.insert(QStringLiteral("visible"), true);
    item.properties.insert(QStringLiteral("enabled"), true);
    item.children = std::move(children);
    if (!item.children.isEmpty()) {
        item.properties.insert(QStringLiteral("children-display"), QStringLiteral("submenu"));
    }
    return item;
}

class FakeMenuObject final : public QDBusVirtualObject
{
    Q_OBJECT

public:
    struct Event {
        int id = -1;
        QString name;
    };

    QString introspect(const QString &path) const override
    {
        Q_UNUSED(path)
        return QStringLiteral(R"XML(
  <interface name="com.canonical.dbusmenu">
    <method name="GetLayout">
      <arg direction="in" type="i" name="parentId"/>
      <arg direction="in" type="i" name="recursionDepth"/>
      <arg direction="in" type="as" name="propertyNames"/>
      <arg direction="out" type="u" name="revision"/>
      <arg direction="out" type="(ia{sv}av)" name="layout"/>
    </method>
    <method name="AboutToShow">
      <arg direction="in" type="i" name="id"/>
      <arg direction="out" type="b" name="needUpdate"/>
    </method>
    <method name="Event">
      <arg direction="in" type="i" name="id"/>
      <arg direction="in" type="s" name="eventId"/>
      <arg direction="in" type="v" name="data"/>
      <arg direction="in" type="u" name="timestamp"/>
    </method>
    <signal name="LayoutUpdated">
      <arg type="u" name="revision"/>
      <arg type="i" name="parentId"/>
    </signal>
    <signal name="ItemsPropertiesUpdated">
      <arg type="a(ia{sv})" name="updatedProps"/>
      <arg type="a(ias)" name="removedProps"/>
    </signal>
  </interface>
)XML");
    }

    bool handleMessage(const QDBusMessage &message, const QDBusConnection &connection) override
    {
        if (message.interface() != QLatin1String(menuInterface)) {
            return false;
        }

        if (message.member() == QLatin1String("GetLayout")) {
            ++getLayoutCalls;

            DBusMenuLayoutItem root;
            root.id = 0;

            DBusMenuLayoutItem open = menuItem(10, QStringLiteral("_Open"));
            DBusMenuLayoutItem disabled = menuItem(11, QStringLiteral("_Disabled"));
            disabled.properties.insert(QStringLiteral("enabled"), false);
            root.children.append(menuItem(1, QStringLiteral("_File"), {open, disabled}));
            root.children.append(menuItem(2, QStringLiteral("_About")));

            connection.send(message.createReply({QVariant::fromValue(1u), QVariant::fromValue(root)}));
            return true;
        }

        if (message.member() == QLatin1String("AboutToShow")) {
            aboutToShowIds.append(message.arguments().at(0).toInt());
            connection.send(message.createReply(QVariant(false)));
            return true;
        }

        if (message.member() == QLatin1String("Event")) {
            events.append({message.arguments().at(0).toInt(), message.arguments().at(1).toString()});
            connection.send(message.createReply());
            return true;
        }

        return false;
    }

    bool hasEvent(int id, const QString &name) const
    {
        for (const Event &event : events) {
            if (event.id == id && event.name == name) {
                return true;
            }
        }
        return false;
    }

    int getLayoutCalls = 0;
    QList<int> aboutToShowIds;
    QList<Event> events;
};
}

class GlobalMenuModelTest final : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void importsLayoutAndForwardsEvents();
};

void GlobalMenuModelTest::importsLayoutAndForwardsEvents()
{
    QDBusConnection connection = QDBusConnection::sessionBus();
    QVERIFY(connection.isConnected());

    FakeMenuObject fakeMenu;
    QVERIFY(connection.registerVirtualObject(
        QString::fromLatin1(menuPath),
        &fakeMenu,
        QDBusConnection::SingleNode));

    GlobalMenuModel model;
    QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
    QObject::disconnect(model.m_tasksModel, nullptr, &model, nullptr);
    model.m_tasksModel->blockSignals(true);
    model.setSource(connection.baseService(), QString::fromLatin1(menuPath));

    QTRY_VERIFY_WITH_TIMEOUT(fakeMenu.getLayoutCalls > 0, 5000);
    QTRY_COMPARE_WITH_TIMEOUT(model.rowCount(), 2, 5000);
    QVERIFY(model.menuAvailable());
    QCOMPARE(model.data(model.index(0, 0), GlobalMenuModel::LabelRole).toString(), QStringLiteral("File"));
    QCOMPARE(model.data(model.index(1, 0), GlobalMenuModel::LabelRole).toString(), QStringLiteral("About"));

    QAction *fileAction = model.data(model.index(0, 0), GlobalMenuModel::ActionRole).value<QAction *>();
    QVERIFY(fileAction);
    QVERIFY(fileAction->menu());
    QCOMPARE(fileAction->menu()->actions().size(), 2);
    QCOMPARE(fileAction->menu()->actions().at(0)->text(), QStringLiteral("&Open"));
    QVERIFY(!fileAction->menu()->actions().at(1)->isEnabled());

    QAction *aboutAction = model.data(model.index(1, 0), GlobalMenuModel::ActionRole).value<QAction *>();
    QVERIFY(aboutAction);
    QVERIFY(!aboutAction->menu());

    model.aboutToShow(0);
    model.menuOpened(0);
    fileAction->menu()->actions().at(0)->trigger();
    model.menuClosed(0);
    aboutAction->trigger();

    QTRY_VERIFY_WITH_TIMEOUT(fakeMenu.aboutToShowIds.contains(1), 5000);
    QTRY_VERIFY_WITH_TIMEOUT(fakeMenu.hasEvent(1, QStringLiteral("opened")), 5000);
    QTRY_VERIFY_WITH_TIMEOUT(fakeMenu.hasEvent(10, QStringLiteral("clicked")), 5000);
    QTRY_VERIFY_WITH_TIMEOUT(fakeMenu.hasEvent(1, QStringLiteral("closed")), 5000);
    QTRY_VERIFY_WITH_TIMEOUT(fakeMenu.hasEvent(2, QStringLiteral("clicked")), 5000);

    connection.unregisterObject(QString::fromLatin1(menuPath), QDBusConnection::UnregisterNode);
}

QTEST_MAIN(GlobalMenuModelTest)

#include "modeltest.moc"
