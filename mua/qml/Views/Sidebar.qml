import QtQuick
import QtQuick.Layouts
import "../Components"
import SmtpMua

Rectangle {
    id: sidebar

    property bool isCollapsed: false
    property int currentNavIndex: 0
    property int currentFoldIndex: 0

    Layout.fillHeight: true
    Layout.preferredWidth: isCollapsed ? 70 : 280
    color: sidebarBg

    radius: isCollapsed ? 20 : 0

    topRightRadius: 20
    bottomRightRadius: 20

    clip: true

    Behavior on Layout.preferredWidth {
        NumberAnimation { duration: 300; easing.type: Easing.OutCubic }
    }

    // Sidebar layout
    ColumnLayout {
        anchors.fill: parent
        anchors.topMargin: 20
        anchors.bottomMargin: 20
        anchors.leftMargin: 10
        anchors.rightMargin: 10
        spacing: 20


        // Profile part
        RowLayout {
            Layout.fillWidth: true
            Layout.preferredHeight: 40
            Layout.leftMargin: 10

            spacing: sidebar.isCollapsed ? 0 : 12

            Rectangle {
                width: 36; height: 36; radius: 18; color: "#E2E8F0"
                Layout.alignment: Qt.AlignHCenter
            }

            ColumnLayout {
                spacing: 0
                visible: !sidebar.isCollapsed
                opacity: sidebar.isCollapsed ? 0 : 1
                Behavior on opacity { NumberAnimation { duration: 200 } }

                Text { text: "Name Surname"; color: textColor; font.bold: true; font.pixelSize: 16 }
                Text { text: "testmail@gmail.com"; color: mutedTextColor; font.pixelSize: 12 }
            }

            Item { Layout.fillWidth: true; visible: !sidebar.isCollapsed }
        }

        // Create btn
        Rectangle {
            // anchors.fill: parent
            radius: 15
            color: panelColor

            implicitWidth: sidebar.isCollapsed ? 50: parent.width
            height: 50


            Layout.alignment: !sidebar.isCollapsed ? Qt.AlignHCenter : Qt.AlignLeft


            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 12
                anchors.rightMargin: 12
                spacing: 5

                Item {
                    Layout.fillWidth: true
                }

                SvgIcon {
                    pathData: "M12 5v14 M5 12h14"
                    color: textColor
                    size: 20
                    Layout.alignment: Qt.AlignVCenter
                }

                Text {
                    text: sidebar.isCollapsed ? "" : "Create new"
                    color: textColor
                    font.pixelSize: 20
                    font.bold: true

                    horizontalAlignment: Text.AlignCenter
                    verticalAlignment: Text.AlignVCenter
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
                onClicked: {composePopup.open()}
            }
        }

        // Navigation part
        ColumnLayout {
            Layout.fillWidth: true
            spacing: 8

            Repeater {
                model: navModel
                delegate: SidebarItem {
                    label: sidebar.isCollapsed ? "" : model.name
                    iconPath: model.path
                    isActive: sidebar.currentNavIndex === index
                    isCollapsed: sidebar.isCollapsed
                    onClicked: sidebar.currentNavIndex = index
                }
            }
        }

        // Spacer
        Rectangle {
            Layout.fillWidth: sidebar.isCollapsed ? false: true
            Layout.leftMargin: sidebar.isCollapsed ? 10 : 0
            Layout.rightMargin: sidebar.isCollapsed ? 10 : 0
            height: 1
            color: itemBorderColor
            opacity: 1
        }

        // "Your folders"
        Text {
            text: "Your folders"
            visible: !sidebar.isCollapsed
            color: mutedTextColor
            font.bold: true
            font.pixelSize: 20
            Layout.alignment: Qt.AlignCenter
        }

        // Folders part
        Flickable {
            id: navFlickable
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            contentHeight: navLayout.implicitHeight
            boundsBehavior: Flickable.StopAtBounds

            ColumnLayout {
                id: navLayout
                width: navFlickable.width
                spacing: 6

                Repeater {
                    model: folModel
                    delegate: SidebarItem {
                        label: sidebar.isCollapsed ? "" : model.name
                        iconPath: model.path
                        isActive: sidebar.currentFoldIndex === index
                        isCollapsed: sidebar.isCollapsed
                        onClicked: sidebar.currentFoldIndex = index
                    }
                }

                SidebarItem {
                    label: sidebar.isCollapsed ? "" : "New folder"
                    iconPath: "M12 5v14 M5 12h14"
                    isCollapsed: sidebar.isCollapsed
                    onClicked: {
                        folModel.append({"name": "New Folder", "path": "M22 19a2 2 0 0 1-2 2H4a2 2 0 0 1-2-2V5a2 2 0 0 1 2-2h5l2 3h9a2 2 0 0 1 2 2z"})
                    }
                }
            }
        }

        // Controlls part
        RowLayout {
            Layout.fillWidth: true
            Layout.preferredHeight: 40
            spacing: 8
            Layout.leftMargin: 10

            SvgIcon {
                pathData: "M12 15a3 3 0 1 0 0-6 3 3 0 0 0 0 6z M19.4 15a1.65 1.65 0 0 0 .33 1.82l.06.06a2 2 0 0 1 0 2.83 2 2 0 0 1-2.83 0l-.06-.06a1.65 1.65 0 0 0-1.82-.33 1.65 1.65 0 0 0-1 1.51V21a2 2 0 0 1-2 2 2 2 0 0 1-2-2v-.09A1.65 1.65 0 0 0 9 19.4a1.65 1.65 0 0 0-1.82.33l-.06.06a2 2 0 0 1-2.83 0 2 2 0 0 1 0-2.83l.06-.06a1.65 1.65 0 0 0 .33-1.82 1.65 1.65 0 0 0-1.51-1H3a2 2 0 0 1-2-2 2 2 0 0 1 2-2h.09A1.65 1.65 0 0 0 4.6 9a1.65 1.65 0 0 0-.33-1.82l-.06-.06a2 2 0 0 1 0-2.83 2 2 0 0 1 2.83 0l.06.06a1.65 1.65 0 0 0 1.82.33H9a1.65 1.65 0 0 0 1-1.51V3a2 2 0 0 1 2-2 2 2 0 0 1 2 2v.09a1.65 1.65 0 0 0 1 1.51 1.65 1.65 0 0 0 1.82-.33l.06-.06a2 2 0 0 1 2.83 0 2 2 0 0 1 0 2.83l-.06.06a1.65 1.65 0 0 0-.33 1.82V9a1.65 1.65 0 0 0 1.51 1H21a2 2 0 0 1 2 2 2 2 0 0 1-2 2h-.09a1.65 1.65 0 0 0-1.51 1z";
                color: mutedTextColor;
                size: 16
                visible: !sidebar.isCollapsed}

            Text { text: "Settings"; color: mutedTextColor; font.pixelSize: 20; visible: !sidebar.isCollapsed}

            Item { Layout.fillWidth: true; visible: !isCollapsed }

            // Hide/show btn
            Rectangle {
                id: collapseBtn
                Layout.alignment: sidebar.isCollapsed ? Qt.AlignHCenter : Qt.AlignLeft
                width: sidebar.isCollapsed ? 44 : 100
                height: 44
                radius: 22
                color: panelColor

                Behavior on width { NumberAnimation { duration: 250 } }

                RowLayout {
                    anchors.centerIn: parent

                    SvgIcon {
                        pathData: "M19 12H5 M12 19l-7-7 7-7"
                        color: textColor; size: 20
                        rotation: sidebar.isCollapsed ? 180 : 0
                        Behavior on rotation { NumberAnimation { duration: 300 } }
                    }

                    Text {
                        text: "Hide"; color: textColor;
                        font.bold: true; font.pixelSize: 14
                        visible: !sidebar.isCollapsed
                    }
                }

                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    onClicked: sidebar.isCollapsed = !sidebar.isCollapsed
                }
            }
        }
    }
}
