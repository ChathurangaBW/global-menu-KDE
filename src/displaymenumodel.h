// SPDX-FileCopyrightText: 2026 ChathurangaBW
// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <QAbstractListModel>
#include <QList>

class QAction;
class QMenu;
class GlobalMenuModel;

class DisplayMenuModel final : public QAbstractListModel
{
    Q_OBJECT

public:
    enum Role {
        LabelRole = Qt::UserRole + 1,
        ActionRole,
    };
    Q_ENUM(Role)

    explicit DisplayMenuModel(GlobalMenuModel *applicationModel, QObject *parent = nullptr);
    ~DisplayMenuModel() override;

    [[nodiscard]] int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    [[nodiscard]] QVariant data(const QModelIndex &index, int role) const override;
    [[nodiscard]] QHash<int, QByteArray> roleNames() const override;

    [[nodiscard]] QAction *actionForIndex(int index) const;
    [[nodiscard]] bool usingApplicationMenu() const;

    void aboutToShow(int index);
    void menuOpened(int index);
    void menuClosed(int index);

private:
    void rebuildView();
    void buildDesktopFallback();

    GlobalMenuModel *m_applicationModel = nullptr;
    QList<QAction *> m_fallbackActions;
    QList<QMenu *> m_fallbackMenus;
};
