#include "include/logger.h"
#include <QDir>

Logger::Logger()
{

}

Logger::~Logger()
{

}

void Logger::messageHandler(QString name, const QString &axes, double &Voltaje, const QString MAC)
{
    QString m_sDate = QDateTime::currentDateTime().toString("yyyyMMMM");//year month
    QString m_sDate2 = QDateTime::currentDateTime().toString("yyyyMMdd");

    QString m_sRut = m_sDate.mid(0,4) + "/" + m_sDate.mid(4);
    QString m_sRutData = m_sMainDirectory + "/" + m_sRut + "/" + m_sDataDirectory + "/" + m_sDate2;

    if(!QDir(m_sRutData).exists()){
        QDir().mkpath(m_sRutData);
        qDebug() << "The Dir" << m_sRutData << " was created";
    }

    m_currentLogPath = m_sRutData + '/' + name + m_fileExt;
    m_logFile.setFileName(m_currentLogPath);

    if (!m_logFile.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Append)){
        qDebug() << "Could not open log file for writing!";
        return;
    }

    QTextStream out(&m_logFile);
    QString str;
    if(m_logFile.pos() == 0){ // if the file is empty
        str = QString("%1,%2,%3,%4,%5,%6\n")
                .arg("DATE","AX","AY","AZ","VOLTAGE","MAC");
        out << str;
        qDebug() << "The File" << m_logFile << " was created";
    }

    QString date = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz");

    str.clear();
    str = QString("%1," )
            .arg(date);
    out << str;

    str.clear();
    str = QString("%1,%2,%3\n")
            .arg(axes)
            .arg(Voltaje)
            .arg(MAC);
    out << str;

    out.flush();        // Clear the buffered data
    m_logFile.close();  //Close the field
}

void Logger::lineBreak()
{

    //QFile file("new2.txt");
    QFile file(m_sFileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Append)){
        return;
    }
    QTextStream out(&file);
    out << "\n";
}

void Logger::Date()
{
    QFile file("new2.txt");
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Append)){
        return;
    }
    QTextStream out(&file);
    out << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz");
}
