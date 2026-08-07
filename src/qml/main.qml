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

    preferredRepresentation: fullRepresentation
    Plasmoid.status: PlasmaCore.Types.ActiveStatus

    // This is a menu applet for an existing Plasma panel, not a second panel
    // or standalone framed widget. The containment supplies the panel surface.
    Plasmoid.backgroundHints: PlasmaCore.Types.NoBackground

    implicitWidth: buttonGrid.implicitWidth
    implicitHeight: buttonGrid.implicitHeight
    Layout.minimumWidth: implicitWidth
    Layout.preferredWidth: implicitWidth
    Layout.maximumWidth: implicitWidth
    Layout.minimumHeight: implicitHeight
    Layout.preferredHeight: implicitHeight
    Layout.maximumHeight: implicitHeight
    Layout.fillWidth: false
    Layout.fillHeight: false

    fullRepresentation: GridLayout {
        id: buttonGrid

        LayoutMirroring.enabled: Application.layoutDirection === Qt.RightToLeft
        flow: root.vertical ? GridLayout.TopToBottom : GridLayout.LeftToRight
        rowSpacing: 0
        columnSpacing: 0

        implicitWidth: childrenRect.width
        implicitHeight: childrenRect.height
        Layout.minimumWidth: implicitWidth
        Layout.preferredWidth: implicitWidth
        Layout.maximumWidth: implicitWidth
        Layout.minimumHeight: implicitHeight
        Layout.preferredHeight: implicitHeight
        Layout.maximumHeight: implicitHeight
        Layout.fillWidth: false
        Layout.fillHeight: false

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

        Connections {
            target: Plasmoid
            function onActivated(): void {
                const button = menuRepeater.itemAt(0) as MenuDelegate;
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

                Layout.fillWidth: false
                Layout.fillHeight: root.vertical ? false : true
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
