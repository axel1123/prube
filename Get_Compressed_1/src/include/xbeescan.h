#ifndef XBEESCAN_H
#define XBEESCAN_H

#include "include/api.h"
#include "include/logger.h"
#include <QObject>
#include <QThread>
#include <QTimer>

class XbeeScan : public QThread
{
    Q_OBJECT
    public:
        XbeeScan(QObject *parent = nullptr);        //Constructor
        ~XbeeScan();                                 //Destructor

    public slots:
        void serialHandler();
        void resendCommand();

    signals:
        void stopTimer();
        void startTimer(int);
        void valorCambiado(int);
        void requestLocalAT(QByteArray, QByteArray, int, char);
        void requestRemoteAT(QByteArray, QByteArray, int, QByteArray, char);
        void xbeeScanComplete(QList<QByteArray>);
        void xbeeScanIncomplete(void);
        void updateCounter(int);

    public:
        void resetScan();
        void doSetup(API &Xbee, QTimer &timer);
        QString qByte2ASCII(QByteArray qArray, int arraySize);
        QByteArray getFNdata(QByteArray data,QByteArray &SlaveAddrr, QByteArray &SlaveNI, int Length);
        void setNumberOfDev(int);

    private:
        QByteArray getmyOwnMAC();
        void sendDiscoveryOptions();
        bool sendLocalFN(QByteArray MyMAC);
        bool sendRemoteFN(QByteArray qMasterID);
        void storeSlaveMatrix(QByteArray qMasterID, int index);

    protected:adasd
        void run() override;

    private:
        API *m_API;                         //include API Class
        QTimer *m_timer;                    //Create timer
        Logger m_Logger;                    //include Logger Class
        bool m_bResend = false;             //Resend flag
        bool m_bIncommingData = false;      //Incoming data flag
        int m_iDevices = 4;                 //Number total of devices in the red
        int m_iAttemps = 30;                 //Number of attemps
        int m_TimeOut = 10000;              //Time Out in ms
        unsigned char m_cFrameType;
        QByteArray m_sMatrix[12*(12-1)][3]; //Stores local and remote MAC'S and RSSI
};

#endif // XBEESCAN_H
