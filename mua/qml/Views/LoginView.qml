import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Effects
import SmtpMua
import "../Components"

Rectangle {
    id: loginPageRoot
    anchors.fill: parent
    color: Theme.windowBg

    // Temp
    signal loginRequest()

    property bool isConnecting: false

    Item {
        anchors.centerIn: parent
        width: 420
        height: 350

        // Shadow
        MultiEffect {
            source: loginPanel
            anchors.fill: loginPanel
            shadowEnabled: true
            shadowColor: Qt.rgba(0, 0, 0, 0.3)
            shadowBlur: 1.0
            shadowHorizontalOffset: 0
            shadowVerticalOffset: 10
        }

        // Login box
        Rectangle {
            id: loginPanel
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
                    text: "SMTP Client"
                    font.pixelSize: 28
                    font.bold: true
                    color: Theme.textColor
                    Layout.alignment: Qt.AlignHCenter
                }

                Item { height: 10; Layout.fillWidth: true }

                // Fields
                StyledInput {
                    id: loginField
                    Layout.fillWidth: true
                    label: "Login:"
                    placeholder: "user@example.com"

                    onTextValueChanged: statusLabel.text = ""
                }

                StyledInput {
                    id: passField
                    Layout.fillWidth: true
                    label: "Password:"
                    placeholder: "Password"
                    echoMode: TextInput.Password

                    onTextValueChanged: statusLabel.text = ""
                }

                Item { Layout.fillHeight: true }

                // Connect btn
                Button {
                    id: connectButton
                    Layout.fillWidth: true
                    Layout.preferredHeight: 45

                    enabled: !loginPageRoot.isConnecting && loginField.textValue.length > 0 && passField.textValue.length > 0

                    background: Rectangle {
                        radius: 15

                        color: {
                            if (!connectButton.enabled) return Theme.panelColor
                            if (connectButton.pressed) return Qt.darker(Theme.itemBgColor, 1.2)
                            if (connectButton.hovered) return Qt.lighter(Theme.itemBgColor, 1.1)
                            return Theme.itemBgColor
                        }

                        opacity: connectButton.enabled ? 1.0 : 0.5

                        Behavior on color { ColorAnimation { duration: 150 } }
                        Behavior on opacity { NumberAnimation { duration: 200 } }
                    }

                    contentItem: Text {
                        text: loginPageRoot.isConnecting ? "Connecting..." : "Connect"
                        color: connectButton.enabled ? Theme.textColor : Theme.mutedTextColor
                        font.pixelSize: Theme.fontSizeHeader
                        font.bold: true
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }

                    onClicked: {
                        loginPageRoot.isConnecting = true
                        statusLabel.text = "Establishing connection..."
                        statusLabel.color = Theme.mutedTextColor

                        loginRequest();

                        //// For backend
                        // Bridge.connectToServer(
                        //     loginField.textValue,
                        //     passField.textValue
                        // )
                    }
                }

                // Loading indicator & Error message
                RowLayout {
                    Layout.alignment: Qt.AlignHCenter
                    Layout.fillWidth: true
                    spacing: 10

                    visible: loginPageRoot.isConnecting || statusLabel.text !== ""

                    BusyIndicator {
                        running: loginPageRoot.isConnecting
                        visible: loginPageRoot.isConnecting
                        Layout.preferredHeight: 24
                        Layout.preferredWidth: 24
                    }

                    Text {
                        id: statusLabel
                        text: ""
                        color: Theme.mutedTextColor
                        font.pixelSize: 13

                        wrapMode: Text.WordWrap
                        Layout.maximumWidth: 300
                        horizontalAlignment: Text.AlignHCenter
                    }
                }
            }
        }
    }

    //// For backend
    // Connections {
    //     target: Bridge
    //
    //     function onErrorOccurred(message) {
    //         loginPageRoot.isConnecting = false
    //         statusLabel.text = "Connection failed: " + message
    //         statusLabel.color = "#EF4444"
    //     }
    // }
}
