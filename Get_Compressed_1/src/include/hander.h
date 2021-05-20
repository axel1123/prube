#ifndef HANDER_H
#define HANDER_H

#include <QObject>
//#include <QCloseEvent>
#include <QMap>

#include "include/api.h"
#include "include/xbeescan.h"
#include "include/logger.h"

#include <QSettings>
#include <QDateTime>
#include <QEventLoop>  //for the sleep
#include "mlog.h"
#include "mlogtypes.h"

#include <QDebug>

class Hander : public QObject
{
    Q_OBJECT
//    Q_PROPERTY(QString m_Payload READ getLocalPayload NOTIFY payloadChanged)
//    //Q_PROPERTY(QString m_Sid READ getID NOTIFY idChanged)
//    Q_PROPERTY(float voltage READ voltage NOTIFY dataChanged)
//    Q_PROPERTY(QString mac READ mac NOTIFY dataChanged)
//    Q_PROPERTY(QString accData READ accData NOTIFY dataChanged)
//    Q_PROPERTY(QString check2 READ getCheck NOTIFY change_check)
//    //Q_PROPERTY(QString check2Final READ getCheckFinal NOTIFY change_check2)
//    Q_PROPERTY(QString check2Final READ getCheckFinal NOTIFY change_getState)
//    Q_PROPERTY(QString system_state READ read_systemstate NOTIFY change_sleeping)
//    //Q_PROPERTY(bool check2 READ getCheck NOTIFY change_check)
//    Q_PROPERTY(bool status READ status WRITE setStatus NOTIFY statusChanged)
//    Q_PROPERTY(float progressBarCounter READ getProgressBarCounter NOTIFY updateProgressBar)
//    Q_PROPERTY(int currentGet READ currentGet NOTIFY currentGetChanged)

//    Q_PROPERTY(QVariant listPort READ listPort NOTIFY listPortChanged)
//    Q_PROPERTY(QString devicesIni READ DevicesIni WRITE setDevicesIni)
//    Q_PROPERTY(QString attemptIni READ AttemptIni WRITE setAttemptIni)
//    //Q_PROPERTY(QString instalationIni READ installIni WRITE setInstallIni)

    public:
        Hander();
        ~Hander();
        //void closeEvent(QCloseEvent *event);

    public:
        //---------------Q_INVOKABLE--------------------------------
        void init(QString port);
        //         QString getMacID();
        //         float getVoltaje();
        //         QString getAccData();
        float getProgressBarCounter();
        void setData( int i);
        QString getLocalPayload();
        QString getCheck();
        QString getCheckFinal();
        void scanGETS();
        QList<QString> name_ports();
        void list_ports();
        void close_port();
        void sendSleepCommand();
        void sendResetCommand();
        void prube();
        void prube1();
        QString setCheck(int i);
        QString setCheckFinal(int i);
        void loadSettings(bool);
        void saveSettings();
        bool getScanStatus();
        QString ubication_port();
        QString setDateInstallIni();
        QString installIni();
        int getNumberOfDevices_Hander(){return m_iDevicesNetwork - 1;}
        //--------------------------------------------------------------

        float voltage() const {return m_iVoltage;}
        QString mac() const {return m_sMac;}
        QString accData() const {return m_sAccData;}

        QString DevicesIni();
        QString AttemptIni();
        QString InstallIni();

        void setDevicesIni(QString);
        void setAttemptIni(QString);
        //void setInstallIni(QString);
        bool status() const {return m_bStatus;}
        float progressBarCounter() const {return m_iProgressBarCounter;}
        int currentGet() const {return m_iCurrentGet;}

        void setStatus(bool);
        void system_state(QString state);
        QString read_systemstate();
        QVariant listPort() const {return m_ListPort;}
        int getNumberOfDevices_Hand(){return m_iDevicesNetwork;}

    private:
        float voltageInterpreter(QByteArray payload);
        QString accInterpreter(QByteArray payload);
        void configurationXbee();
        void detection(QByteArray packet,QByteArray slave);
        void write(QString message);    //write in the file qFalsePositive
        void read(); //write in the file analysis
        void iniFileFalsePositive();
        void timerGetInd();
        inline void sleep(int time);    //sleep
        void changeThreshold(int);      //change threshold for sleep

    signals:
        void idChanged();
        void voltageChanged();
        void payloadChanged();
        void dataChanged();
        void receive_data(int);
        void change_check();
        void change_check2(int);
        void statusChanged();
        void updateProgressBar();
        void currentGetChanged();
        void listPortChanged();
        void iniChanged();
        void change_sleeping();
        void change_getState();
        void stopTimer();
        void stopTimerSleep();

    public slots:
        void updatePayload();
        void getXbeeList(QList<QByteArray>);
        void errorScan(void);
        void updateProgressBarCounter(int);
        void statusGet();
        void statusSleep();
        //QList<QString> test2() {return name_ports(); }


    private:
        API m_API;              //  Xbee class
        QTimer *m_timer;        //  Timer
        XbeeScan *mScan;        //  Scan thread
        Logger m_Logger;        //  Logger class
        int m_iDevicesNetwork = 4; //Number total of devices in the red
        int m_iCollector = 0;      //accumulate the "Gets" that arrive at the PORT
        int m_iNext = 0;           //indicate that the "Get" actual is different from the previous one
        int m_iThresholdSleep = 4; //Threshold to send "Gets" to sleep
        int m_iCurrentGet = 999;   //Set the current position of Get detection
        int m_iCyclesDetection  = 3;    //Detection Cycles
        int m_iTimeOutGet = 18000;      //Time Out in ms //change red to transparent
        int m_iTimeSleep = 15000000;       //System sleep  //Change yellow to transparent
        bool m_bStatus = false;
        bool m_bSignal_NI = false;
        bool m_bSleepGets = false;
        //bool m_bGet_NI = false;
        float m_iProgressBarCounter = 0;
        float m_iVoltage = 0;
        float m_lmyVoltList[9];         // Voltaje list
        QStringList m_lNameGets = {"DATA_GET1","DATA_GET2","DATA_GET3","DATA_GET4","DATA_GET5","DATA_GET6","DATA_GET7","DATA_GET8","DATA_GET9"};
        QStringList m_qStatesSleep = {"SinInitializar","Recibiendo","GetCaido","EnReposo"};
        QStringList m_lStateGet = {"Puesto","Caido","Indeterminado"};
        QStringList m_lGetFall = {};
        QString m_sCheck2 = m_lStateGet[1];
        QString m_sCheck2Final = m_lStateGet[1];
        QString m_sCheck[9] = {m_lStateGet[1]};
        QString m_sCheckFinal[9] = {m_lStateGet[1]};
        QString m_sMainDirectory = "output";
        QString m_sLoggingDirectory = "logging";
        QString m_sDataDirectory = "data";
        QString m_qPort;
        QString m_sMac = "000000";
        QString m_sAccData = "0, 0, 0";                 // x,y,z position
        QString m_Payload;
        QString m_lmyAccDataList[9];                    // Accell list
        QString m_bSystemSleeping = m_qStatesSleep[1];  //indicate if the GETS are in repose
        QList<QByteArray> m_lmyXbeeList = {};           // Device list
        QList<QString> m_SnameID = {};
        bool m_bScanStatus = true;                      // Scan status variable

        //QList<QByteArray> m_Pibot;
        QByteArray m_Pibot1;
        QByteArray m_Pibot2;
        QSettings* m_qSettings;
        QFile m_qFilPosNeg;
        QVariant m_ListPort;     //List to send qml

        QTimer m_timerSleep;
        QTimer m_timerGetInd;

};

#endif // HANDER_H
