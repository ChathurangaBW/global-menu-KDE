/* This file is part of the dbusmenu-qt library
    SPDX-FileCopyrightText: 2009 Canonical
    SPDX-FileContributor: Aurelien Gateau <aurelien.gateau@canonical.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/
#include "dbusmenushortcut_p.h"

#include <QKeySequence>
#include <utility>

using namespace Qt::StringLiterals;

static constexpr int QT_COLUMN = 0;
static constexpr int DM_COLUMN = 1;

static void processKeyTokens(QStringList *tokens, int srcCol, int dstCol)
{
    struct Row {
        const char16_t *zero;
        const char16_t *one;
        const char16_t *operator[](int col) const
        {
            return col == 0 ? zero : one;
        }
    };
    static constexpr Row table[] = {{u"Meta", u"Super"},
                                    {u"Ctrl", u"Control"},
                                    {u"+", u"plus"},
                                    {u"-", u"minus"},
                                    {nullptr, nullptr}};

    const Row *ptr = table;
    for (; ptr->zero != nullptr; ++ptr) {
        const QStringView from = (*ptr)[srcCol];
        const QStringView to = (*ptr)[dstCol];
        tokens->replaceInStrings(from, to);
    }
}

DBusMenuShortcut DBusMenuShortcut::fromKeySequence(const QKeySequence &sequence)
{
    QString string = sequence.toString();
    DBusMenuShortcut shortcut;
    QStringList tokens = string.split(u", "_s);
    for (QString &token : tokens) {
        token.replace(QLatin1String("++"), QLatin1String("+plus"));
        QStringList keyTokens = token.split(u'+');
        processKeyTokens(&keyTokens, QT_COLUMN, DM_COLUMN);
        shortcut << keyTokens;
    }
    return shortcut;
}

QKeySequence DBusMenuShortcut::toKeySequence() const
{
    QStringList tmp;
    for (const QStringList &keyTokens_ : std::as_const(*this)) {
        QStringList keyTokens = keyTokens_;
        processKeyTokens(&keyTokens, DM_COLUMN, QT_COLUMN);
        tmp << keyTokens.join(QLatin1String("+"));
    }
    QString string = tmp.join(QLatin1String(", "));
    return QKeySequence::fromString(string);
}
