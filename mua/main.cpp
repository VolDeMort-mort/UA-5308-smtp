#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>

#include "bridge/MuaBridge.hpp"

int main(int argc, char* argv[]) {
    QGuiApplication app(argc, argv);

    MuaBridge bridge;

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty("muaBridge", &bridge);
    engine.load(QUrl("qrc:/SmtpMua/main.qml"));
    return app.exec();
}
