import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import SmtpMua
import "./Components"
import "./Views"

ApplicationWindow {
    id: rootWindow
    visible: true
    width: 1200
    height: 800
    title: "SMTP/IMAP Client"
    color: Theme.windowBg

    property string currentUserEmail: ""
    property string currentUserName: "User"
    property string currentUserSurname: ""

    Rectangle {
        id: mainContent
        anchors.fill: parent
        color: Theme.windowBg

        property bool isSignUpMode: false

        state: "LOGIN"

        states: [
            State {
                name: "LOGIN"
                PropertyChanges { target: mainAppLayout; visible: false }
            },
            State {
                name: "APP"
                PropertyChanges { target: signInView; visible: false }
                PropertyChanges { target: signUpView; visible: false }
                PropertyChanges { target: mainAppLayout; visible: true }
            }
        ]

        SignInView {
            id: signInView
            anchors.fill: parent
            visible: mainContent.state === "LOGIN" && !mainContent.isSignUpMode

            onSignInRequest: (email) => {
                                 rootWindow.currentUserEmail = email

                                 mainContent.state = "APP"
                                 muaBridge.fetchFolders()
                                 muaBridge.fetchMails("INBOX", 0, 50)
                             }

            onSwitchToSignUp: mainContent.isSignUpMode = true
        }

        LoginView {
            id: signUpView
            anchors.fill: parent
            visible: mainContent.state === "LOGIN" && mainContent.isSignUpMode

            onSignUpRequest: (email, name, surname) => {
                                 rootWindow.currentUserEmail = email
                                 rootWindow.currentUserName = name
                                 rootWindow.currentUserSurname = surname

                                 mainContent.state = "APP"
                                 muaBridge.fetchFolders()
                                 muaBridge.fetchMails("INBOX", 0, 50)
                             }

            onSwitchToSignIn: mainContent.isSignUpMode = false
        }

        RowLayout {
            id: mainAppLayout
            anchors.fill: parent
            visible: false

            Sidebar {
                id: sidebar
                folderModel: folModel
                Layout.fillHeight: true
            }

            ColumnLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.margins: 20
                spacing: 15

                TopSearchBar {
                    Layout.fillWidth: true
                    onSearchTextChanged: (query) => {
                                             filteredModel.searchQuery = query
                                             mainController.changeState("LISTSTATE")
                                         }
                }

                Toolbar {
                    Layout.fillWidth: true
                }

                MailArea {
                    id: areaContainer
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                }
            }
        }
    }

    ListModel {
        id: folModel
    }

    ListModel {
        id: mockEmailModel
    }

    FilterModel {
        id: filteredModel
        model: mockEmailModel
    }

    ComposePopup {
        id: composePopup
    }

    FilterPopup {
        id: mainFilterPopup
    }


    Connections {
        target: muaBridge

        function onMailFetched() {
            areaContainer.selectedMailData = muaBridge.currentMail
            areaContainer.state = "MAILSTATE"
        }

        function onAttachmentDownloaded(outputPath) {
            console.log("Attachment saved to: " + outputPath)
        }
    }

    QtObject {
        id: mainController

        function changeState(newState) {
            areaContainer.state = newState
        }

        function handleLogout() {
            muaBridge.disconnectFromServer()
            mockEmailModel.clear()
            folModel.clear()

            rootWindow.currentUserEmail = ""
            rootWindow.currentUserName = ""
            rootWindow.currentUserSurname = ""

            areaContainer.selectedMailData = null
            mainContent.state = "LOGIN"
        }

        function filterNav(type) {
            let targetFolder = "";

            switch(type) {
            case NavType.Inbox: targetFolder = "INBOX"; break;
            case NavType.Sent: targetFolder = "[Gmail]/Sent Mail"; break;
            case NavType.Trash: targetFolder = "[Gmail]/Trash"; break;
            case NavType.Spam: targetFolder = "[Gmail]/Spam"; break;
            }

            if (targetFolder !== "") {
                mockEmailModel.clear()
                muaBridge.fetchMails(targetFolder, 0, 50)
            }

            areaContainer.selectedMailData = null
            changeState("LISTSTATE")
        }
    }
}
