import QtQuick
import QtQuick.Shapes

Item {
    id: iconRoot
    property string pathData: ""
    property color color: "transparent"
    property color fill: "transparent"
    property int size: 18
    property real strokeWidth: 3

    width: size
    height: size

    layer.enabled: true
    layer.samples: 4

    Shape {
        id: shape

        anchors.centerIn: parent
        width: 24
        height: 24


        scale: iconRoot.size / 24


        preferredRendererType: Shape.GeometryRenderer
        antialiasing: true

        ShapePath {
            id: sPath
            fillColor: iconRoot.fill
            strokeColor: iconRoot.color
            strokeWidth: iconRoot.strokeWidth
            joinStyle: ShapePath.RoundJoin
            capStyle: ShapePath.RoundCap


            Behavior on strokeColor { ColorAnimation { duration: 120 } }
            Behavior on fillColor { ColorAnimation { duration: 120 } }

            PathSvg {
                path: iconRoot.pathData
            }
        }
    }
}
