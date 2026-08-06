// SPDX-FileCopyrightText: 2026 ChathurangaBW
// SPDX-License-Identifier: GPL-2.0-or-later

#include "dbusmenutypes.h"

#include <QKeySequence>
#include <QTest>

class ShortcutTest final : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void translatesControl();
    void translatesSuper();
    void translatesPlusAndMinus();
    void preservesMultiChordSequences();
};

void ShortcutTest::translatesControl()
{
    DBusMenuShortcut shortcut;
    shortcut.append({QStringLiteral("Control"), QStringLiteral("Q")});

    QCOMPARE(
        shortcut.toKeySequence(),
        QKeySequence::fromString(QStringLiteral("Ctrl+Q"), QKeySequence::PortableText));
}

void ShortcutTest::translatesSuper()
{
    DBusMenuShortcut shortcut;
    shortcut.append({QStringLiteral("Super"), QStringLiteral("Space")});

    QCOMPARE(
        shortcut.toKeySequence(),
        QKeySequence::fromString(QStringLiteral("Meta+Space"), QKeySequence::PortableText));
}

void ShortcutTest::translatesPlusAndMinus()
{
    DBusMenuShortcut plusShortcut;
    plusShortcut.append({QStringLiteral("Control"), QStringLiteral("plus")});
    QCOMPARE(
        plusShortcut.toKeySequence(),
        QKeySequence::fromString(QStringLiteral("Ctrl++"), QKeySequence::PortableText));

    DBusMenuShortcut minusShortcut;
    minusShortcut.append({QStringLiteral("Control"), QStringLiteral("minus")});
    QCOMPARE(
        minusShortcut.toKeySequence(),
        QKeySequence::fromString(QStringLiteral("Ctrl+-"), QKeySequence::PortableText));
}

void ShortcutTest::preservesMultiChordSequences()
{
    DBusMenuShortcut shortcut;
    shortcut.append({QStringLiteral("Control"), QStringLiteral("K")});
    shortcut.append({QStringLiteral("Control"), QStringLiteral("C")});

    QCOMPARE(
        shortcut.toKeySequence(),
        QKeySequence::fromString(QStringLiteral("Ctrl+K, Ctrl+C"), QKeySequence::PortableText));
}

QTEST_GUILESS_MAIN(ShortcutTest)

#include "shortcuttest.moc"
