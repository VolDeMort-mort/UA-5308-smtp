import QtQuick
import QtQuick.Layouts
import "../Components"
import SmtpMua

Rectangle {
    id: listViewContainer

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

        model: filteredModel


        add: Transition {
            NumberAnimation {
                property: "opacity";
                from: 0
                to: 1
                duration: 250
            }
            NumberAnimation {
                property: "y";
                from: -15;
                duration: 300;
                easing.type: Easing.OutCubic
            }
        }

        remove: Transition {
            NumberAnimation {
                property: "opacity";
                to: 0;
                duration: 200
            }
        }

        displaced: Transition {
            NumberAnimation {
                properties: "y";
                duration: 350;
                easing.type: Easing.OutQuad
            }
        }


        // Label "Nothing to slow..."
        Text {
            id: emptyModelText

            anchors.centerIn: parent

            text: "Nothing to show..."
            font.pixelSize: Theme.fontSizeLarge
            color: Theme.mutedTextColor
            visible: !emailList.model || emailList.count === 0

            Behavior on visible {
                NumberAnimation { property: "opacity"; duration: 300 }
            }
        }
    }
}
