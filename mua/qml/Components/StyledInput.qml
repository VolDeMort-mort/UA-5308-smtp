import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import SmtpMua

Item {
    id: root

    property string label: ""
    property string placeholder: ""
    property alias textValue: input.text
    property alias echoMode: input.echoMode

    implicitWidth: 200
    implicitHeight: 35

    RowLayout {
        anchors.fill: parent
        spacing: 10

        // Label
        Text {
            text: root.label
            color: Theme.mutedTextColor
            font.pixelSize: Theme.fontSizeMedium
            Layout.preferredWidth: 60
            visible: root.label !== ""
        }

        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true

            Rectangle {
                anchors.bottom: parent.bottom
                width: parent.width
                height: 1
                color: Theme.itemBorderColor
            }

            TextField {
                id: input
                anchors.fill: parent
                verticalAlignment: TextInput.AlignVCenter

                color: Theme.textColor
                font.pixelSize: Theme.fontSizeMedium

                placeholderText: root.placeholder
                placeholderTextColor: Theme.mutedTextColor

                background: Item {}
                selectByMouse: true
            }
        }
    }
}
