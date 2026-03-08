import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import SmtpMua
import "../Components"

Popup {
    id: readPopup

    property var msgData: null

    width: 700      // FIX
    height: 600

    anchors.centerIn: Overlay.overlay

    modal: true
    dim: true
    focus: true

    // Darker bg
    Overlay.modal: Rectangle {
        color: Qt.alpha("transparent", 0.5)
    }

    // Pop up animation
    enter: Transition {
        NumberAnimation {
            property: "opacity"
            from: 0.0
            to: 1.0
            duration: 300
            easing.type: Easing.OutCubic
        }
        NumberAnimation {
            property: "y"
            from: readPopup.y + 15
            to: readPopup.y
            duration: 500
            easing.type: Easing.OutBack
        }
    }

    // Fade out animation
    exit: Transition {
        NumberAnimation {
            property: "opacity"
            from: 1.0
            to: 0.0
            duration: 150
        }
        NumberAnimation {
            property: "scale"
            from: 1.0
            to: 0.95
            duration: 159
        }
    }

    background: Rectangle {
        color: Theme.panelColor
        radius: 16
        border.color: Theme.itemBorderColor
        border.width: 1
    }

    // Main layout
    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 25
        spacing: 20


        // Subject + close button
        RowLayout {
            Layout.fillWidth: true
            Text {
                text: msgData ? msgData.subject : "No Subject"
                color: Theme.textColor
                font.pixelSize: 22
                font.bold: true
                Layout.fillWidth: true
                elide: Text.ElideRight
            }

            IconButton {
                iconPath: "M19 6.41L17.59 5 12 10.59 6.41 5 5 6.41 10.59 12 5 17.59 6.41 19 12 13.41 17.59 19 19 17.59 13.41 12z"
                onClicked: readPopup.close()
            }
        }

        Item{Layout.fillWidth: true}

        // Sender info + date
        RowLayout {
            Layout.fillWidth: true
            spacing: 12

            // Avatar icon (stub)
            Rectangle {
                width: 40; height: 40; radius: 20
                color: "#F59E0B"
                Text {
                    anchors.centerIn: parent
                    text: msgData ? msgData.receiver[0].toUpperCase() : "?"
                    color: "white"; font.bold: true
                }
            }

            // Other info
            ColumnLayout {
                spacing: 2
                Text {
                    text: msgData ? msgData.receiver : "Unknown"
                    color: Theme.textColor
                    font.pixelSize: 14; font.bold: true
                }
                Text {
                    text: "to me"
                    color: Theme.mutedTextColor
                    font.pixelSize: 12
                }
            }

            Item { Layout.fillWidth: true }

            // Date
            Text {
                text: msgData ? msgData.createdAt : ""
                color: Theme.mutedTextColor
                font.pixelSize: 12
            }
        }

        // Mail buttons
        RowLayout {
            Layout.fillWidth: true
            spacing: 15

            IconButton { iconPath: "M10 9V5l-7 7 7 7v-4.1c5 0 8.5 1.6 11 5.1-1-5-4-10-11-11z"; text: "Reply" }
            IconButton { iconPath: "M6 19c0 1.1.9 2 2 2h8c1.1 0 2-.9 2-2V7H6v12zM19 4h-3.5l-1-1h-5l-1 1H5v2h14V4z"; text: "Delete" }
            IconButton {
                iconPath: "M12 17.27L18.18 21l-1.64-7.03L22 9.24l-7.19-.61L12 2 9.19 8.63 2 9.24l5.46 4.73L5.82 21z"
                text: "Star"
                iconColor: msgData && msgData.isStarred ? "#F59E0B" : Theme.mutedTextColor
            }
        }

        // Spacer
        Rectangle {
            Layout.fillWidth: true; Layout.preferredHeight: 1
            color: Theme.mutedTextColor
            // Layout.topMargin: 5; Layout.bottomMargin: 5
        }

        // Mail body
        ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true

            Text {
                width: parent.width
                text: msgData ? msgData.body : ""
                color: Theme.textColor
                font.pixelSize: 15
                lineHeight: 1.4
                wrapMode: Text.WordWrap
                textFormat: Text.PlainText
            }
        }
    }

    component IconButton : MouseArea {
        property string iconPath
        property string text: ""
        property color iconColor: Theme.mutedTextColor

        implicitWidth: iconRow.width + 10
        implicitHeight: 30
        hoverEnabled: true

        RowLayout {
            id: iconRow
            anchors.centerIn: parent
            spacing: 6
            SvgIcon { pathData: iconPath; color: parent.parent.containsMouse ? "#F59E0B" : iconColor; size: 18 }
            Text {
                text: parent.parent.text;
                color: parent.parent.containsMouse ? "#F59E0B" : Theme.mutedTextColor;
                visible: text !== ""; font.pixelSize: 13
            }
        }
    }
}
