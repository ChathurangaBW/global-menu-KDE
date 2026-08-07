// SPDX-FileCopyrightText: 2026 ChathurangaBW
// SPDX-License-Identifier: GPL-2.0-or-later

#include "appmenumodel.h"
#include "dbusmenutypes_p.h"

#include <QAction>
#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusVirtualObject>
#include <QMenu>
#include <QTest>
#include <QVariant>

namespace
{
constexpr auto menuPath = "/com/example/GlobalMenu";
constexpr auto menuInterface = "com.canonical.dbusmenu";

DBusMenuLayoutItem item(int id, const QString &label, bool submenu = false)
{
    DBusMenuLayoutItem result;
    result.id = id;
    result.properties.insert(QStringLiteral("label"), label);
    result.properties.insert(QStringLiteral("visible"), true);
    result.properties.insert(QStringLiteral("enabled"), true);
    if (submenu) {
        result.properties.insert(QStringLiteral("children-display"), QStringLiteral("submenu"));
    }
    return result;
}

QString plainLabel(QString text)
{
    text.remove(QLatin1Char('&'));
    return text;
}

class FakeMenuObject final : public QDBusVirtualObject
{
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
            const int parentId = message.arguments().at(0).toInt();
            DBusMenuLayoutItem root;
            root.id = parentId;
            if (parentId == 0) {
                root.children.append(item(1, QStringLiteral("_File"), true));
                root.children.append(item(2, QStringLiteral("_About")));
            } else if (parentId == 1) {
                root.children.append(item(10, QStringLiteral("_Open")));
            }
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

class AppMenuModelTest final : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void fallbackApplicationFallback();
};

void AppMenuModelTest::fallbackApplicationFallback()
{
    QDBusConnection connection = QDBusConnection::sessionBus();
    QVERIFY(connection.isConnected());

    FakeMenuObject fakeMenu;
    QVERIFY(connection.registerVirtualObject(QString::fromLatin1(menuPath), &fakeMenu, QDBusConnection::SingleNode));

    AppMenuModel model;
    QVERIFY(model.menuAvailable());
    QVERIFY(model.usingDesktopFallback());
    QCOMPARE(model.rowCount(), 7);

    const QStringList desktopHeadings = {
        QStringLiteral("File"),
        QStringLiteral("Edit"),
        QStringLiteral("View"),
        QStringLiteral("Go"),
        QStringLiteral("Tools"),
        QStringLiteral("Settings"),
        QStringLiteral("Help"),
    };
    for (int row = 0; row < desktopHeadings.size(); ++row) {
        QCOMPARE(plainLabel(model.data(model.index(row, 0), AppMenuModel::MenuRole).toString()), desktopHeadings.at(row));
    }

    // Use the connection's unique service name for a same-process fixture.
    // This mirrors Qt's proven self-hosted D-Bus test pattern and avoids
    // well-known-name loopback quirks that do not exist for real applications.
    model.updateApplicationMenu(connection.baseService(), QString::fromLatin1(menuPath));

    QTRY_VERIFY_WITH_TIMEOUT(fakeMenu.getLayoutCalls > 0, 5000);
    QTRY_VERIFY_WITH_TIMEOUT(!model.usingDesktopFallback(), 5000);
    QTRY_VERIFY_WITH_TIMEOUT(model.rowCount() >= 2, 5000);
    QCOMPARE(plainLabel(model.data(model.index(0, 0), AppMenuModel::MenuRole).toString()), QStringLiteral("File"));
    QCOMPARE(plainLabel(model.data(model.index(1, 0), AppMenuModel::MenuRole).toString()), QStringLiteral("About"));

    QAction *fileAction = model.data(model.index(0, 0), AppMenuModel::ActionRole).value<QAction *>();
    QVERIFY(fileAction);
    QVERIFY(fileAction->menu());
    QTRY_COMPARE_WITH_TIMEOUT(fileAction->menu()->actions().size(), 1, 5000);

    QAction *openAction = fileAction->menu()->actions().constFirst();
    QCOMPARE(openAction->text(), QStringLiteral("&Open"));
    openAction->trigger();
    QTRY_VERIFY_WITH_TIMEOUT(fakeMenu.hasEvent(10, QStringLiteral("clicked")), 5000);

    QAction *aboutAction = model.data(model.index(1, 0), AppMenuModel::ActionRole).value<QAction *>();
    QVERIFY(aboutAction);
    QVERIFY(!aboutAction->menu());
    aboutAction->trigger();
    QTRY_VERIFY_WITH_TIMEOUT(fakeMenu.hasEvent(2, QStringLiteral("clicked")), 5000);

    // Clearing the active application's exported menu must restore the desktop
    // fallback immediately; service disappearance takes this same model path.
    model.updateApplicationMenu(QString(), QString());
    QTRY_VERIFY_WITH_TIMEOUT(model.usingDesktopFallback(), 5000);
    QTRY_COMPARE_WITH_TIMEOUT(model.rowCount(), 7, 5000);

    connection.unregisterObject(QString::fromLatin1(menuPath), QDBusConnection::UnregisterNode);
}

QTEST_MAIN(AppMenuModelTest)
#include "appmenumodeltest.moc"
