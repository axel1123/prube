/*!
  Interface
*-------------------------------------------------
* E2 Innovation
* @authors: Axel Martinez
* @authors: Eduardo Zarate
* @authors: Brayan Espinoza
*-------------------------------------------------
*/
#include <QCoreApplication>
#include <QDateTime>

#include "include/hander.h"
#include "mlog.h"
#include "mlogtypes.h"

#include <QSettings>

Q_LOGGING_CATEGORY(coreMain, "core.main")

int main(int argc, char *argv[])
{

    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    // Create MiloLog instance before Q*Application to capture all messages it
    // generates
    MLog::instance();
    QCoreApplication app(argc, argv);

    //configure the .ini file y its directory

    //app.setWindowIcon(QIcon(":/files/images/logo_48x48.png"));
    app.setOrganizationName("E2 Innovation");
    app.setApplicationName("Interface Sensor GETs Desktop Application");

    //prube level logging
    logger() -> setLogLevel(MLog::DebugLog);
    // Enable the logging
    QString date = QDateTime::currentDateTime().toString("mm");
    logger()->enableLogToFile(date);
    //logger() -> enableLogToFile(app.applicationName());

    qCInfo(coreMain) << "------------------------------";
    qCInfo(coreMain) << "The Program Started";
    qCInfo(coreMain) << "------------------------------";

    qDebug() << "abc";
    Hander routine;
    //como se va saber a que puerto se debe conectar?, con la info que linux ?
    routine.init("COM12");
    //routine.scanGETS();

    //routine.saveSettings();
    //routine.loadSettings(true);
    routine.sendSleepCommand();

    //cuando nose detecte un get, emitir una senal , por ahora un print();

    //xD.resetScan();
    //hander.init();

    return app.exec();
}
