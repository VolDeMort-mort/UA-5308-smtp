import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Effects
import SmtpMua
import "../Components"

Rectangle {
    id: signUpPageRoot
    anchors.fill: parent
    color: Theme.windowBg

    signal signUpRequest(string email, string name, string surname)
    signal switchToSignIn()

    property bool isConnecting: false

    Item {
        anchors.centerIn: parent
        width: 420
        height: 450

        MultiEffect {
            source: signUpPanel
            anchors.fill: signUpPanel
            shadowEnabled: true
            shadowColor: Qt.rgba(0, 0, 0, 0.3)
            shadowBlur: 1.0
            shadowHorizontalOffset: 0
            shadowVerticalOffset: 10
        }

        Rectangle {
            id: signUpPanel
            anchors.fill: parent
            color: Theme.sidebarBg
            radius: 15
            border.width: 1
            border.color: Qt.darker(Theme.sidebarBg, 1.2)

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 40
                spacing: 15

                Text {
                    text: "Create Account"
                    font.pixelSize: 28
                    font.bold: true
                    color: Theme.textColor
                    Layout.alignment: Qt.AlignHCenter
                }

                Item { height: 10; Layout.fillWidth: true }


                    StyledInput {
                        id: nameField
                        Layout.fillWidth: true
                        label: "Name:"
                        placeholder: "John"
                    }

                    StyledInput {
                        id: surnameField
                        Layout.fillWidth: true
                        label: "Surname:"
                        placeholder: "Doe"
                    }


                StyledInput {
                    id: emailField
                    Layout.fillWidth: true
                    label: "Email:"
                    placeholder: "user@example.com"
                }

                StyledInput {
                    id: passField
                    Layout.fillWidth: true
                    label: "Password:"
                    placeholder: "Password"
                    echoMode: TextInput.Password
                }

                Item { Layout.fillHeight: true }

                Button {
                    id: submitButton
                    Layout.fillWidth: true
                    Layout.preferredHeight: 45

                    enabled: !signUpPageRoot.isConnecting &&
                             nameField.textValue.length > 0 &&
                             surnameField.textValue.length > 0 &&
                             emailField.textValue.length > 0 &&
                             passField.textValue.length > 0

                    background: Rectangle {
                        radius: 15
                        color: {
                            if (!submitButton.enabled) return Theme.panelColor
                            if (submitButton.pressed) return Qt.darker(Theme.itemBgColor, 1.2)
                            if (submitButton.hovered) return Qt.lighter(Theme.itemBgColor, 1.1)
                            return Theme.itemBgColor
                        }
                        opacity: submitButton.enabled ? 1.0 : 0.5
                        Behavior on color { ColorAnimation { duration: 150 } }
                        Behavior on opacity { NumberAnimation { duration: 200 } }
                    }

                    contentItem: Text {
                        text: signUpPageRoot.isConnecting ? "Connecting..." : "Continue"
                        color: submitButton.enabled ? Theme.textColor : Theme.mutedTextColor
                        font.pixelSize: Theme.fontSizeHeader
                        font.bold: true
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }

                    onClicked: {
                        signUpPageRoot.isConnecting = true

                        muaBridge.connectToServer(
                            "smtp.gmail.com", 465,
                            "imap.gmail.com", 993,
                            emailField.textValue,
                            passField.textValue
                        )
                    }
                }



                Item { Layout.fillHeight: true }

                RowLayout {
                    Layout.alignment: Qt.AlignHCenter
                    spacing: 5

                    Text {
                        text: "Already have an account?"
                        color: Theme.mutedTextColor
                        font.pixelSize: Theme.fontSizeMedium
                    }

                    Text {
                        text: "Sign In"
                        color: Theme.iconSelectColor
                        font.pixelSize: Theme.fontSizeMedium
                        font.bold: true

                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            onClicked: signUpPageRoot.switchToSignIn()
                        }
                    }
                }
            }
        }
    }

    Connections {
        target: muaBridge

        function onConnected() {
            signUpPageRoot.isConnecting = false
            signUpPageRoot.signUpRequest(emailField.textValue, nameField.textValue, surnameField.textValue)
        }

        function onErrorOccurred(message) {
            signUpPageRoot.isConnecting = false
        }
    }
}
