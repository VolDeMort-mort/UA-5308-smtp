#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQuickStyle>

int main(int argc, char* argv[]) {
    QGuiApplication app(argc, argv);
    QQmlApplicationEngine engine;
	QQuickStyle::setStyle("Basic");

	// MuaBridge muaBridge;

	// QQmlContext* context = engine.rootContext();
	// context->setContextProperty("muaBridge", &muaBridge);

	// QObject::connect(&engine, &QQmlApplicationEngine::objectCreated,
	// 				 &app, [url](QObject *obj, const QUrl &objUrl) {
	// 					 if (!obj && url == objUrl)
	// 						 QCoreApplication::exit(-1);
	// 				 }, Qt::QueuedConnection);

	engine.load(QUrl("qrc:/SmtpMua/qml/main.qml"));

    return app.exec();
}
