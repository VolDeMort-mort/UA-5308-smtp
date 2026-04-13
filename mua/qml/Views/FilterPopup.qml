import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../Components"
import SmtpMua

Popup {
    id: filterPopup

    width: 280
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
        StyledCheckBox {
            text: "Seen"
            checked: filteredModel.seenOnly
            onCheckedChanged: filteredModel.seenOnly = checked
        }
        StyledCheckBox {
            text: "Unseen"
            checked: filteredModel.unseenOnly
            onCheckedChanged: filteredModel.unseenOnly = checked
        }
        StyledCheckBox {
            text: "Attachments"
            checked: filteredModel.attachmentsOnly
            onCheckedChanged: filteredModel.attachmentsOnly = checked
        }

        // Spacer
        Rectangle {
            Layout.fillWidth: true; Layout.preferredHeight: 1
            color: Theme.mutedTextColor
            Layout.topMargin: 5; Layout.bottomMargin: 5
        }

        // Auto dates checkboxes 
        ButtonGroup { id: dateGroup }

        StyledCheckBox {
            text: "Today"
            ButtonGroup.group: dateGroup
            checked: filteredModel.dateFilter === "today"
            onCheckedChanged: if(checked) filteredModel.dateFilter = "today"
        }
        StyledCheckBox {
            text: "This week"
            ButtonGroup.group: dateGroup
            checked: filteredModel.dateFilter === "week"
            onCheckedChanged: if(checked) filteredModel.dateFilter = "week"
        }
        StyledCheckBox {
            text: "This month"
            ButtonGroup.group: dateGroup
            checked: filteredModel.dateFilter === "month"
            onCheckedChanged: if(checked) filteredModel.dateFilter = "month"
        }
        StyledCheckBox {
            text: "This year"
            ButtonGroup.group: dateGroup
            checked: filteredModel.dateFilter === "year"
            onCheckedChanged: if(checked) filteredModel.dateFilter = "year"
        }

        Item { Layout.fillHeight: true }

        // Manual dates fields
        DateInputRow {
            label: "From:"
            placeholder: "dd.mm.yy hh:mm"
            onDateTextChanged: (newText) => filteredModel.dateFrom = newText
        }
        DateInputRow {
            label: "To:"
            placeholder: "dd.mm.yy hh:mm"
            onDateTextChanged: (newText) => filteredModel.dateTo = newText
        }
    }

    // Date field
    component DateInputRow: RowLayout {
        property string label
        property string placeholder

        signal dateTextChanged(string newText)

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

                onTextChanged: {
                    if (text !== placeholder) {
                        dateTextChanged(text)
                    }
                }
            }
        }
    }
}
