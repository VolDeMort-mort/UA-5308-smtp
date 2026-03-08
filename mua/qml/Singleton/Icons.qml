pragma Singleton
import QtQuick

QtObject {
    // Навігація
    readonly property string inbox: "M22 12h-6l-2 3h-4l-2-3H2 v8a2 2 0 0 0 2 2h16a2 2 0 0 0 2-2z..."
    readonly property string sent: "M22 2L11 13M22 2l-7 20-4-9-9-4 20-7z"
    readonly property string trash: "M3 6h18 M19 6v14a2 2 0 0 1-2 2H7a2 2 0 0 1-2-2V6m3 0V4a2 2 0 0 1 2-2h4a2 2 0 0 1 2 2v2"

    // Дії
    readonly property string plus: "M12 5v14 M5 12h14"
    readonly property string settings: "M12 15a3 3 0 1 0 0-6 3 3 0 0 0 0 6z..."
    readonly property string search: "M11 19a8 8 0 1 0 0-16 8 8 0 0 0 0 16z M21 21l-4.35-4.35"

    // Папки
    readonly property string folder: "M22 19a2 2 0 0 1-2 2H4a2 2 0 0 1-2-2V5a2 2 0 0 1 2-2h5l2 3h9a2 2 0 0 1 2 2z"
}
