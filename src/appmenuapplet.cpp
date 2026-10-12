/*
    SPDX-FileCopyrightText: 2016 Kai Uwe Broulik <kde@privat.broulik.de>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#include "appmenuapplet.h"
#include "appmenumodel.h"

#include <QAction>
#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QKeyEvent>
#include <QMenu>
#include <QMouseEvent>
#include <QQuickItem>
#include <QQuickWindow>
#include <QScreen>
#include <QTimer>

int AppMenuApplet::s_refs = 0;

namespace
{
QString viewService()
{
    return QStringLiteral("org.kde.kappmenuview");
}
}

AppMenuApplet::AppMenuApplet(QObject *parent, const KPluginMetaData &data, const QVariantList &args)
    : Plasma::Applet(parent, data, args)
{
    ++s_refs;
    if (s_refs == 1) {
        QDBusConnection::sessionBus().interface()->registerService(viewService(),
                                                                   QDBusConnectionInterface::QueueService,
                                                                   QDBusConnectionInterface::DontAllowReplacement);
    }

    connect(this, &Applet::destroyedChanged, this, [](bool destroyed) {
        if (destroyed) {
            if (--s_refs == 0) {
                QDBusConnection::sessionBus().interface()->unregisterService(viewService());
            }
        } else {
            if (++s_refs == 1) {
                QDBusConnection::sessionBus().interface()->registerService(viewService(),
                                                                           QDBusConnectionInterface::QueueService,
                                                                           QDBusConnectionInterface::DontAllowReplacement);
            }
        }
    });
}

AppMenuApplet::~AppMenuApplet()
{
    if (m_currentMenu) {
        disconnect(m_currentMenu, nullptr, this, nullptr);
        m_currentMenu->hide();
    }
    restoreSourceMenu();
}

void AppMenuApplet::init()
{
}

QAbstractItemModel *AppMenuApplet::model() const
{
    return m_model;
}

void AppMenuApplet::setModel(QAbstractItemModel *model)
{
    if (m_model != model) {
        if (m_currentMenu) {
            m_currentMenu->hide();
        }
        restoreSourceMenu();
        m_model = model;
        Q_EMIT modelChanged();
    }
}

int AppMenuApplet::view() const
{
    return m_viewType;
}

void AppMenuApplet::setView(int type)
{
    if (m_viewType != type) {
        if (m_currentMenu) {
            m_currentMenu->hide();
            m_currentMenu->removeEventFilter(this);
            disconnect(m_currentMenu, &QMenu::aboutToHide, this, &AppMenuApplet::onMenuAboutToHide);
        }
        restoreSourceMenu();
        m_currentMenu = nullptr;
        m_sourceMenu = nullptr;
        m_popupMenu.reset();
        setCurrentIndex(-1);
        m_viewType = type;
        Q_EMIT viewChanged();
    }
}

int AppMenuApplet::currentIndex() const
{
    return m_currentIndex;
}

void AppMenuApplet::setCurrentIndex(int currentIndex)
{
    if (m_currentIndex != currentIndex) {
        m_currentIndex = currentIndex;
        Q_EMIT currentIndexChanged();
    }
}

QQuickItem *AppMenuApplet::buttonGrid() const
{
    return m_buttonGrid;
}

void AppMenuApplet::setButtonGrid(QQuickItem *buttonGrid)
{
    if (m_buttonGrid != buttonGrid) {
        m_buttonGrid = buttonGrid;
        Q_EMIT buttonGridChanged();
    }
}

QMenu *AppMenuApplet::createMenu(int idx) const
{
    QMenu *menu = nullptr;
    if (!m_model || idx < 0 || idx >= m_model->rowCount()) {
        return nullptr;
    }

    if (view() == CompactView) {
        if (auto *menuAction = m_model->data(QModelIndex(), AppMenuModel::ActionRole).value<QAction *>()) {
            menu = menuAction->menu();
        }
    } else if (view() == FullView) {
        const QModelIndex index = m_model->index(idx, 0);
        if (auto *action = m_model->data(index, AppMenuModel::ActionRole).value<QAction *>();
            action && action->isEnabled() && action->isVisible()) {
            menu = action->menu();
        }
    }

    return menu;
}

void AppMenuApplet::onMenuAboutToHide()
{
    restoreSourceMenu();
    setCurrentIndex(-1);
}

void AppMenuApplet::restoreSourceMenu()
{
    if (!m_popupMenu || m_currentMenu != m_popupMenu.get()) {
        return;
    }
    for (QAction *action : m_popupMenu->actions()) {
        m_popupMenu->removeAction(action);
        if (m_sourceMenu) {
            m_sourceMenu->addAction(action);
        }
    }
    if (m_sourceMenu) {
        m_sourceMenu->menuAction()->setMenu(m_sourceMenu);
    }
}

void AppMenuApplet::navigate(QQuickItem *ctx, int idx)
{
    // Hover/arrow navigation may open menus, never execute a direct command.
    if (createMenu(idx)) {
        trigger(ctx, idx);
    }
}

Qt::Edges edgeFromLocation(Plasma::Types::Location location)
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
        break;
    }
    return {};
}

void AppMenuApplet::trigger(QQuickItem *ctx, int idx)
{
    if (!m_model || idx < 0 || idx >= m_model->rowCount()) {
        return;
    }
    if (m_currentIndex == idx) {
        return;
    }

    if (!ctx || !ctx->window() || !ctx->window()->screen()) {
        return;
    }

    QMenu *actionMenu = createMenu(idx);
    if (actionMenu) {
        auto ungrabMouseHack = [ctx]() {
            if (ctx && ctx->window() && ctx->window()->mouseGrabberItem()) {
                ctx->window()->mouseGrabberItem()->ungrabMouse();
            }
        };

        if (view() == FullView) {
            if (!m_popupMenu) {
                m_popupMenu = std::make_unique<QMenu>();
                m_currentMenu = m_popupMenu.get();
                connect(m_currentMenu, &QMenu::aboutToHide, this, &AppMenuApplet::onMenuAboutToHide, Qt::UniqueConnection);
            }
            restoreSourceMenu();
            m_sourceMenu = actionMenu;
            auto menuAction = m_sourceMenu->menuAction();
            for (QAction *action : m_sourceMenu->actions()) {
                m_sourceMenu->removeAction(action);
                m_currentMenu->addAction(action);
            }
            menuAction->setMenu(m_currentMenu);
        } else {
            m_currentMenu = actionMenu;
            m_sourceMenu = actionMenu;
        }

        QTimer::singleShot(0, ctx, ungrabMouseHack);

        const auto &geo = ctx->window()->screen()->availableVirtualGeometry();
        QPoint pos = ctx->window()->mapToGlobal(ctx->mapToScene(QPointF()).toPoint());

        const Qt::Edges edges = edgeFromLocation(location());
        m_currentMenu->setProperty("_breeze_menu_seamless_edges", QVariant::fromValue(edges));

        if (location() == Plasma::Types::TopEdge) {
            pos.setY(pos.y() + ctx->height());
        }

        m_currentMenu->adjustSize();
        pos = QPoint(qBound(geo.x(), pos.x(), geo.x() + geo.width() - m_currentMenu->width()),
                     qBound(geo.y(), pos.y(), geo.y() + geo.height() - m_currentMenu->height()));

        if (view() == FullView) {
            if (m_currentMenu->isVisible()) {
                m_currentMenu->move(pos);
            } else {
                m_currentMenu->installEventFilter(this);
                m_currentMenu->winId();
                m_currentMenu->windowHandle()->setTransientParent(ctx->window());
                m_currentMenu->popup(pos);
            }
        } else if (view() == CompactView) {
            if (m_currentMenu->isEmpty()) {
                return;
            }
            m_currentMenu->popup(pos);
            connect(actionMenu, &QMenu::aboutToHide, this, &AppMenuApplet::onMenuAboutToHide, Qt::UniqueConnection);
        }

        setCurrentIndex(idx);
    } else {
        if (auto *action = m_model->index(idx, 0).data(AppMenuModel::ActionRole).value<QAction *>();
            action && action->isEnabled() && action->isVisible()) {
            Q_ASSERT(!action->menu());
            action->trigger();
        }
    }
}

bool AppMenuApplet::eventFilter(QObject *watched, QEvent *event)
{
    auto *menu = qobject_cast<QMenu *>(watched);
    if (!menu) {
        return false;
    }

    if (event->type() == QEvent::KeyPress) {
        auto *e = static_cast<QKeyEvent *>(event);

        if (e->key() == Qt::Key_Left || e->key() == Qt::Key_Right) {
            const bool rtl = QGuiApplication::layoutDirection() == Qt::RightToLeft;
            const int forwardKey = rtl ? Qt::Key_Left : Qt::Key_Right;
            if (e->key() == forwardKey && menu->activeAction() && menu->activeAction()->menu()) {
                return false;
            }
            const int count = m_model ? m_model->rowCount() : 0;
            const int step = e->key() == forwardKey ? 1 : -1;
            for (int distance = 1; distance <= count; ++distance) {
                const int desiredIndex = (m_currentIndex + step * distance + count) % count;
                if (createMenu(desiredIndex)) {
                    Q_EMIT requestActivateIndex(desiredIndex);
                    break;
                }
            }
            return true;
        }
    } else if (event->type() == QEvent::MouseMove) {
        auto *e = static_cast<QMouseEvent *>(event);

        if (!m_buttonGrid || !m_buttonGrid->window()) {
            return false;
        }

        const QPointF &windowLocalPos = m_buttonGrid->window()->mapFromGlobal(e->globalPosition());
        const QPointF &buttonGridLocalPos = m_buttonGrid->mapFromScene(windowLocalPos);
        auto *item = m_buttonGrid->childAt(buttonGridLocalPos.x(), buttonGridLocalPos.y());
        if (!item) {
            return false;
        }

        bool ok;
        const int buttonIndex = item->property("buttonIndex").toInt(&ok);
        if (!ok) {
            return false;
        }

        if (createMenu(buttonIndex)) {
            Q_EMIT requestActivateIndex(buttonIndex);
        }
    }

    return false;
}

K_PLUGIN_CLASS_WITH_JSON(AppMenuApplet, "metadata.json")

#include "appmenuapplet.moc"
#include "moc_appmenuapplet.cpp"
