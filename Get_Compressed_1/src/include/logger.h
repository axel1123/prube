#ifndef LOGGER_H
#define LOGGER_H

#include <QObject>
#include <QFile>
#include <QDateTime>
#include <QDebug>


class Logger : public QObject
{
    Q_OBJECT
    public:
        Logger();   //Default constructor
        ~Logger();  //Destructor

    public:        
        void messageHandler(QString name, const QString &axes, double &Voltaje, const QString MAC);                
        void lineBreak();
        void Date();        

    public:
        QString m_sFileName;

    private:
        const QString m_sMainDirectory = "output";
        const QString m_sDataDirectory = "data";
        QString m_currentLogPath;
        QFile m_logFile;        
        const QString m_fileExt = QStringLiteral(".csv"); //.csv
};

#endif // LOGGER_H
