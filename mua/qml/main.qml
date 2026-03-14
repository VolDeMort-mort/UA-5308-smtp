import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "Views"
import "Singleton"
import SmtpMua
import "Components"

ApplicationWindow {
    id: rootWindow
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

    // Elements
    ListModel {
        id: mockEmailModel

        ListElement {
            id_msg: 1; id_fol: 1; subject: "Welcome to SMTP Client"; body: "This is a test message to check your UI layout.";
            receiver: "user@example.com"; status: 0; isSeen: false; isStarred: true; isSaved: false; createdAt: "10:45 AM"; isChecked: false
        }

        ListElement {
            id_msg: 2; id_fol: 2; subject: "GitHub: New Login Detected"; body: "A new login was detected on your account from Kyiv, Ukraine. If this wasn't you, please change your password.";
            receiver: "dev@github.com"; status: 0; isSeen: false; isStarred: false; isSaved: true; createdAt: "Yesterday"; isChecked: false
        }

        ListElement {
            id_msg: 3; id_fol: 2; subject: "Urgent: Project Diploma Update"; body: "Please check the latest requirements for the Neo4j integration and LLM processing logic.";
            receiver: "professor@university.edu"; status: 1; isSeen: false; isStarred: true; isSaved: false; createdAt: "Monday"; isChecked: false
        }

        ListElement {
            id_msg: 4; id_fol: 1; subject: "Amazon: Your order has shipped!"; body: "Great news! Your package with 'Mechanical Keyboard' is on its way to your destination.";
            receiver: "ship@amazon.com"; status: 2; isSeen: false; isStarred: false; isSaved: false; createdAt: "09:12 AM"; isChecked: false
        }

        ListElement {
            id_msg: 5; id_fol: 1; subject: "Security Alert: Microsoft Account"; body: "We detected unusual activity. Someone might have accessed your account from a new device.";
            receiver: "account-security@ms.com"; status: 3; isSeen: false; isStarred: false; isSaved: false; createdAt: "08:05 AM"; isChecked: false
        }

        ListElement {
            id_msg: 6; id_fol: 3; subject: "LinkedIn: You have 5 new job matches"; body: "Software Engineer, QA Intern, C++ Developer and more roles that match your skills.";
            receiver: "jobs@linkedin.com"; status: 3; isSeen: false; isStarred: false; isSaved: true; createdAt: "Mar 08"; isChecked: false
        }

        ListElement {
            id_msg: 7; id_fol: 3; subject: "Weekly Newsletter: AI News"; body: "Top 10 new models released this week. Why Gemini 3 Flash Image is a game changer for developers.";
            receiver: "news@techcrunch.com"; status: 1; isSeen: false; isStarred: false; isSaved: false; createdAt: "Mar 07"; isChecked: false
        }

        ListElement {
            id_msg: 8; id_fol: 3; subject: "Discord: You missed a call"; body: "Your friend 'Gamer123' tried to call you in the 'General' voice channel.";
            receiver: "notifications@discord.com"; status: 2; isSeen: false; isStarred: false; isSaved: false; createdAt: "Mar 06"; isChecked: false
        }

        ListElement {
            id_msg: 9; id_fol: 1; subject: "Bank: Monthly Statement"; body: "Your monthly financial statement for February 2026 is now available for download in PDF format.";
            receiver: "info@monobank.ua"; status: 2; isSeen: false; isStarred: true; isSaved: true; createdAt: "Mar 05"; isChecked: false
        }

        ListElement {
            id_msg: 10; id_fol: 1; subject: "System: Server Down Alert"; body: "Critical: Your node 'Neo4j-Graph-DB' is unreachable. Check your AWS instance status immediately.";
            receiver: "admin@aws.amazon.com"; status: 4; isSeen: false; isStarred: true; isSaved: false; createdAt: "Mar 04"; isChecked: false
        }
    }

    // Folders
    ListModel {
        id: folModel

        ListElement { id_fol: 1; name: "School";}
        ListElement { id_fol: 2; name: "Work";}
        ListElement { id_fol: 3; name: "Family";}
        ListElement { id_fol: 4; name: "Family";}

    }


    FilterModel {
        id: filteredModel
        model: mockEmailModel
        delegate: MailListDelegate {}
    }

    // Program layout
    RowLayout {
        anchors.fill: parent
        spacing: 0

        Sidebar {
            id: mainSidebar
            folderModel: folModel
            onFolderSelected: (fol_id) => mainController.filterFold(fol_id)
            onNavSelected: (type) => mainController.filterNav(type)
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

            MailArea {
                id: areaContainer
            }
        }
    }

    // Binding with logic
    QtObject{
        id: mainController

        // FIX
        function changeState(state){
            areaContainer.state = state
        }

        function filterNav(type) {
            filteredModel.resetFilters();
            console.log("Controller: Navigating to type", type);

            switch(type) {
            case NavType.Inbox:
                filteredModel.filterStatus = 2;
                break;
            case NavType.Sent:
                filteredModel.filterStatus = 1;
                break;
            case NavType.Draft:
                filteredModel.filterStatus = 0;
                break;
            case NavType.Starred:
                filteredModel.starredOnly = true;
                break;
            case NavType.Saved:
                filteredModel.savedOnly = true;
                break;
            case NavType.Spam:
                break;
            case NavType.Trash:
                filteredModel.filterStatus = 3;
                break;
            }

            changeState("LISTSTATE")
        }

        function filterFold(fol_id) {
            filteredModel.resetFilters();
            filteredModel.filterFolder = fol_id;

            changeState("LISTSTATE")
        }

    }
}
