import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "Views"
import "Singleton"
import SmtpMua

ApplicationWindow {
    id: mainWindow
    visible: true
    width: 1024
    height: 1024
    title: "Email Client"

    readonly property color windowBg: "#1B263B"
    readonly property color sidebarBg: "#0D1B2A"
    readonly property color panelColor: "#1B263B"
    readonly property color itemBgColor: "#1A2234"
    readonly property color itemBorderColor: "#2A364F"
    readonly property color textColor: "#E2E8F0"
    readonly property color mutedTextColor: "#8494A8"
    readonly property color unreadColor: "#F59E0B"

    color: Theme.windowBg;


    FilterPopup {
        id: mainFilterPopup
    }

    ComposePopup {
        id: composePopup
    }

    ReadMessagePopup{
        id: readMail
    }

    RowLayout {
        anchors.fill: parent
        spacing: 0
        // Layout.leftMargin: 20

        ListModel {
            id: navModel
            ListElement { name: "Inbox"; path: "M22 12h-6l-2 3h-4l-2-3H2 v8a2 2 0 0 0 2 2h16a2 2 0 0 0 2-2z M5.45 5.11L2 12v6a2 2 0 0 0 2 2h16a2 2 0 0 0 2-2v-6l-3.45-6.89A2 2 0 0 0 16.76 4H7.24a2 2 0 0 0-1.79 1.11z" }
            ListElement { name: "Sent"; path: "M22 2L11 13M22 2l-7 20-4-9-9-4 20-7z" }
            ListElement { name: "Drafts"; path: "M13 2H6a2 2 0 0 0-2 2v16a2 2 0 0 0 2 2h12a2 2 0 0 0 2-2V9z M13 2v7h7" }
            ListElement { name: "Starred"; path: "M12 2l3.09 6.26L22 9.27l-5 4.87 1.18 6.88L12 17.77l-6.18 3.25L7 14.14 2 9.27l6.91-1.01L12 2z" }
            ListElement { name: "Saved"; path: "M20.59 13.41l-7.17 7.17a2 2 0 0 1-2.83 0L2 12V2h10l8.59 8.59a2 2 0 0 1 0 2.82z M7 7h.01" }
            ListElement { name: "Spam"; path: "M10.29 3.86L1.82 18a2 2 0 0 0 1.71 3h16.94a2 2 0 0 0 1.71-3L13.71 3.86a2 2 0 0 0-3.42 0z M12 9v4 M12 17h.01" }
            ListElement { name: "Trash"; path: "M3 6h18 M19 6v14a2 2 0 0 1-2 2H7a2 2 0 0 1-2-2V6m3 0V4a2 2 0 0 1 2-2h4a2 2 0 0 1 2 2v2" }
        }

        ListModel {
            id: folModel
            ListElement { name: "School"; path: "M22 19a2 2 0 0 1-2 2H4a2 2 0 0 1-2-2V5a2 2 0 0 1 2-2h5l2 3h9a2 2 0 0 1 2 2z"}
            ListElement { name: "Work"; path: "M22 19a2 2 0 0 1-2 2H4a2 2 0 0 1-2-2V5a2 2 0 0 1 2-2h5l2 3h9a2 2 0 0 1 2 2z"}
            ListElement { name: "Family"; path: "M22 19a2 2 0 0 1-2 2H4a2 2 0 0 1-2-2V5a2 2 0 0 1 2-2h5l2 3h9a2 2 0 0 1 2 2z"}
        }

        // ListModel {
        //     id: emailModel
        //     ListElement {
        //         checked: false; unread: true; starred: false; saved: false;
        //         sender: "Sender name"; title: "Letter title...";
        //         snippet: "A wonderful serenity..."; time: "12:53"
        //     }
        //     ListElement {
        //         checked: true; unread: true; starred: true; saved: false;
        //         sender: "Sender name"; title: "Letter title...";
        //         snippet: "A wonderful serenity..."; time: "25 Feb."
        //     }
        //     ListElement {
        //         checked: false; unread: true; starred: false; saved: false;
        //         sender: "Sender name"; title: "Letter title...";
        //         snippet: "A wonderful serenity..."; time: "12:53"
        //     }
        //     ListElement {
        //         checked: true; unread: true; starred: true; saved: false;
        //         sender: "Sender name"; title: "Letter title...";
        //         snippet: "A wonderful serenity..."; time: "25 Feb."
        //     }
        // }

        ListModel {
            id: mockEmailModel

            ListElement {
                id_msg: 1
                id_fol: 1
                subject: "Welcome to SMTP Client"
                body: "This is a test message to check your UI layout."
                receiver: "user@example.com"
                status: 2
                isSeen: true
                isStarred: true
                createdAt: "10:45 AM"

                isChecked: false

            }

            ListElement {
                id_msg: 2
                id_fol: 1
                subject: "GitHub: New Login Detected"
                body: "A new login was detected on your account from Kyiv, Ukraine."
                receiver: "dev@github.com"
                status: 2
                isSeen: false
                isStarred: false
                createdAt: "Yesterday"

                isChecked: false

            }

            ListElement {
                id_msg: 3
                id_fol: 1
                subject: "Urgent: Project Diploma Update"
                body: "Please check the latest requirements for the Neo4j integration."
                receiver: "professor@university.edu"
                status: 4 // Failed
                isSeen: false
                isStarred: true
                createdAt: "Monday"

                isChecked: false
            }
        }

        Sidebar {
            id: mainSidebar
        }

        ColumnLayout {
            Layout.leftMargin: 20
            Layout.rightMargin: 20
            Layout.topMargin: 15
            Layout.bottomMargin: 15

            Layout.fillWidth: true
            Layout.fillHeight: true

            spacing: 20

            TopSearchBar {
                Layout.fillWidth: true
            }

            Toolbar {
                Layout.fillWidth: true
            }

            EmailListView {
                Layout.fillWidth: true
                Layout.fillHeight: true
            }
        }
    }
}
