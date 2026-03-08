import QtQuick
import QtQuick.Layouts
import "../Components"
import SmtpMua

Rectangle {
    id: toolbarBackground
    Layout.fillWidth: true
    Layout.preferredHeight: 56

    color: sidebarBg
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
                SvgIcon { pathData: "M3 3h18v18H3z"; color: mutedTextColor; size: 14 },
                SvgIcon { pathData: "M6 9l6 6 6-6"; color: mutedTextColor; size: 12 }
            ]
            onClicked: {
                allSelected = !allSelected
                for (var i = 0; i < emailModel.count; ++i) {
                    emailModel.setProperty(i, "checked", allSelected)
                }
            }
        }

        // Refresh btn
        ActionBtn {
            implicitWidth: 40
            iconRow.children: [
                SvgIcon { pathData: "M23 4v6h-6 M20.49 15a9 9 0 1 1-2.12-9.36L23 10"; color: mutedTextColor; size: 14 }
            ]
        }

        // Spacer
        Text { text: "|"; color: itemBorderColor; font.pixelSize: 16; Layout.margins: 4 }


        // Forward to btn
        ActionBtn {
            implicitWidth: 95
            iconRow.children: [
                SvgIcon { pathData: "M15 14l5-5-5-5 M20 9H9a5 5 0 0 0 0 10h3"; color: mutedTextColor; size: 14 },
                Text { text: "Forward"; color: mutedTextColor; font.pixelSize: 13 }
            ]
        }

        // Move to btn
        ActionBtn {
            implicitWidth: 105
            iconRow.children: [
                SvgIcon { pathData: "M22 19a2 2 0 0 1-2 2H4a2 2 0 0 1-2-2V5a2 2 0 0 1 2-2h5l2 3h9a2 2 0 0 1 2 2z"; color: mutedTextColor; size: 14 },
                Text { text: "Move to"; color: mutedTextColor; font.pixelSize: 13 }
            ]
        }

        // Spam btn
        ActionBtn {
            implicitWidth: 85
            iconRow.children: [
                SvgIcon { pathData: "M10.29 3.86L1.82 18a2 2 0 0 0 1.71 3h16.94a2 2 0 0 0 1.71-3L13.71 3.86a2 2 0 0 0-3.42 0z M12 9v4 M12 17h.01"; color: mutedTextColor; size: 14 },
                Text { text: "Spam"; color: mutedTextColor; font.pixelSize: 13 }
            ]
        }

        // Delete btn
        ActionBtn {
            implicitWidth: 90
            iconRow.children: [
                SvgIcon { pathData: "M3 6h18 M19 6v14a2 2 0 0 1-2 2H7a2 2 0 0 1-2-2V6m3 0V4a2 2 0 0 1 2-2h4a2 2 0 0 1 2 2v2"; color: mutedTextColor; size: 14 },
                Text { text: "Delete"; color: mutedTextColor; font.pixelSize: 18 }
            ]
        }

        // ... btn
        ActionBtn {
            implicitWidth: 40
            iconRow.children: [ SvgIcon { pathData: "M12 12h.01 M19 12h.01 M5 12h.01"; color: mutedTextColor; size: 16 } ]
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
                    font.pixelSize: 14
                }

                SvgIcon {
                    pathData: "M6 9l6 6 6-6"
                    color: Theme.textColor
                    size: 14
                }
            }

            MouseArea {
                anchors.fill: parent
                cursorShape: Qt.PointingHandCursor
                hoverEnabled: true

                onClicked: {
                    // Твоя логіка позиціонування залишається такою самою
                    let globalPos = allButton.mapToItem(mainWindow.contentItem, 0, allButton.height + 5)
                    mainFilterPopup.x = globalPos.x - mainFilterPopup.width * 0.8
                    mainFilterPopup.y = globalPos.y
                    mainFilterPopup.open()
                }
            }
        }
    }
}
