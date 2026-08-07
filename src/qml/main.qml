// SPDX-FileCopyrightText: 2013 Heena Mahour <heena393@gmail.com>
// SPDX-FileCopyrightText: 2013 Sebastian Kügler <sebas@kde.org>
// SPDX-FileCopyrightText: 2016 Kai Uwe Broulik <kde@privat.broulik.de>
// SPDX-FileCopyrightText: 2026 ChathurangaBW
// SPDX-License-Identifier: GPL-2.0-or-later
pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts
import QtQml

import org.kde.plasma.plasmoid
import org.kde.plasma.core as PlasmaCore
import org.kde.plasma.private.keyboardindicator as KeyboardIndicator
import org.kde.kirigami as Kirigami

PlasmoidItem {
    id: root

    readonly property bool vertical: Plasmoid.formFactor === PlasmaCore.Types.Vertical
    readonly property bool hasApplicationMenu: Plasmoid.model.menuAvailable

    preferredRepresentation: fullRepresentation
    Plasmoid.constraintHints: Plasmoid.CanFillArea
    Plasmoid.status: root.hasApplicationMenu
        ? PlasmaCore.Types.ActiveStatus
        : PlasmaCore.Types.HiddenStatus

    // A non-exporting application must leave no placeholder or dead panel gap.
    implicitWidth: root.hasApplicationMenu ? buttonGrid.implicitWidth : 0
    implicitHeight: root.hasApplicationMenu ? buttonGrid.implicitHeight : 0
    Layout.minimumWidth: root.implicitWidth
    Layout.preferredWidth: root.implicitWidth
    Layout.maximumWidth: root.implicitWidth
    Layout.minimumHeight: root.implicitHeight
    Layout.preferredHeight: root.implicitHeight
    Layout.maximumHeight: root.implicitHeight

    fullRepresentation: GridLayout {
        id: buttonGrid

        visible: root.hasApplicationMenu
        LayoutMirroring.enabled: Application.layoutDirection === Qt.RightToLeft
        flow: root.vertical ? GridLayout.TopToBottom : GridLayout.LeftToRight
        rowSpacing: 0
        columnSpacing: 0

        Layout.minimumWidth: visible ? implicitWidth : 0
        Layout.maximumWidth: visible ? implicitWidth : 0
        Layout.minimumHeight: visible ? implicitHeight : 0
        Layout.maximumHeight: visible ? implicitHeight : 0

        Binding {
            target: Plasmoid
            property: "buttonGrid"
            value: buttonGrid
            restoreMode: Binding.RestoreNone
        }

        Connections {
            target: Plasmoid
            function onRequestActivateIndex(index: int): void {
                const button = menuRepeater.itemAt(index) as MenuDelegate;
                if (button) {
                    button.activated();
                }
            }
        }

        Repeater {
            id: menuRepeater
            model: Plasmoid.model

            MenuDelegate {
                required property int index
                required property string label
                required property PlasmaCore.Action activeAction

                readonly property int buttonIndex: index

                Layout.fillWidth: root.vertical
                Layout.fillHeight: !root.vertical
                vertical: root.vertical
                text: activeAction?.text ?? label
                visible: text.length > 0 && (activeAction?.visible ?? true)
                enabled: activeAction?.enabled ?? true
                down: Plasmoid.currentIndex === index
                menuIsOpen: Plasmoid.currentIndex !== -1
                Kirigami.MnemonicData.active: altState.pressed

                onActivated: Plasmoid.trigger(this, index)

                KeyboardIndicator.KeyState {
                    id: altState
                    key: Qt.Key_Alt
                }
            }
        }
    }
}
