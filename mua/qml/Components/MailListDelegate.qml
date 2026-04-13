import QtQuick
import QtQuick.Layouts
import SmtpMua

Rectangle {
    id: delegateRoot

    width: ListView.view.width
    height: 40
    radius: 20

    color: model.isSeen ? Qt.darker(Theme.mailBgColor, 1.2): Theme.mailBgColor

    opacity: model.isSeen ? 0.5 : 1.0

    MouseArea {
        anchors.fill: parent
        onClicked: delegController.handleBodyClick()
    }

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: 20
        anchors.rightMargin: 20
        spacing: 10

        Item {
            width: 20; height: 20
            SvgIcon {
                anchors.centerIn: parent
                pathData: model.isChecked ? Icons.selectCheckBox : Icons.deselCheckbox
                color: model.isChecked ? Theme.iconSelectColor : Theme.mutedTextColor
                size: 16
            }
            MouseArea {
                anchors.fill: parent
                onClicked: delegController.handleSelectClick()
            }
        }

        Item {
            width: 20; height: 20
            SvgIcon {
                anchors.centerIn: parent
                pathData: Icons.circle
                color: !model.isSeen ? Theme.iconSelectColor : Theme.mutedTextColor
                fill: !model.isSeen ? Theme.iconSelectColor : "transparent"
                strokeWidth: 3
                size: 10
            }
        }

        Item {
            width: 20;
            height: 20
            SvgIcon {
                anchors.centerIn: parent
                pathData: Icons.bookmark
                color: model.isSaved ? Theme.iconSelectColor : Theme.mutedTextColor
                fill: model.isSaved ? Theme.iconSelectColor : "transparent"
                size: 14
            }
            MouseArea {
                anchors.fill: parent
                onClicked: delegController.handleSaveClick()

            }
        }

        Item {
            width: 20; height: 20
            SvgIcon {
                anchors.centerIn: parent
                pathData: Icons.star
                color: model.isStarred ? Theme.iconSelectColor : Theme.mutedTextColor
                fill: model.isStarred ? Theme.iconSelectColor : "transparent"
                size: 16
            }
            MouseArea {
                anchors.fill: parent
                onClicked: delegController.handleStarClick()

            }
        }

        RowLayout {
            Layout.fillWidth: parent
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
    }

    Rectangle {
        width: parent.width - parent.radius * 1.4
        height: 2.5
        color: Theme.itemBorderColor
        anchors.bottom: parent.bottom
        anchors.horizontalCenter: parent.horizontalCenter
    }

    QtObject {
        id: delegController

        function handleSelectClick() {
            mockEmailModel.setProperty(model.index, "isChecked", !model.isChecked)
        }

        function handleSaveClick() {
            mockEmailModel.setProperty(model.index, "isSaved", !model.isSaved)
        }

        function handleStarClick() {
            mockEmailModel.setProperty(model.index, "isStarred", !model.isStarred)
        }

        function handleBodyClick() {
            delegateRoot.ListView.view.mailSelected(index);
            mockEmailModel.setProperty(index, "isSeen", !model.isSeen)
        }
    }
}
