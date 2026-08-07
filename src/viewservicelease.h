// SPDX-FileCopyrightText: 2026 ChathurangaBW
// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <QMetaObject>

namespace Plasma
{
class Applet;
}

class ViewServiceLease final
{
public:
    explicit ViewServiceLease(Plasma::Applet *applet);
    ~ViewServiceLease();

    ViewServiceLease(const ViewServiceLease &) = delete;
    ViewServiceLease &operator=(const ViewServiceLease &) = delete;

private:
    void setActive(bool active);

    Plasma::Applet *m_applet = nullptr;
    QMetaObject::Connection m_destroyedConnection;
    bool m_active = false;
};
