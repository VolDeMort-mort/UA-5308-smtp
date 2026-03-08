import QtQuick
import QtQuick.Layouts
import QtQuick.Controls

Button {
    id: control
    property alias iconRow: contentRow

    leftPadding: 12
    rightPadding: 12
    topPadding: 0
    bottomPadding: 0


    background: Rectangle {
        implicitHeight: 32
        radius: 16

        color: control.pressed ? Qt.darker(itemBgColor, 1.2) :
               (control.hovered ? Qt.alpha("#FFFFFF", 0.05) : itemBgColor)

        border.color: control.hovered ? Qt.alpha("#FFFFFF", 0.2) : itemBorderColor
        border.width: 1
        clip: true
    }

    contentItem: RowLayout {
        id: contentRow
        spacing: 8
    }
}
