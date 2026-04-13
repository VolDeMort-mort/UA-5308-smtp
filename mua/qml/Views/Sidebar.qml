import QtQuick
import QtQuick.Layouts
import "../Components"
import SmtpMua

Rectangle {
    id: sidebar

    property var folderModel: null

    property bool isCollapsed: false
    property int currentNavIndex: 0
    property int currentFoldIndex: -1

    signal folderSelected(int fol_id)
    signal navSelected(int navType)

    readonly property var navModel: [
        { name: "Inbox", path: Icons.inbox, navType: NavType.Inbox },
        { name: "Sent", path: Icons.sent, navType: NavType.Sent },
        { name: "Drafts", path: Icons.draft, navType: NavType.Draft },
        { name: "Starred", path: Icons.star, navType: NavType.Starred },
        { name: "Saved", path: Icons.bookmark, navType: NavType.Saved },
        { name: "Spam", path: Icons.spam, navType: NavType.Spam },
        { name: "Trash", path: Icons.trash, navType: NavType.Trash }
    ]

    Layout.fillHeight: true
    Layout.preferredWidth: isCollapsed ? 70 : 250
    color: Theme.sidebarBg


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
            Layout.leftMargin: 5

            spacing: sidebar.isCollapsed ? 0 : 10

            Rectangle {
                width: 36;
                height: 36;
                radius: 18;
                color: "white"
                Layout.alignment: Qt.AlignHCenter

                MouseArea {
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: sideBarController.handleProfileClick()
                }
            }

            ColumnLayout {
                spacing: 0
                visible: !sidebar.isCollapsed
                opacity: sidebar.isCollapsed ? 0 : 1
                Behavior on opacity { NumberAnimation { duration: 200 } }

                Text {
                    text: "Name Surname";
                    color: Theme.textColor;
                    font.bold: true;
                    font.pixelSize: Theme.fontSizeLarge }
                Text {
                    text: "testmail@gmail.com";
                    color: Theme.mutedTextColor;
                    font.pixelSize: Theme.fontSizeSmall
                }
            }
        }

        // Create btn
        Rectangle {
            id: create_tn
            radius: 15
            color: Theme.panelColor

            implicitWidth: parent.width
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
                    pathData: Icons.plus
                    color: Theme.textColor
                    size: 20
                    Layout.alignment: Qt.AlignVCenter
                }

                Text {
                    text: sidebar.isCollapsed ? "" : "Create new"
                    color: Theme.textColor
                    font.pixelSize: Theme.fontSizeHeader
                    font.bold: true

                    horizontalAlignment: Text.AlignCenter
                    verticalAlignment: Text.AlignVCenter
                }

                Item {
                    Layout.fillWidth: true
                }
            }

            MouseArea {
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: sideBarController.handleComposeMail()
            }
        }

        // Navigation part
        Flickable {
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true

            contentHeight: navLayout.implicitHeight

            boundsBehavior: Flickable.StopAtBounds

            // Navigation
            ColumnLayout {
                id: navLayout
                width: parent.width
                spacing: 5

                // Basic navigation
                Repeater {
                    model: sidebar.navModel
                    delegate: SidebarItem {
                        label: sidebar.isCollapsed ? "" : modelData.name
                        iconPath: modelData.path
                        isActive: sidebar.currentNavIndex === index
                        isCollapsed: sidebar.isCollapsed
                        onClicked: sideBarController.handleNavClick(modelData.navType, index)
                    }
                }

                // Spacer
                Rectangle {
                    Layout.fillWidth: true
                    Layout.topMargin: navLayout.spacing
                    Layout.leftMargin: sidebar.isCollapsed ? 10 : 0
                    Layout.rightMargin: sidebar.isCollapsed ? 10 : 0
                    Layout.bottomMargin: navLayout.spacing

                    height: 1
                    color: Theme.itemBorderColor
                    opacity: 1
                }

                // "Your folders"
                Text {
                    Layout.topMargin: navLayout.spacing
                    Layout.bottomMargin: navLayout.spacing
                    Layout.alignment: Qt.AlignCenter

                    text: "Your folders"
                    visible: !sidebar.isCollapsed
                    color: Theme.textColor
                    font.bold: true
                    font.pixelSize: Theme.fontSizeLarge
                }

                // Folders navigation
                Repeater {
                    model: folderModel
                    delegate: SidebarItem {
                        label: sidebar.isCollapsed ? "" : model.name
                        iconPath: Icons.folder
                        isActive: sidebar.currentFoldIndex === index
                        isCollapsed: sidebar.isCollapsed
                        onClicked: sideBarController.handleFoldClick(index, model.id_fol)
                    }
                }


                SidebarItem {
                    label: sidebar.isCollapsed ? "" : "New folder"
                    iconPath: Icons.plus
                    isCollapsed: sidebar.isCollapsed
                    onClicked: sideBarController.handleCreateFolder()
                }
            }
        }

        // Settings part
        RowLayout {
            Layout.fillWidth: true
            Layout.preferredHeight: 40
            spacing: 8
            Layout.leftMargin: sidebar.isCollapsed ? 0 : 10

            Layout.alignment: Qt.AlignHCenter

            // Settings btn
            SvgIcon {
                pathData: Icons.settings;
                color: Theme.mutedTextColor;
                size: 16
                visible: !sidebar.isCollapsed}

            Text {
                text: "Settings";
                color: Theme.mutedTextColor;
                font.pixelSize: Theme.fontSizeLarge;
                visible: !sidebar.isCollapsed}

            Item { Layout.fillWidth: true; visible: !sidebar.isCollapsed }

            // Hide/show rect
            Rectangle {
                id: collapseBtn
                // Layout.alignment: sidebar.isCollapsed ? Qt.AlignHCenter : Qt.AlignLeft
                Layout.alignment: Qt.AlignHCenter
                Layout.preferredWidth: sidebar.isCollapsed ? 44 : 100
                height: 44
                radius: 20
                color: Theme.panelColor

                Behavior on width { NumberAnimation { duration: 250 } }

                RowLayout {
                    anchors.centerIn: parent

                    SvgIcon {
                        pathData: Icons.rightArrow
                        color: Theme.textColor;
                        size: 20
                        rotation: sidebar.isCollapsed ? 180 : 0
                        Behavior on rotation { NumberAnimation { duration: 300 } }
                    }

                    Text {
                        text: "Hide";
                        color: Theme.textColor;
                        font.bold: true;
                        font.pixelSize: Theme.fontSizeLarge
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

    // Logic
    QtObject{
        id: sideBarController


        function handleProfileClick(){
            console.log("Opening profile...")
        }

        function handleComposeMail(){
            composePopup.open()
            console.log("Opening compose window...")
        }

        function handleNavClick(type, index){
            sidebar.currentNavIndex = index;
            sidebar.currentFoldIndex = -1;

            console.log("Opened mails with index: ", index)
            navSelected(type)
        }

        function handleFoldClick(index, fol_id){
            sidebar.currentFoldIndex = index
            sidebar.currentNavIndex = -1;
            console.log("Opened folder with id: ", fol_id)

            folderSelected(fol_id)
        }

        function handleCreateFolder(){
            console.log("Creating new folder...")
        }

        function handleSettingsClick(){
            console.log("Opening settings...")
        }
    }
}

