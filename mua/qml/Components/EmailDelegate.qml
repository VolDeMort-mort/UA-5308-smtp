import QtQuick
import QtQuick.Layouts
import SmtpMua

Rectangle {
    id: delegateRoot
    width: ListView.view.width
    height: 44
    radius: 20

    color: itemBgColor
    border.color: model.isChecked ? "#F59E0B" : itemBorderColor
    border.width: model.isChecked ? 1.5 : 1

    // Main layout
    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: 10
        anchors.rightMargin: 10
        spacing: 10

        // Selected icon
        Item {
            width: 20; height: 20
            SvgIcon {
                anchors.centerIn: parent
                pathData: model.isChecked ? "M19 3H5a2 2 0 0 0-2 2v14a2 2 0 0 0 2 2h14a2 2 0 0 0 2-2V5a2 2 0 0 0-2-2m-9 14l-5-5 1.41-1.41L10 14.17l7.59-7.59L19 8l-9 9z" : "M3 3h18v18H3z"
                color: model.isChecked ? "#F59E0B" : mutedTextColor
                size: 16
            }
            MouseArea {
                anchors.fill: parent
                onClicked:
                    mockEmailModel.setProperty(index, "isChecked", !model.isChecked)
            }
        }

        // Opened icon
        Item {
            width: 20; height: 20
            SvgIcon {
                anchors.centerIn: parent
                pathData: "M 12, 2 a 10, 10 0 1, 1 0, 20 a 10, 10 0 1, 1 0, -20"
                color: model.isSeen ? "#F59E0B" : mutedTextColor
                fill: model.isSeen ? "#F59E0B" : "transparent"
                strokeWidth: 3
                size: 10
            }
            MouseArea {
                anchors.fill: parent
                onClicked:
                    mockEmailModel.setProperty(index, "isSeen", !model.isSeen)
            }
        }

        // Saved icon
        Item {
            width: 20;
            height: 20
            SvgIcon {
                anchors.centerIn: parent
                pathData: "M19 21l-7-5-7 5V5a2 2 0 0 1 2-2h10a2 2 0 0 1 2 2z"
                color: model.isSaved ? "#F59E0B" : Theme.mutedTextColor
                fill: model.isSaved ? "#F59E0B" : "transparent"
                size: 14
            }
            MouseArea {
                anchors.fill: parent
                onClicked:
                    mockEmailModel.setProperty(index, "isSaved", !model.isSaved)
            }
        }

        // Starred icon
        Item {
            width: 20; height: 20
            SvgIcon {
                anchors.centerIn: parent
                pathData: "M12 2l3.09 6.26L22 9.27l-5 4.87 1.18 6.88L12 17.77l-6.18 3.25L7 14.14 2 9.27l6.91-1.01L12 2z"
                color: model.isStarred ? "#F59E0B" : mutedTextColor
                fill: model.isStarred ? "#F59E0B" : "transparent"
                size: 16
            }
            MouseArea {
                anchors.fill: parent
                onClicked:
                    mockEmailModel.setProperty(index, "isStarred", !model.isStarred)
            }
        }

        // Mail info
        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true


            RowLayout {
                anchors.fill: parent
                spacing: 15

                Text {
                    Layout.fillWidth: true
                    Layout.minimumWidth: 80
                    Layout.maximumWidth: 150
                    text: model.receiver
                    color: Theme.textColor
                    font.bold: !model.isSeen
                    font.pixelSize: 14
                    elide: Text.ElideRight
                }

                Item{
                    Layout.fillWidth: true
                }

                Text {
                    Layout.fillWidth: true
                    Layout.preferredWidth: 80
                    text: model.subject
                    color: Theme.textColor
                    font.bold: !model.isSeen
                    font.pixelSize: 14
                    elide: Text.ElideRight
                }

                Text {
                    Layout.fillWidth: true
                    Layout.preferredWidth: 100
                    text: model.body
                    color: Theme.mutedTextColor
                    font.pixelSize: 14
                    elide: Text.ElideRight
                }

                Text {
                    Layout.alignment: Qt.AlignRight
                    text: model.createdAt
                    color: Theme.mutedTextColor
                    font.pixelSize: 12
                }            }

            MouseArea {
                anchors.fill: parent
                onClicked: {
                    delegateRoot.ListView.view.currentIndex = index
                }
            }
        }
    }



    MouseArea {
        anchors.fill: parent
        onClicked:{
            delegateRoot.ListView.view.currentIndex = index
            readMail.msgData = mockEmailModel.get(index)
            readMail.open()
        }
    }
}
