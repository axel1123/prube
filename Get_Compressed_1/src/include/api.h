#ifndef API_H
#define API_H

#include <QtSerialPort/QSerialPort>
#include <QSerialPortInfo>
#include <QObject>

class API : public QObject
{
    Q_OBJECT

    public:
        API(QObject *parent = nullptr);     //Default constructor
        ~API();                             //Destructor
        bool init(const QString &portName);

    signals:
        void XbeeDataReady (void);

    public slots:
        void sendLocalAT(QByteArray ATCommad, QByteArray Payload, int PayloadSize, char FrameID);
        void sendTransmitRequest(QByteArray data, int size_data, QByteArray DestAddr, char FrameID);
        void sendRemoteAT(QByteArray ATCommad, QByteArray Payload, int PayloadSize, QByteArray DestAddr, char FrameID);
        void sendRemoteAT(QByteArray ATCommad, char Payload, int PayloadSize, QByteArray DestAddr, char FrameID);
        void onReceivedData();

    private:
        void sendFrame(QByteArray frame);

    public :
        char getFrameID();
        char getFrameType();
        int  getFrameLength();
        char getTransmitRequestStatus();
        char getRemoteATstatus();
        QByteArray getRemoteATcommand();
        QByteArray getRemoteATadrress();
        QByteArray getRemoteATpayload();
        char getLocalATstatus();
        QByteArray getLocalATcommand();
        QByteArray getLocalATpayload();
        char getRecivedPackageOptions();
        QByteArray getRecivedPackageAdrress();
        QByteArray getRecivedPackagePayload();
        bool receivedFrame(QByteArray data);
        void close();
        QList<QString> ports();

    private:
        QString m_sPortName;
        QByteArray m_bPayload;
        QByteArray m_bReadData;
        QByteArray m_bDestAddr;
        QByteArray m_bAtCommand;
        unsigned char m_cLength;
        unsigned char m_cStatus;
        unsigned char m_cFrameID;
        unsigned char m_cFrameType;

    public:
        QSerialPort* m_qSerial;

};

#endif // API_H
