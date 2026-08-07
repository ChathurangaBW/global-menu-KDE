// SPDX-FileCopyrightText: 2016 Kai Uwe Broulik <kde@privat.broulik.de>
// SPDX-FileCopyrightText: 2026 ChathurangaBW
// SPDX-License-Identifier: GPL-2.0-or-later
#include "globalmenuapplet.h"

#include "displaymenumodel.h"
#include "globalmenumodel.h"

#include <KPluginFactory>

#include <QAction>
#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDBusServiceWatcher>
#include <QEvent>
#include <QKeyEvent>
#include <QMenu>
#include <QMouseEvent>
#include <QQuickItem>
#include <QQuickWindow>
#include <QScreen>
#include <QTimer>
#include <QWindow>

namespace
{
QString viewService()
{
    return QStringLiteral("org.kde.kappmenuview");
}

void registerViewService()
{
    if (QDBusConnectionInterface *interface = QDBusConnection::sessionBus().interface()) {
        interface->registerService(
            viewService(),
            QDBusConnectionInterface::QueueService,
            QDBusConnectionInterface::DontAllowReplacement);
    }
}

Qt::Edges edgeForLocation(Plasma::Types::Location location)
{
    switch (location) {
    case Plasma::Types::TopEdge:
        return Qt::TopEdge;
    case Plasma::Types::BottomEdge:
        return Qt::BottomEdge;
    case Plasma::Types::LeftEdge:
        return Qt::LeftEdge;
    case Plasma::Types::RightEdge:
        return Qt::RightEdge;
    case Plasma::Types::Floating:
    case Plasma::Types::Desktop:
    case Plasma::Types::FullScreen:
        return {};
    }
    return {};
}
}

GlobalMenuApplet::GlobalMenuApplet(QObject *parent, const KPluginMetaData &data, const QVariantList &args)
    : Plasma::Applet(parent, data, args)
    , m_applicationModel(new GlobalMenuModel(this))
    , m_model(new DisplayMenuModel(m_applicationModel, this))
{
    registerViewService();

    // KDE's stock Global Menu applet and this applet can briefly coexist in
    // the same plasmashell process. If the stock applet unregisters the shared
    // presence name when it is removed, immediately reacquire it so KDED keeps
    // the application-menu registrar active for this applet.
    auto *viewServiceWatcher = new QDBusServiceWatcher(
        viewService(),
        QDBusConnection::sessionBus(),
        QDBusServiceWatcher::WatchForUnregistration,
        this);
    connect(viewServiceWatcher, &QDBusServiceWatcher::serviceUnregistered, this, [this](const QString &) {
        QTimer::singleShot(0, this, [] {
            registerViewService();
        });
    });

    connect(m_model, &QAbstractItemModel::modelAboutToBeReset, this, [this] {
        if (m_popupMenu) {
            m_popupMenu->hide();
        }
    });
}

GlobalMenuApplet::~GlobalMenuApplet()
{
    if (m_currentIndex >= 0 && m_model) {
        m_model->menuClosed(m_currentIndex);
        m_currentIndex = -1;
    }
    restoreSourceMenu();
    delete m_popupMenu;
}

QAbstractItemModel *GlobalMenuApplet::model() const
{
    return m_model;
}

int GlobalMenuApplet::currentIndex() const
{
    return m_currentIndex;
}

QQuickItem *GlobalMenuApplet::buttonGrid() const
{
    return m_buttonGrid;
}

void GlobalMenuApplet::setButtonGrid(QQuickItem *buttonGrid)
{
    if (m_buttonGrid == buttonGrid) {
        return;
    }
    m_buttonGrid = buttonGrid;
    Q_EMIT buttonGridChanged();
}

QAction *GlobalMenuApplet::sourceActionForIndex(int index) const
{
    return m_model ? m_model->actionForIndex(index) : nullptr;
}

QMenu *GlobalMenuApplet::sourceMenuForIndex(int index) const
{
    QAction *action = sourceActionForIndex(index);
    return action ? action->menu() : nullptr;
}

void GlobalMenuApplet::restoreSourceMenu()
{
    if (!m_popupMenu || !m_sourceMenu) {
        return;
    }

    const QList<QAction *> actions = m_popupMenu->actions();
    for (QAction *action : actions) {
        m_popupMenu->removeAction(action);
        m_sourceMenu->addAction(action);
    }
}

void GlobalMenuApplet::trigger(QQuickItem *contextItem, int index)
{
    if (m_currentIndex == index || !contextItem || !contextItem->window() || !contextItem->window()->screen()) {
        return;
    }

    QAction *requestedAction = sourceActionForIndex(index);
    if (!requestedAction || !requestedAction->isEnabled()) {
        return;
    }

    m_model->aboutToShow(index);
    QMenu *requestedSourceMenu = requestedAction->menu();

    // A dbusmenu top-level item is normally a submenu, but the protocol also
    // permits a direct action. Trigger those instead of silently ignoring them.
    if (!requestedSourceMenu) {
        if (m_popupMenu && m_popupMenu->isVisible()) {
            m_popupMenu->hide();
        }
        requestedAction->trigger();
        return;
    }

    if (requestedSourceMenu->isEmpty()) {
        return;
    }

    if (!m_popupMenu) {
        m_popupMenu = new QMenu;
        connect(m_popupMenu, &QMenu::aboutToHide, this, &GlobalMenuApplet::onMenuAboutToHide);
    }

    if (m_currentIndex >= 0) {
        m_model->menuClosed(m_currentIndex);
        setCurrentIndex(-1);
    }

    if (m_sourceMenu != requestedSourceMenu) {
        restoreSourceMenu();
        m_sourceMenu = requestedSourceMenu;
        const QList<QAction *> actions = m_sourceMenu->actions();
        for (QAction *action : actions) {
            m_sourceMenu->removeAction(action);
            m_popupMenu->addAction(action);
        }
    }

    QTimer::singleShot(0, contextItem, [contextItem] {
        if (contextItem && contextItem->window() && contextItem->window()->mouseGrabberItem()) {
            contextItem->window()->mouseGrabberItem()->ungrabMouse();
        }
    });

    const QRect availableGeometry = contextItem->window()->screen()->availableVirtualGeometry();
    QPoint position = contextItem->window()->mapToGlobal(contextItem->mapToScene(QPointF()).toPoint());

    if (location() == Plasma::Types::TopEdge) {
        position.setY(position.y() + qRound(contextItem->height()));
    } else if (location() == Plasma::Types::LeftEdge) {
        position.setX(position.x() + qRound(contextItem->width()));
    }

    m_popupMenu->setProperty("_breeze_menu_seamless_edges", QVariant::fromValue(edgeForLocation(location())));
    m_popupMenu->adjustSize();
    position.setX(qBound(
        availableGeometry.left(),
        position.x(),
        availableGeometry.right() - m_popupMenu->width() + 1));
    position.setY(qBound(
        availableGeometry.top(),
        position.y(),
        availableGeometry.bottom() - m_popupMenu->height() + 1));

    if (m_popupMenu->isVisible()) {
        m_popupMenu->move(position);
    } else {
        m_popupMenu->installEventFilter(this);
        m_popupMenu->winId();
        if (m_popupMenu->windowHandle()) {
            m_popupMenu->windowHandle()->setTransientParent(contextItem->window());
        }
        m_popupMenu->popup(position);
    }

    m_model->menuOpened(index);
    setCurrentIndex(index);
}

void GlobalMenuApplet::setCurrentIndex(int index)
{
    if (m_currentIndex == index) {
        return;
    }
    m_currentIndex = index;
    Q_EMIT currentIndexChanged();
}

void GlobalMenuApplet::onMenuAboutToHide()
{
    if (m_currentIndex >= 0 && m_model) {
        m_model->menuClosed(m_currentIndex);
    }
    restoreSourceMenu();
    m_sourceMenu = nullptr;
    setCurrentIndex(-1);
}

bool GlobalMenuApplet::eventFilter(QObject *watched, QEvent *event)
{
    auto *menu = qobject_cast<QMenu *>(watched);
    if (!menu || !m_model) {
        return false;
    }

    if (event->type() == QEvent::KeyPress) {
        auto *keyEvent = static_cast<QKeyEvent *>(event);
        const int count = m_model->rowCount();
        if (count == 0) {
            return false;
        }

        if (keyEvent->key() == Qt::Key_Left) {
            Q_EMIT requestActivateIndex((m_currentIndex - 1 + count) % count);
            return true;
        }
        if (keyEvent->key() == Qt::Key_Right) {
            if (menu->activeAction() && menu->activeAction()->menu()) {
                return false;
            }
            Q_EMIT requestActivateIndex((m_currentIndex + 1) % count);
            return true;
        }
    } else if (event->type() == QEvent::MouseMove) {
        auto *mouseEvent = static_cast<QMouseEvent *>(event);
        if (!m_buttonGrid || !m_buttonGrid->window()) {
            return false;
        }

        const QPointF windowPosition = m_buttonGrid->window()->mapFromGlobal(mouseEvent->globalPosition());
        const QPointF gridPosition = m_buttonGrid->mapFromScene(windowPosition);
        QQuickItem *item = m_buttonGrid->childAt(gridPosition.x(), gridPosition.y());
        while (item && item != m_buttonGrid) {
            bool ok = false;
            const int index = item->property("buttonIndex").toInt(&ok);
            if (ok) {
                Q_EMIT requestActivateIndex(index);
                break;
            }
            item = item->parentItem();
        }
    }

    return false;
}

K_PLUGIN_CLASS_WITH_JSON(GlobalMenuApplet, "metadata.json")

#include "globalmenuapplet.moc"
#include "moc_globalmenuapplet.cpp"
