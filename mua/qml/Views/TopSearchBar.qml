import QtQuick
import QtQuick.Layouts
import "../Components"
import SmtpMua

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

            SvgIcon {
                pathData: Icons.search;
                color: mutedTextColor;
                size: 16
            }
            TextInput {
                Layout.fillWidth: true
                color: textColor
                font.pixelSize: Theme.fontSizeMedium
                text: "Search..."
                verticalAlignment: TextInput.AlignVCenter
            }
            SvgIcon {
                pathData: Icons.filter;
                color: mutedTextColor;
                size: 16
            }
        }
    }
}
