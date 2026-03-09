import QtQuick
import QtQuick.Layouts
import "../Components"
import SmtpMua

Rectangle {
    id: listViewContainer
    Layout.fillWidth: true
    Layout.fillHeight: true

    color: Theme.sidebarBg

    radius: 20

    ListView {
        id: emailList
        anchors.fill: parent

        anchors.leftMargin: 10
        anchors.rightMargin: 20
        anchors.topMargin: 10
        anchors.bottomMargin: 10

        clip: true
        spacing: 8

        focus: true
        highlightFollowsCurrentItem: true
        currentIndex: -1

        model: mockEmailModel

        delegate: EmailDelegate {}
    }
}
