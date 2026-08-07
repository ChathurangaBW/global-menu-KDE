// SPDX-FileCopyrightText: 2026 ChathurangaBW
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QDBusPendingCall>
#include <QObject>
#include <memory>

class QMenu;

class DesktopFallback final : public QObject
{
public:
    explicit DesktopFallback(QObject *parent = nullptr);
    ~DesktopFallback() override;

    QMenu *menu() const;

private:
    std::unique_ptr<QMenu> m_menu;
};
