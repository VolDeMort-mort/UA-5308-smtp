#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include "bridge/MuaBridge.hpp"
#include <QQuickStyle>

int main(int argc, char* argv[]) {
    QGuiApplication app(argc, argv);
    QQmlApplicationEngine engine;

	QQuickStyle::setStyle("Basic");

	MuaBridge muaBridge;

	QQmlContext* context = engine.rootContext();
	context->setContextProperty("muaBridge", &muaBridge);

	const QUrl url(u"qrc:/SmtpMua/qml/main.qml"_qs);
	QObject::connect(&engine, &QQmlApplicationEngine::objectCreated,
					 &app, [url](QObject *obj, const QUrl &objUrl) {
						 if (!obj && url == objUrl)
							 QCoreApplication::exit(-1);
					 }, Qt::QueuedConnection);

	engine.load(url);

    return app.exec();
}
