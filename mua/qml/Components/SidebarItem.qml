import QtQuick
import QtQuick.Layouts
import SmtpMua


Item {
    id: itemRoot
    property string iconPath: ""
    property string label: ""
    property bool isActive: false
    property color bgColor: isActive ? panelColor : (mArea.containsMouse ? Qt.darker(panelColor, 1.2) : "transparent")
    property bool boldText: isActive
    property bool isCollapsed: false
    signal clicked()


    property int iconTextSpacing: 20

    Layout.fillWidth: true
    Layout.leftMargin: isCollapsed ? 0 : 10
    Layout.rightMargin: isCollapsed ? 0 : 10
    height: 38

    Rectangle {
        id: bg
        anchors.fill: parent
        radius: isCollapsed ? height / 2 : 12
        color: isActive ? bgColor : "transparent"
        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 20
            anchors.rightMargin: 20
            spacing: iconTextSpacing

            anchors.centerIn: parent
            width: parent.width

            SvgIcon {
                pathData: itemRoot.iconPath
                color: itemRoot.isActive ? textColor : mutedTextColor
                size: 16
                Layout.alignment: Qt.AlignHCenter
            }

            Text {
                visible: !isCollapsed
                text: itemRoot.label
                color: itemRoot.isActive ? textColor : mutedTextColor
                font.pixelSize: Theme.fontSizeMedium
                font.bold: itemRoot.boldText

                horizontalAlignment: Text.AlignLeft
                verticalAlignment: Text.AlignVCenter

                Layout.preferredWidth: contentWidth
            }

            Item {
                Layout.fillWidth: true
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
