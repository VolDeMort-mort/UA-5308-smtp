import QtQuick
import QtQuick.Layouts
import "../Components"

RowLayout {
    Layout.fillWidth: true
    Layout.alignment: Qt.AlignHCenter
    Layout.leftMargin: 30
    Layout.rightMargin: 30

    Rectangle {
        Layout.fillWidth: true
        Layout.maximumWidth: 600
        Layout.alignment: Qt.AlignHCenter
        height: 40
        radius: 20
        color: panelColor
        border.color: itemBorderColor

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 15
            anchors.rightMargin: 15
            spacing: 12

            SvgIcon { pathData: "M11 19a8 8 0 1 0 0-16 8 8 0 0 0 0 16z M21 21l-4.35-4.35"; color: mutedTextColor; size: 16 }
            TextInput {
                Layout.fillWidth: true
                color: textColor
                font.pixelSize: 14
                text: "Search..."
                verticalAlignment: TextInput.AlignVCenter
            }
            SvgIcon { pathData: "M22 3H2l8 9.46V19l4 2v-8.54L22 3z"; color: mutedTextColor; size: 16 }
        }
    }
}
