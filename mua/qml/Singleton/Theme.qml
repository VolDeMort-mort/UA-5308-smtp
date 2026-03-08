pragma Singleton
import QtQuick
import "ThemeData.js" as Data

QtObject {
    id: root

    property string themeName: "DarkBlue"

    readonly property var colors: Data.themes[themeName]

    Component.onCompleted: {
        console.log("Available themes:", Object.keys(Data.themes))
    }

    readonly property color windowBg: colors.windowBg
    readonly property color sidebarBg: colors.barBg
    readonly property color panelColor: colors.panelColor
    readonly property color itemBgColor: colors.itemBgColor
    readonly property color itemBorderColor: colors.itemBorderColor
    readonly property color textColor: colors.textColor
    readonly property color mutedTextColor: colors.mutedTextColor
    readonly property color unreadColor: colors.unreadColor
}
