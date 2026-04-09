import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import SmtpMua
import "../Components"
Item{
    id: readPopup

    property var mailData: null
    signal backClicked

    anchors.fill: parent

    focus: true

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 25
        spacing: 20


        // Subject + close button
        RowLayout {
            Layout.fillWidth: true
            spacing: 20

            IconButton {
                iconPath: Icons.rightArrow
                onClicked: backClicked()
            }

            Text {
                text: mailData ? mailData.subject : "No Subject"
                color: Theme.textColor
                font.pixelSize: Theme.fontSizeHeader
                font.bold: true
                Layout.fillWidth: true
                elide: Text.ElideRight
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
                    text: mailData ? mailData.receiver[0].toUpperCase() : "?"
                    color: "white"
                    font.bold: true
                }
            }

            // Other info
            ColumnLayout {
                spacing: 2
                Text {
                    text: mailData ? mailData.receiver : "Unknown"
                    color: Theme.textColor
                    font.pixelSize: Theme.fontSizeLarge
                    font.bold: true
                }
                Text {
                    text: "to me"
                    color: Theme.mutedTextColor
                    font.pixelSize: Theme.fontSizeMedium
                }
            }

            Item { Layout.fillWidth: true }

            // Date
            Text {
                text: mailData ? mailData.createdAt : ""
                color: Theme.mutedTextColor
                font.pixelSize: Theme.fontSizeMedium
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
                text: mailData ? mailData.body : ""
                color: Theme.textColor
                font.pixelSize: Theme.fontSizeLarge
                lineHeight: 1.4
                wrapMode: Text.WordWrap
                textFormat: Text.PlainText
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
                    font.pixelSize: Theme.fontSizeLarge
                }
            }
        }
    }
}
