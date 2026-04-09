import QtQuick
import QtQuick.Layouts
import "../Components"
import SmtpMua

Rectangle {
    Layout.fillWidth: true
    Layout.fillHeight: true
    color: Theme.sidebarBg

    radius: 20

    property string status: "list"
    property var selectedMailData: null

    states: [
        State {
            name: "LISTSTATE"
            when: status === "list"
            PropertyChanges {target: mailList; visible: true}
            PropertyChanges {target: mailObj; visible: false}
        },
        State {
            name: "MAILSTATE"
            when: status === "mail"
            PropertyChanges {target: mailList; visible: false}
            PropertyChanges {target: mailObj; visible: true}
        }
    ]

    MailListView{
        id: mailList
        visible: true
        onMailSelected: (index) =>{areaCotroller.handleMailSelection(index)}
    }

    MailView {
        id: mailObj
        visible: false
        mailData: selectedMailData
        onBackClicked: areaCotroller.handleBackClicked()
    }

    QtObject{
        id: areaCotroller

        function handleMailSelection(index){
            selectedMailData = mockEmailModel.get(index)
            state = "MAILSTATE"
        }

        function handleBackClicked(){
            state = "LISTSTATE"
            selectedMailData = null
        }
    }
}
