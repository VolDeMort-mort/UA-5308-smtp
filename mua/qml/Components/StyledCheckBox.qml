// StyledCheckBox.qml
import QtQuick
import QtQuick.Controls
import SmtpMua

CheckBox {
    id: control
    text: "Status"

    contentItem: Text {
        text: control.text
        font.pixelSize: Theme.fontSizeMedium
        color: Theme.textColor
        leftPadding: control.indicator.width + control.spacing
        verticalAlignment: Text.AlignVCenter
    }

    indicator: Rectangle {
        implicitWidth: 18; implicitHeight: 18
        radius: 4
        anchors.verticalCenter: parent.verticalCenter
        color: control.checked ? Theme.iconSelectColor : "transparent"
        border.color: control.checked ? Theme.iconSelectColor : Theme.mutedTextColor
        border.width: 1.5

        Rectangle {
            width: 8; height: 8
            anchors.centerIn: parent
            color: "white"
            radius: 1
            visible: control.checked
        }
    }
}
