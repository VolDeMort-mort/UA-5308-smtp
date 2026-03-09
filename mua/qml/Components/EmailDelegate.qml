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
                pathData: model.isChecked ? Icons.selectCheckbox : Icons.deselCheckbox
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
                pathData: Icons.circle
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
                pathData: Icons.bookmark
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
                pathData: Icons.star
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
                    font.pixelSize: Theme.fontSizeMedium
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
                    font.pixelSize: Theme.fontSizeMedium
                    elide: Text.ElideRight
                }

                Text {
                    Layout.fillWidth: true
                    Layout.preferredWidth: 100
                    text: model.body
                    color: Theme.mutedTextColor
                    font.pixelSize: Theme.fontSizeMedium
                    elide: Text.ElideRight
                }

                Text {
                    Layout.alignment: Qt.AlignRight
                    text: model.createdAt
                    color: Theme.mutedTextColor
                    font.pixelSize: Theme.fontSizeSmall
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
