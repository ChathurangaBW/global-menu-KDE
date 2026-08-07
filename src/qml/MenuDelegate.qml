// SPDX-FileCopyrightText: 2020 Carson Black <uhhadd@gmail.com>
// SPDX-FileCopyrightText: 2026 ChathurangaBW
// SPDX-License-Identifier: GPL-2.0-or-later

import QtQuick
import QtQuick.Controls

import org.kde.ksvg as KSvg
import org.kde.plasma.components as PlasmaComponents
import org.kde.kirigami as Kirigami

AbstractButton {
    id: control

    property bool menuIsOpen: false
    property bool vertical: false
    signal activated()

    hoverEnabled: true
    onHoveredChanged: if (hovered && menuIsOpen) {
        activated();
    }
    onPressed: activated()

    enum VisualState {
        Rest,
        Hover,
        Down
    }

    readonly property int visualState: {
        if (down) {
            return MenuDelegate.VisualState.Down;
        }
        if (hovered && !menuIsOpen) {
            return MenuDelegate.VisualState.Hover;
        }
        return MenuDelegate.VisualState.Rest;
    }

    Kirigami.MnemonicData.controlType: Kirigami.MnemonicData.SecondaryControl
    Kirigami.MnemonicData.label: text

    // Keep the strip visually close to a desktop menubar: compact vertically,
    // but with enough horizontal separation for File / Edit / View / … labels.
    topPadding: Math.max(frame.margins.top, Kirigami.Units.smallSpacing / 2)
    bottomPadding: Math.max(frame.margins.bottom, Kirigami.Units.smallSpacing / 2)
    leftPadding: Math.max(
        frame.margins.left,
        control.vertical ? Kirigami.Units.smallSpacing : Kirigami.Units.smallSpacing * 2)
    rightPadding: Math.max(
        frame.margins.right,
        control.vertical ? Kirigami.Units.smallSpacing : Kirigami.Units.smallSpacing * 2)

    Accessible.description: qsTr("Open application menu")

    background: KSvg.FrameSvgItem {
        id: frame
        imagePath: "widgets/menubaritem"
        prefix: switch (control.visualState) {
        case MenuDelegate.VisualState.Down:
            return "pressed";
        case MenuDelegate.VisualState.Hover:
            return "hover";
        default:
            return "normal";
        }
    }

    contentItem: PlasmaComponents.Label {
        text: control.Kirigami.MnemonicData.richTextLabel
        textFormat: Text.StyledText
        verticalAlignment: Text.AlignVCenter
        horizontalAlignment: Text.AlignHCenter
        elide: Text.ElideRight
        color: control.visualState === MenuDelegate.VisualState.Rest
            ? Kirigami.Theme.textColor
            : Kirigami.Theme.highlightedTextColor
    }
}
