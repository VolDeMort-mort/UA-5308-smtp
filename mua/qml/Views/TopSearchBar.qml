import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../Components"
import SmtpMua

RowLayout {
    id: topSearchBarRoot
    Layout.fillWidth: true
    Layout.alignment: Qt.AlignHCenter
    Layout.leftMargin: 30
    Layout.rightMargin: 30

    signal searchTextChanged(string query)

    Rectangle {
        Layout.fillWidth: true
        Layout.maximumWidth: 600
        Layout.alignment: Qt.AlignHCenter
        height: 40
        radius: 20
        color: Theme.panelColor
        border.color: Theme.itemBorderColor

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 15
            anchors.rightMargin: 15
            spacing: 12

            SvgIcon {
                pathData: Icons.search;
                color: Theme.mutedTextColor;
                size: 16
            }

            TextField {
                id: searchInput
                Layout.fillWidth: true
                color: Theme.textColor
                font.pixelSize: Theme.fontSizeMedium

                placeholderText: "Search..."
                placeholderTextColor: Theme.mutedTextColor

                background: Item {}
                verticalAlignment: TextInput.AlignVCenter

                onTextChanged: topSearchBarRoot.searchTextChanged(text)
            }

            SvgIcon {
                pathData: Icons.filter;
                color: Theme.mutedTextColor;
                size: 16

                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    onClicked: mainFilterPopup.open()
                }
            }
        }
    }
}
