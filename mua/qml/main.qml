import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "Views"
import "Singleton"
import SmtpMua

ApplicationWindow {
    id: mainWindow
    visible: true

    width: Screen.width
    height: Screen.height
    minimumWidth: 800
    minimumHeight: 600

    visibility: Window.Maximized
    title: "Email Client"

    color: Theme.windowBg;

    // Pop up windows
    FilterPopup {
        id: mainFilterPopup
    }

    ComposePopup {
        id: composePopup
    }

    ReadMessagePopup{
        id: readMail
    }

    // Program layout
    RowLayout {
        anchors.fill: parent
        spacing: 0

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
