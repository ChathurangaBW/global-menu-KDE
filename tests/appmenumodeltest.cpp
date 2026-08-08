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
            if (parentId == 0 && !emptyRoot) {
                root.children.append(item(1, QStringLiteral("_File"), true));
                root.children.append(item(2, QStringLiteral("_About")));
            } else if (parentId == 1 && !emptyRoot) {
                root.children.append(item(10, QStringLiteral("_Open")));
            }
            connection.send(message.createReply({QVariant::fromValue(revision), QVariant::fromValue(root)}));
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

    bool sendLayoutUpdated(const QDBusConnection &connection)
    {
        ++revision;
        QDBusMessage signal = QDBusMessage::createSignal(QString::fromLatin1(menuPath),
                                                         QString::fromLatin1(menuInterface),
                                                         QStringLiteral("LayoutUpdated"));
        signal << revision << 0;
        return connection.send(signal);
    }

    bool emptyRoot = false;
    uint revision = 1;
    int getLayoutCalls = 0;
    QList<int> aboutToShowIds;
    QList<Event> events;
};

void isolateTaskSignals(AppMenuModel &model)
{
    auto *tasksModel = model.findChild<TaskManager::TasksModel *>();
    Q_ASSERT(tasksModel);
    QObject::disconnect(tasksModel, nullptr, &model, nullptr);
    tasksModel->blockSignals(true);
}

void verifyDesktopHeadings(AppMenuModel &model)
{
    const QStringList desktopHeadings = {
        QStringLiteral("File"),
        QStringLiteral("Edit"),
        QStringLiteral("View"),
        QStringLiteral("Go"),
        QStringLiteral("Tools"),
        QStringLiteral("Settings"),
        QStringLiteral("Help"),
    };
    QCOMPARE(model.rowCount(), desktopHeadings.size());
    for (int row = 0; row < desktopHeadings.size(); ++row) {
        QCOMPARE(plainLabel(model.data(model.index(row, 0), AppMenuModel::MenuRole).toString()), desktopHeadings.at(row));
    }
}
}

class AppMenuModelTest final : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void fallbackApplicationExporterLossFallback();
    void emptyApplicationMenuKeepsFallbackUntilUsable();
};

void AppMenuModelTest::fallbackApplicationExporterLossFallback()
{
    const QString connectionName = QStringLiteral("global-menu-kde-exporter-primary");
    const QString serviceName = QStringLiteral("org.example.GlobalMenuKde.Primary");
    QDBusConnection exporter = QDBusConnection::connectToBus(QDBusConnection::SessionBus, connectionName);
    QVERIFY(exporter.isConnected());
    QVERIFY(exporter.registerService(serviceName));

    FakeMenuObject fakeMenu;
    QVERIFY(exporter.registerVirtualObject(QString::fromLatin1(menuPath), &fakeMenu, QDBusConnection::SingleNode));

    AppMenuModel model;
    QVERIFY(model.menuAvailable());
    QVERIFY(model.usingDesktopFallback());
    verifyDesktopHeadings(model);

    // This is a model/importer integration test, not an active-window test.
    // Isolate LibTaskManager so the injected exporter is not replaced by the
    // deliberately empty active-window state in this headless fixture.
    isolateTaskSignals(model);

    model.updateApplicationMenu(serviceName, QString::fromLatin1(menuPath));

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

    // Exercise the real service-watcher path. Releasing the well-known name
    // produces the same owner-loss signal that the model sees when an
    // application exporter exits, without relying on QDBusConnection wrapper
    // lifetime/refcount details in the test process.
    QVERIFY(exporter.unregisterService(serviceName));
    QTRY_VERIFY_WITH_TIMEOUT(model.usingDesktopFallback(), 5000);
    QTRY_COMPARE_WITH_TIMEOUT(model.rowCount(), 7, 5000);
    verifyDesktopHeadings(model);

    QDBusConnection::disconnectFromBus(connectionName);
}

void AppMenuModelTest::emptyApplicationMenuKeepsFallbackUntilUsable()
{
    const QString connectionName = QStringLiteral("global-menu-kde-exporter-empty");
    const QString serviceName = QStringLiteral("org.example.GlobalMenuKde.Empty");
    QDBusConnection exporter = QDBusConnection::connectToBus(QDBusConnection::SessionBus, connectionName);
    QVERIFY(exporter.isConnected());
    QVERIFY(exporter.registerService(serviceName));

    FakeMenuObject fakeMenu;
    fakeMenu.emptyRoot = true;
    QVERIFY(exporter.registerVirtualObject(QString::fromLatin1(menuPath), &fakeMenu, QDBusConnection::SingleNode));

    AppMenuModel model;
    isolateTaskSignals(model);
    QVERIFY(model.usingDesktopFallback());

    model.updateApplicationMenu(serviceName, QString::fromLatin1(menuPath));
    QTRY_VERIFY_WITH_TIMEOUT(fakeMenu.getLayoutCalls > 0, 5000);
    QVERIFY(model.usingDesktopFallback());
    verifyDesktopHeadings(model);

    // An exporter can initially publish an empty layout and populate it later.
    // The fallback remains visible until a usable application menu arrives.
    fakeMenu.emptyRoot = false;
    QVERIFY(fakeMenu.sendLayoutUpdated(exporter));
    QTRY_VERIFY_WITH_TIMEOUT(!model.usingDesktopFallback(), 5000);
    QTRY_VERIFY_WITH_TIMEOUT(model.rowCount() >= 2, 5000);
    QCOMPARE(plainLabel(model.data(model.index(0, 0), AppMenuModel::MenuRole).toString()), QStringLiteral("File"));

    QVERIFY(exporter.unregisterService(serviceName));
    QTRY_VERIFY_WITH_TIMEOUT(model.usingDesktopFallback(), 5000);
    verifyDesktopHeadings(model);

    QDBusConnection::disconnectFromBus(connectionName);
}

QTEST_MAIN(AppMenuModelTest)
#include "appmenumodeltest.moc"
