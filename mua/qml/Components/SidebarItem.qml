import QtQuick
import QtQuick.Layouts
import SmtpMua


Item {
    id: itemRoot
    property string iconPath: ""
    property string label: ""
    property bool isActive: false
    property color bgColor: isActive ? Theme.panelColor : (mArea.containsMouse ? Qt.darker(Theme.panelColor, 1.2) : "transparent")
    property bool boldText: isActive
    property bool isCollapsed: false
    signal clicked()


    property int iconTextSpacing: 20

    Layout.fillWidth: true
    Layout.leftMargin: isCollapsed ? 0 : 10
    Layout.rightMargin: isCollapsed ? 0 : 10
    height: 38

    // Item container
    Rectangle {
        id: bg
        anchors.fill: parent
        radius: 12
        color: isActive ? bgColor : "transparent"

        // Items layuot
        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: isCollapsed ? 0 : 20
            anchors.rightMargin: isCollapsed ? 0 : 20
            spacing: iconTextSpacing

            anchors.centerIn: parent
            width: parent.width

            SvgIcon {
                pathData: itemRoot.iconPath
                color: itemRoot.isActive ? Theme.textColor : Theme.mutedTextColor
                size: 16
                Layout.alignment: Qt.AlignHCenter
            }

            Text {
                visible: !isCollapsed
                text: itemRoot.label
                color: itemRoot.isActive ? Theme.textColor : Theme.mutedTextColor
                font.pixelSize: Theme.fontSizeMedium
                font.bold: itemRoot.boldText

                horizontalAlignment: Text.AlignLeft
                verticalAlignment: Text.AlignVCenter

                Layout.preferredWidth: contentWidth
            }

            Item {
                Layout.fillWidth: true
                visible: !isCollapsed
            }
        }

        MouseArea {
            id: mArea
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            onClicked: itemRoot.clicked()
        }
    }
}
