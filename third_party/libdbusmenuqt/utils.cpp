/* This file is part of the dbusmenu-qt library
    SPDX-FileCopyrightText: 2010 Canonical
    SPDX-FileContributor: Aurelien Gateau <aurelien.gateau@canonical.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/
#include "utils_p.h"

#include <QString>

QString swapMnemonicChar(const QString &in, QChar src, QChar dst)
{
    QString out;
    bool mnemonicFound = false;

    for (int pos = 0; pos < in.length();) {
        const QChar ch = in[pos];
        if (ch == src) {
            if (pos == in.length() - 1) {
                ++pos;
            } else if (in[pos + 1] == src) {
                out += src;
                pos += 2;
            } else if (!mnemonicFound) {
                mnemonicFound = true;
                out += dst;
                ++pos;
            } else {
                ++pos;
            }
        } else if (ch == dst) {
            out += dst;
            out += dst;
            ++pos;
        } else {
            out += ch;
            ++pos;
        }
    }

    return out;
}
