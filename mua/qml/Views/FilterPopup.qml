// FilterPopup.qml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../Components"
import SmtpMua

Popup {
    id: filterPopup

    width: 280          // FIX
    height: 400
    padding: 15

    modal: true
    dim: true
    focus: true

    // Darker bg
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
            property: "x"
            from: filterPopup.x + 15
            to: filterPopup.x
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

    // Mail layout
    ColumnLayout {
        anchors.fill: parent
        spacing: 5

        // Status checkboxes
        StyledCheckBox { text: "Seen" }
        StyledCheckBox { text: "Unseen" }
        StyledCheckBox { text: "Attachments" }

        // Spacer
        Rectangle {
            Layout.fillWidth: true; Layout.preferredHeight: 1
            color: Theme.mutedTextColor
            Layout.topMargin: 5; Layout.bottomMargin: 5
        }

        // Auto dates checkboxes
        StyledCheckBox { text: "Today" }
        StyledCheckBox { text: "This week" }
        StyledCheckBox { text: "This month" }
        StyledCheckBox { text: "This year" }

        Item { Layout.fillHeight: true }

        // Manual dates fields
        DateInputRow { label: "From:"; placeholder: "dd.mm.yy hh:mm" }
        DateInputRow { label: "To:"; placeholder: "dd.mm.yy hh:mm" }
    }

    // Date field
    component DateInputRow: RowLayout {
        property string label
        property string placeholder

        spacing: 10

        // Label
        Text {
            text: label
            color: Theme.mutedTextColor
            font.pixelSize: Theme.fontSizeMedium
            Layout.preferredWidth: 40
        }

        // Input rect
        Rectangle {
            Layout.fillWidth: true;
            height: 32
            radius: 16
            color: Qt.alpha("white", 0.05)
            border.color: Theme.itemBorderColor

            TextInput {
                anchors.fill: parent
                anchors.leftMargin: 12
                verticalAlignment: TextInput.AlignVCenter
                text: placeholder
                color: Theme.mutedTextColor
                font.pixelSize: Theme.fontSizeMedium
            }
        }
    }
}
