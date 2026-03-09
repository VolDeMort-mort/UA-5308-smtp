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
                font.pixelSize: Theme.fontSizeHeader
                font.bold: true
                Layout.fillWidth: true
                elide: Text.ElideRight
            }

            IconButton {
                iconPath: Icons.cross
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
                color: Theme.iconSelectColor
                Text {
                    anchors.centerIn: parent
                    text: msgData ? msgData.receiver[0].toUpperCase() : "?"
                    color: "white"
                    font.bold: true
                }
            }

            // Other info
            ColumnLayout {
                spacing: 2
                Text {
                    text: msgData ? msgData.receiver : "Unknown"
                    color: Theme.textColor
                    font.pixelSize: Theme.fontSizeMedium
                    font.bold: true
                }
                Text {
                    text: "to me"
                    color: Theme.mutedTextColor
                    font.pixelSize: Theme.fontSizeSmall
                }
            }

            Item { Layout.fillWidth: true }

            // Date
            Text {
                text: msgData ? msgData.createdAt : ""
                color: Theme.mutedTextColor
                font.pixelSize: Theme.fontSizeSmall
            }
        }

        // Mail buttons
        RowLayout {
            Layout.fillWidth: true
            spacing: 15

            IconButton { iconPath: Icons.forward; text: "Reply" }
            IconButton { iconPath: Icons.trash; text: "Delete" }
            IconButton {
                iconPath: Icons.star
                text: "Star"
                iconColor: msgData && msgData.isStarred ? Theme.iconSelectColor : Theme.mutedTextColor
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
                font.pixelSize: Theme.fontSizeMedium
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
            SvgIcon {
                pathData: iconPath;
                color: parent.parent.containsMouse ? Theme.iconSelectColor : iconColor
                size: 18 }
            Text {
                text: parent.parent.text;
                color: parent.parent.containsMouse ? Theme.iconSelectColor : Theme.mutedTextColor;
                visible: text !== ""
                font.pixelSize: Theme.fontSizeMedium
            }
        }
    }
}
