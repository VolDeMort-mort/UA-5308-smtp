import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../Components"
import SmtpMua

Rectangle {
    id: toolbarBackground
    Layout.fillWidth: true
    Layout.preferredHeight: 56

    color: Theme.sidebarBg
    radius: 20

    property bool allSelected: false
    property real w: toolbarBackground.width

    RowLayout {
        id: toolbarRoot
        anchors.fill: parent
        anchors.leftMargin: 30
        anchors.rightMargin: 30
        spacing: 8

        // Select all btn
        ActionBtn {
            implicitWidth: 50
            iconRow.children: [
                SvgIcon { pathData: Icons.deselCheckbox; color: Theme.textColor; size: 14 },
                SvgIcon { pathData: Icons.downArrow; color: Theme.textColor; size: 12 }
            ]
            onClicked: {
                allSelected = !allSelected
                for (var i = 0; i < mockEmailModel.count; ++i) {
                    mockEmailModel.setProperty(i, "isChecked", allSelected)
                }
            }
        }

        // Refresh btn
        ActionBtn {
            implicitWidth: 40
            iconRow.children: [ SvgIcon { pathData: Icons.refresh; color: Theme.textColor; size: 14 } ]
            onClicked: {
                if (typeof muaBridge !== "undefined") {
                    muaBridge.fetchMails("INBOX", 0, 50)
                }
            }
        }

        // Spacer
        Text {
            text: "|"
            color: Theme.textColor
            font.pixelSize: Theme.fontSizeMedium
            Layout.margins: 4
            opacity: w > 350 ? 1.0 : 0.0
            visible: opacity > 0.0
        }

        // Forward to btn
        ActionBtn {
            implicitWidth: 95
            opacity: w > 350 ? 1.0 : 0.0
            visible: opacity > 0.0

            iconRow.children: [
                SvgIcon { pathData: Icons.moveTo; color: Theme.textColor; size: 14 },
                Text { text: "Forward"; color: Theme.textColor; font.pixelSize: Theme.fontSizeMedium }
            ]

            onClicked: {
                let selected = getSelectedMails()
                if (selected.length > 0) {
                    composePopup.openForward(selected[0])
                }
            }
        }

        // Move to btn
        ActionBtn {
            id: moveToBtn
            implicitWidth: 105
            opacity: w > 450 ? 1.0 : 0.0
            visible: opacity > 0.0

            iconRow.children: [
                SvgIcon { pathData: Icons.folder; color: Theme.textColor; size: 14 },
                Text { text: "Move to"; color: Theme.textColor; font.pixelSize: Theme.fontSizeMedium }
            ]

            onClicked: moveMenu.open()

            Menu {
                id: moveMenu
                y: moveToBtn.height

                background: Rectangle {
                    color: Theme.panelColor
                    radius: 8
                    border.color: Theme.itemBorderColor
                }

                Instantiator {
                    model: folModel
                    MenuItem {
                        text: model.name
                        contentItem: Text {
                            text: parent.text
                            color: Theme.textColor
                            font.pixelSize: Theme.fontSizeMedium
                        }
                        background: Rectangle {
                            color: parent.highlighted ? Qt.alpha(Theme.iconSelectColor, 0.2) : "transparent"
                        }
                        onTriggered: {
                            let selected = getSelectedMails()
                            selected.forEach(mail => {
                                muaBridge.moveMail(mail.id_msg || mail.mailId, model.name)
                            })
                            removeSelectedFromUI()
                        }
                    }
                }
            }
        }

        // Spam btn
        ActionBtn {
            implicitWidth: 85
            opacity: w > 550 ? 1.0 : 0.0
            visible: opacity > 0.0

            iconRow.children: [
                SvgIcon { pathData: Icons.spam; color: Theme.textColor; size: 14 },
                Text { text: "Spam"; color: Theme.textColor; font.pixelSize: Theme.fontSizeMedium }
            ]

            onClicked: {
                let selected = getSelectedMails()
                selected.forEach(mail => {
                    muaBridge.moveMail(mail.id_msg || mail.mailId, "Spam")
                })
                removeSelectedFromUI()
            }
        }

        // Delete btn
        ActionBtn {
            implicitWidth: 90
            opacity: w > 650 ? 1.0 : 0.0
            visible: opacity > 0.0

            iconRow.children: [
                SvgIcon { pathData: Icons.trash; color: Theme.textColor; size: 14 },
                Text { text: "Delete"; color: Theme.textColor; font.pixelSize: Theme.fontSizeMedium }
            ]

            onClicked: {
                let selected = getSelectedMails()
                selected.forEach(mail => {
                    muaBridge.deleteMail(mail.id_msg || mail.mailId)
                })
                removeSelectedFromUI()
            }
        }

        // ... btn
        ActionBtn {
            implicitWidth: 40
            iconRow.children: [ SvgIcon { pathData: Icons.etc; color: Theme.mutedTextColor; size: 16 } ]
        }

        Item { Layout.fillWidth: true }

        // Filters AllButton
        Item {
            id: allButton
            implicitWidth: allButtonLayout.implicitWidth
            implicitHeight: allButtonLayout.implicitHeight

            RowLayout {
                id: allButtonLayout
                anchors.fill: parent
                spacing: 4
                Text { text: "All"; color: Theme.textColor; font.pixelSize: Theme.fontSizeMedium }
                SvgIcon { pathData: Icons.downArrow; color: Theme.textColor; size: 14 }
            }

            MouseArea {
                anchors.fill: parent
                cursorShape: Qt.PointingHandCursor
                hoverEnabled: true

                onClicked: {
                    let globalPos = allButton.mapToItem(rootWindow.contentItem, 0, allButton.height + 5)
                    mainFilterPopup.x = globalPos.x - mainFilterPopup.width * 0.8
                    mainFilterPopup.y = globalPos.y
                    mainFilterPopup.open()
                }
            }
        }
    }

    function getSelectedMails() {
        let selected = []
        for (let i = 0; i < mockEmailModel.count; ++i) {
            let mail = mockEmailModel.get(i)
            if (mail.isChecked) {
                selected.push(mail)
            }
        }
        if (selected.length === 0 && typeof areaContainer !== "undefined" && areaContainer.selectedMailData) {
            selected.push(areaContainer.selectedMailData)
        }
        return selected
    }

    function removeSelectedFromUI() {
        for (let i = mockEmailModel.count - 1; i >= 0; --i) {
            if (mockEmailModel.get(i).isChecked) {
                mockEmailModel.remove(i)
            }
        }
        if (typeof mainController !== "undefined") {
            mainController.changeState("LISTSTATE")
        }
        allSelected = false
    }
}
