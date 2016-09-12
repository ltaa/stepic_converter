#include <QGuiApplication>
#include <QApplication>
#include <QtQuickWidgets>
#include "iodata.h"

int main(int argc, char *argv[])
{

    QApplication app(argc, argv);
    QQmlApplicationEngine engine;

    engine.load(QUrl(QStringLiteral("qrc:/main.qml")));

    QQmlContext *rootContext =  engine.rootContext();
    rootContext->setContextProperty("iodata", new IOData());

    return app.exec();
}
