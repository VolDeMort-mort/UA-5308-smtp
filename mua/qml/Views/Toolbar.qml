import QtQuick
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
                SvgIcon {
                    pathData: Icons.deselCheckbox
                    color: Theme.textColor
                    size: 14 },
                SvgIcon {
                    pathData: Icons.downArrow
                    color: Theme.textColor
                    size: 12 }
            ]
            onClicked: {
                allSelected = !allSelected
                for (var i = 0; i < mockEmailModel.count; ++i) {
                    mockEmailModel.setProperty(i, "checked", allSelected)
                }
            }
        }

        // Refresh btn
        ActionBtn {
            implicitWidth: 40
            iconRow.children: [
                SvgIcon {
                    pathData: Icons.refresh
                    color: Theme.textColor
                    size: 14 }
            ]
        }

        // Spacer
        Text {
            text: "|";
            color: Theme.textColor;
            font.pixelSize: Theme.fontSizeMedium;
            Layout.margins: 4 }


        // Forward to btn
        ActionBtn {
            implicitWidth: 95
            iconRow.children: [
                SvgIcon { pathData: Icons.moveTo; color: Theme.textColor; size: 14 },
                Text { text: "Forward"; color: Theme.textColor; font.pixelSize: Theme.fontSizeMedium }
            ]
        }

        // Move to btn
        ActionBtn {
            implicitWidth: 105
            iconRow.children: [
                SvgIcon { pathData: Icons.folder; color: Theme.textColor; size: 14 },
                Text { text: "Move to"; color: Theme.textColor; font.pixelSize: Theme.fontSizeMedium }
            ]
        }

        // Spam btn
        ActionBtn {
            implicitWidth: 85
            iconRow.children: [
                SvgIcon { pathData: Icons.spam; color: Theme.textColor; size: 14 },
                Text { text: "Spam"; color: Theme.textColor; font.pixelSize: Theme.fontSizeMedium }
            ]
        }

        // Delete btn
        ActionBtn {
            implicitWidth: 90
            iconRow.children: [
                SvgIcon { pathData: Icons.trash; color: Theme.textColor; size: 14 },
                Text { text: "Delete"; color: Theme.textColor; font.pixelSize: Theme.fontSizeMedium }
            ]
        }

        // ... btn
        ActionBtn {
            implicitWidth: 40
            iconRow.children: [ SvgIcon { pathData: Icons.etc; color: Theme.mutedTextColor; size: 16 } ]
        }

        // Spacer
        Item { Layout.fillWidth: true }

        // Filters
        Item {
            id: allButton
            implicitWidth: allButtonLayout.implicitWidth
            implicitHeight: allButtonLayout.implicitHeight

            RowLayout {
                id: allButtonLayout
                anchors.fill: parent
                spacing: 4

                Text {
                    text: "All"
                    color: Theme.textColor
                    font.pixelSize: Theme.fontSizeMedium
                }

                SvgIcon {
                    pathData: Icons.downArrow
                    color: Theme.textColor
                    size: 14
                }
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
}
