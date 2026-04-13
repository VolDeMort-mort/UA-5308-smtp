import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Effects
import SmtpMua
import "../Components"

Rectangle {
    id: signInPageRoot
    anchors.fill: parent
    color: Theme.windowBg

    signal signInRequest(string email)
    signal switchToSignUp()

    property bool isConnecting: false

    Item {
        anchors.centerIn: parent
        width: 420
        height: 350

        MultiEffect {
            source: signInPanel
            anchors.fill: signInPanel
            shadowEnabled: true
            shadowColor: Qt.rgba(0, 0, 0, 0.3)
            shadowBlur: 1.0
            shadowHorizontalOffset: 0
            shadowVerticalOffset: 10
        }

        Rectangle {
            id: signInPanel
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
                    text: "Sign In"
                    font.pixelSize: 28
                    font.bold: true
                    color: Theme.textColor
                    Layout.alignment: Qt.AlignHCenter
                }

                Item { height: 10; Layout.fillWidth: true }

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
                    id: signInButton
                    Layout.fillWidth: true
                    Layout.preferredHeight: 45

                    enabled: !signInPageRoot.isConnecting && emailField.textValue.length > 0 && passField.textValue.length > 0

                    background: Rectangle {
                        radius: 15
                        color: {
                            if (!signInButton.enabled) return Theme.panelColor
                            if (signInButton.pressed) return Qt.darker(Theme.itemBgColor, 1.2)
                            if (signInButton.hovered) return Qt.lighter(Theme.itemBgColor, 1.1)
                            return Theme.itemBgColor
                        }
                        opacity: signInButton.enabled ? 1.0 : 0.5
                        Behavior on color { ColorAnimation { duration: 150 } }
                        Behavior on opacity { NumberAnimation { duration: 200 } }
                    }

                    contentItem: Text {
                        text: signInPageRoot.isConnecting ? "Connecting..." : "Sign In"
                        color: signInButton.enabled ? Theme.textColor : Theme.mutedTextColor
                        font.pixelSize: Theme.fontSizeHeader
                        font.bold: true
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }

                    onClicked: {
                        signInPageRoot.isConnecting = true

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
                        text: "Don't have an account?"
                        color: Theme.mutedTextColor
                        font.pixelSize: Theme.fontSizeMedium
                    }

                    Text {
                        text: "Sign Up"
                        color: Theme.iconSelectColor
                        font.pixelSize: Theme.fontSizeMedium
                        font.bold: true

                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            onClicked: signInPageRoot.switchToSignUp()
                        }
                    }
                }
            }
        }
    }

    Connections {
        target: muaBridge

        function onConnected() {
            signInPageRoot.isConnecting = false
            signInPageRoot.signInRequest(emailField.textValue)
        }

        function onErrorOccurred(message) {
            signInPageRoot.isConnecting = false
        }
    }
}
