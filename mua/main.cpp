#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>

#include "MuaBridge.hpp"

int main(int argc, char* argv[]) {
    QGuiApplication app(argc, argv);

    MuaBridge bridge;

    QQmlApplicationEngine engine;
    // expose bridge to all QML files
    engine.rootContext()->setContextProperty("bridge", &bridge);

    engine.load(QUrl("qrc:/SmtpMua/main.qml"));
    return app.exec();
}
