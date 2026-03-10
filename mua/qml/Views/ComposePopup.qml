import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import SmtpMua

Popup {
    id: composePopup
    width: 600
    height: 500

    anchors.centerIn: Overlay.overlay

    modal: true
    dim: true
    focus: true

    // Bg
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
            from: composePopup.y + 15
            to: composePopup.y
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

    // Style of the pop up window
    background: Rectangle {
        color: Theme.panelColor
        radius: 12
        border.color: Theme.itemBorderColor
        border.width: 1
    }

    // Main layout
    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 20
        spacing: 15

        Text {
            text: "New Message"
            color: Theme.textColor
            font.pixelSize: Theme.fontSizeHeader
            font.bold: true
        }

        // Receivers
        StyledInput {
            id: receiverInput
            Layout.fillWidth: true
            placeholder: "To (user1@mail.com, user2@mail.com)"
            label: "To:"
        }

        // Subject
        StyledInput {
            id: subjectInput
            Layout.fillWidth: true
            placeholder: "Subject"
            label: "Subject:"
        }

        // Mail body
        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: Qt.alpha("white", 0.03)
            radius: 8
            border.color: Theme.itemBorderColor

            ScrollView {
                anchors.fill: parent
                TextArea {
                    id: bodyText
                    placeholderText: "Write your message here..."
                    color: Theme.textColor
                    font.pixelSize: Theme.fontSizeMedium
                    wrapMode: TextEdit.WordWrap
                    leftPadding: 10
                    topPadding: 10
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 15

            // Attach btn
            Button {
                id: attachBtn
                flat: true
                contentItem: RowLayout {
                    SvgIcon { pathData: Icons.attachment;
                        color: Theme.mutedTextColor; size: 18 }
                    Text {
                        text: "Attach file";
                        color: Theme.mutedTextColor;
                        font.pixelSize: Theme.fontSizeMedium }
                }
            }

            Item { Layout.fillWidth: true }

            //Send btn
            Button {
                id: sendBtn
                Layout.preferredWidth: 100
                Layout.preferredHeight: 36
                background: Rectangle {
                    color: Theme.iconSelectColor
                    radius: 18
                }
                contentItem: Text {
                    text: "Send"
                    color: "white"
                    font.bold: true
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                onClicked: {
                    console.log("Sending to:", receiverInput.textValue)
                    composePopup.close()
                }
            }
        }
    }


    component StyledInput : RowLayout {
        property string label
        property string placeholder
        property alias textValue: input.text

        spacing: 10
        Text {
            text: label
            color: Theme.mutedTextColor
            font.pixelSize: Theme.fontSizeMedium
            Layout.preferredWidth: 60
        }
        Rectangle {
            Layout.fillWidth: true
            height: 35
            color: "transparent"
            border.color: Theme.itemBorderColor
            border.width: 0;

            Rectangle {
                anchors.bottom: parent.bottom; width: parent.width; height: 1;
                color: Theme.itemBorderColor
            }

            TextInput {
                id: input
                anchors.fill: parent
                verticalAlignment: TextInput.AlignVCenter
                color: Theme.textColor
                font.pixelSize: Theme.fontSizeMedium
                clip: true

                Text {
                    text: placeholder
                    color: Theme.mutedTextColor
                    visible: !parent.text && !parent.activeFocus
                    anchors.verticalCenter: parent.verticalCenter
                }
            }
        }
    }
}
