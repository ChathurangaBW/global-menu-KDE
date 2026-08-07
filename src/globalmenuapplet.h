// SPDX-FileCopyrightText: 2016 Kai Uwe Broulik <kde@privat.broulik.de>
// SPDX-FileCopyrightText: 2026 ChathurangaBW
// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include "viewservicelease.h"

#include <Plasma/Applet>

#include <QAbstractItemModel>
#include <QPointer>

class GlobalMenuModel;
class QAction;
class QMenu;
class QQuickItem;

class GlobalMenuApplet final : public Plasma::Applet
{
    Q_OBJECT
    Q_PROPERTY(QAbstractItemModel *model READ model CONSTANT)
    Q_PROPERTY(int currentIndex READ currentIndex NOTIFY currentIndexChanged)
    Q_PROPERTY(QQuickItem *buttonGrid READ buttonGrid WRITE setButtonGrid NOTIFY buttonGridChanged)

public:
    explicit GlobalMenuApplet(QObject *parent, const KPluginMetaData &data, const QVariantList &args);
    ~GlobalMenuApplet() override;

    [[nodiscard]] QAbstractItemModel *model() const;
    [[nodiscard]] int currentIndex() const;
    [[nodiscard]] QQuickItem *buttonGrid() const;
    void setButtonGrid(QQuickItem *buttonGrid);

    Q_INVOKABLE void trigger(QQuickItem *contextItem, int index);

Q_SIGNALS:
    void currentIndexChanged();
    void buttonGridChanged();
    void requestActivateIndex(int index);

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    QAction *sourceActionForIndex(int index) const;
    QMenu *sourceMenuForIndex(int index) const;
    void restoreSourceMenu();
    void setCurrentIndex(int index);
    void onMenuAboutToHide();

    ViewServiceLease m_viewServiceLease{this};
    GlobalMenuModel *m_model = nullptr;
    QPointer<QMenu> m_popupMenu;
    QPointer<QMenu> m_sourceMenu;
    QPointer<QQuickItem> m_buttonGrid;
    int m_currentIndex = -1;
};
