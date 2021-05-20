#include "include/api.h"
#include "mlog.h"

#include <QDebug>

//Constructor function set new Serial Port
API::API(QObject *parent) :
    QObject(parent)
{
    m_qSerial = new QSerialPort();
}

//Destructor function close Serial Port
API::~API(){

}

/*bool API::init(&portName)
 * Creates Xbee serial and return true if serial comunication is
 * succesfully or false if selected device cannot be openned
 * Inputs:
 *  const QString &portName: Xbee Serial Port.
 */
bool API::init(const QString &portName)
{
    //Local variables
    bool Status;
    //Assign Serial Port
    m_qSerial->setPortName(portName);
    m_qSerial->setBaudRate(QSerialPort::Baud9600);
    m_qSerial->setDataBits(QSerialPort::Data8);
    m_qSerial->setParity(QSerialPort::NoParity);
    m_qSerial->setStopBits(QSerialPort::OneStop);
    m_qSerial->setFlowControl(QSerialPort::NoFlowControl);
    m_qSerial->open(QIODevice::ReadWrite);

    if (m_qSerial->isOpen() == true){
        Status = true;
        connect(m_qSerial, SIGNAL(readyRead()), this, SLOT(onReceivedData()));
    } else {
        Status = false;
    }
    return Status;
}

/*
 *
 *
 */
QList<QString> API::ports()
{
    QList<QString> list;
    const auto infos = QSerialPortInfo::availablePorts();
    for (const QSerialPortInfo &info : infos){
        //list.append(info.portName());
        list.append(info.portName());
        //  ui->comboBox_Port->addItem(info.portName());
    }
    return list;
    //return {};
}

/*
 *
 *
 */
void API::close(){
    m_qSerial -> close();
}

/*void API_transmitRequest(@data, @size_data, @DestAddr, @FrameID)
 * Create a transmit Request frame in API Mode
 * Inputs:
 *  char array data: Data to be sended.
 *  int size_data: length of payload data
 *  char array DestAddr: Receiver 64-bit Address
 *  char FrameID: Frame Id
 */
void API::sendTransmitRequest(QByteArray data, int size_data, QByteArray DestAddr, char FrameID)
{
    //Local Variables
    char Length = 0;
    QByteArray frame;
    char checksum = 0;
    int size_DestAddr = DestAddr.size();

    //Transmit Request frame length
    Length = 0x0E + size_data;

    //Organizing array
    frame.resize(18+sizeof(data));
    frame[0]= 0x7E;                             // Start delimiter
    frame[1]= (Length >> 8) & 0Xff;             // MSB_length
    frame[2]= Length & 0Xff;                    // LSB_length
    frame[3]= 0x10;                             // Frame Type (Transmit Request)
    frame[4]= FrameID;                          // Frame ID
    frame[13]= 0xFF;                            // MSB 16-bit dest.add (unknown)
    frame[14]= 0XFE;                            // LSB 16-bit dest.add (unknown)
    frame[15]= 0X00;                            // Broadcast radius
    frame[16]= 0X00;                            // Options

    //Merge DestAddr and DestAddr
    frame.replace(5,size_DestAddr,DestAddr);
    frame.replace(17,size_data,data);

    //Get checksum
    for (int i = 3; i < 17+size_data; i++)
        checksum += frame[i];
    checksum = 0Xff - checksum&0Xff;

    //Merge cheksum in Frame and get framesize
    frame[17+size_data] = checksum;

    //Convert into APIframe2 and send throught SerialPort
    sendFrame(frame);
}

/*void API_remoteAT(@ATCommad, @Payload, @PayloadSize, @DestAddr, @FrameID)
 * Create a remoteAT API frame
 * Inputs:
 *  char *ATCommad: AT command to be sended.
 *  char *Payload: Payload to be sended.
 *  int PayloadSize: length of payload data
 *  char array DestAddr: Receiver 64-bit Address
 *  char FrameID: Frame Id
 */
void API::sendRemoteAT(QByteArray ATCommad, QByteArray Payload, int PayloadSize, QByteArray DestAddr, char FrameID)
{
    //Local variables
    char Length = 0;
    QByteArray frame;
    char checksum = 0;
    int ATCommadSize=2;
    int size_DestAddr = DestAddr.size();

    //Calculate frame length
    Length = 0x0D + ATCommadSize + PayloadSize;

    //Organizing Frame
    frame.resize(17+ATCommadSize+ PayloadSize);
    frame[0]= 0x7E;                             // Start delimiter
    frame[1]= (Length >> 8) & 0Xff;             // MSB_length
    frame[2]= Length & 0Xff;                    // LSB_length
    frame[3]= 0x17;                             // Frame Type (Remote AT command)
    frame[4]= FrameID;                          // Frame ID
    frame[13]= 0xFF;                            // MSB 16-bit dest.add (unknown)
    frame[14]= 0XFE;                            // LSB 16-bit dest.add (unknown)
    frame[15]= 0X02;                            // Remote cmd. options

    //Merge DestAddress, AT Command and Payload
    frame.replace(5,size_DestAddr,DestAddr);
    frame.replace(16,2,ATCommad);
    if(PayloadSize !=0 )
        frame.replace(18,PayloadSize,Payload);

    //Calculate Checksum
    for (int i = 3; i < 16+(ATCommadSize+ PayloadSize); i++){
        checksum += frame.at(i);
    }
    checksum = 0Xff - checksum&0Xff;

    //Merge cheksum in Frame
    frame[16+ATCommadSize+PayloadSize] = checksum;

    //Convert into APIframe2 and send throught SerialPort
    sendFrame(frame);
}

void API::sendRemoteAT(QByteArray ATCommad, char Payload, int PayloadSize, QByteArray DestAddr, char FrameID)
{
    //Local variables
    char Length = 0;
    QByteArray frame;
    char checksum = 0;
    int ATCommadSize=2;
    int size_DestAddr = DestAddr.size();

    //Calculate frame length
    Length = 0x0D + ATCommadSize + PayloadSize;

    //Organizing Frame
    frame.resize(17+ATCommadSize+ PayloadSize);
    frame[0]= 0x7E;                             // Start delimiter
    frame[1]= (Length >> 8) & 0Xff;             // MSB_length
    frame[2]= Length & 0Xff;                    // LSB_length
    frame[3]= 0x17;                             // Frame Type (Remote AT command)
    frame[4]= FrameID;                          // Frame ID
    frame[13]= 0xFF;                            // MSB 16-bit dest.add (unknown)
    frame[14]= 0XFE;                            // LSB 16-bit dest.add (unknown)
    frame[15]= 0X02;                            // Remote cmd. options

    //Merge DestAddress, AT Command and Payload
    frame.replace(5,size_DestAddr,DestAddr);
    frame.replace(16,2,ATCommad);

    if((ATCommad == "SM" || ATCommad == "AP")&&(PayloadSize == 1)){
        frame[18] = Payload;
        //frame.replace(18,PayloadSize,Payload.toHex());
    }
    //frame[18] = 0x01;
    //Calculate Checksum
    for (int i = 3; i < 16+(ATCommadSize+ PayloadSize); i++){
        checksum += frame.at(i);
    }
    checksum = 0Xff - checksum&0Xff;

    //Merge cheksum in Frame
    frame[16+ATCommadSize+PayloadSize] = checksum;

    //Convert into APIframe2 and send throught SerialPort
    sendFrame(frame);
}

/*void API_localAT(@*ATCommad, @*Payload, @PayloadSize, @ FrameID)
 * Create a localAT API frame
 * Inputs:
 *  char *ATCommad: AT command to be sended.
 *  char *Payload: Payload to be sended.
 *  int PayloadSize: length of payload data
 *  char FrameID: Frame Id
 */
void API::sendLocalAT(QByteArray ATCommad, QByteArray Payload, int PayloadSize, char FrameID)
{
    /*
    qDebug()<<"Local";
    qDebug()<<"Data1:"<<ATCommad;
    qDebug()<<"Data2:"<<Payload;
    qDebug()<<"Data3:"<<PayloadSize;
    qDebug()<<"Data4:"<<(unsigned char)FrameID;
    */

    //Local Variable
    char Length = 0;
    QByteArray frame;
    char checksum = 0;
    int ATCommadSize = 2;

    //Calculate frame length
    Length = 0x02 + ATCommadSize + PayloadSize;

    //Generate LocalAT frame
    frame.resize(6+ATCommadSize+ PayloadSize);
    frame[0]= 0x7E;                          // Start delimiter
    frame[1]= (Length >> 8) & 0Xff;          // MSB_length
    frame[2]= Length & 0Xff;                 // LSB_length
    frame[3]= 0x08;                          // Frame Type (Transmit Request)
    frame[4]= FrameID;                       // Frame ID

    //Copy AT command and Payload
    frame.replace(5,ATCommadSize,ATCommad);
    if(PayloadSize !=0 )
        frame.replace(7,PayloadSize,Payload);

    //Calculate Checksum
    for (int i = 3; i < 5 +(ATCommadSize + PayloadSize); i++){
        checksum += frame.at(i);
    }
    checksum = 0Xff - checksum & 0Xff;

    //Merge cheksum in Frame
    frame[5+ATCommadSize+PayloadSize] = checksum;

    //Convert into APIframe2 and send throught SerialPort
    sendFrame(frame);
}


/*void API2_Send(@frame, @PayloadSize, @size_frame)
 * Turn a API frame into API frame2 and send throught
 * Serial Port
 * Inputs:
 *  char array frame: API frame to be sended.
 *  int PayloadSize: length of payload data
 *  int size_frame: Frame lenght.
 */

void API::sendFrame(QByteArray frame)
{
    //Local variables
    int add = 0;
    QByteArray Message;
    int size_frame = frame.size();

    //Check for special cases
    for(int j = 1; j < size_frame; j++){
        if (frame.at(j) == 0x13 or frame.at(j) == 0x11 or frame.at(j) == 0x7E or frame.at(j) == 0x7D)
            add++;
    }

    //Create Message and copy frame
    Message.resize(size_frame + add);
    Message.replace(0,size_frame,frame);
    int Message_size = Message.size();

    //Replace special cases
    for(int j = 1; j < Message_size; j++){
        if (Message.at(j) == 0x13 or Message.at(j) == 0x11 or Message.at(j) == 0x7E or Message.at(j) == 0x7D){
            //Message.replace(j+1,Message_size-j-add,Message.mid(j,Message_size-j-add));
            for(int k = Message.size()-2 ; k > j ; k--){
                Message[k+1] = Message.at(k);
            }
            Message[j+1] = Message.at(j) ^ 0x20;
            Message[j] = 0x7D;
        }
    }

    //Send througth SerialPort
//    qDebug()<<">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>";
//    QString hextoString = QString::fromLatin1(Message.toHex());
//    qDebug()<<"Send API:" << hextoString;
//    qDebug()<<">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>";

    m_qSerial->write(Message);
}

/*void API_receive(@data)
 * Check the start delimiter, checksum bit and recognize data frame Type
 * Inputs:
 *  QByteArray m_Data: Serial Port incoming data.
 */
bool API::receivedFrame(QByteArray data)
{
    //Local Variables
    int Length;
    unsigned char add = 0;
    unsigned char checksum  = 0;
    QString hextoString;

    //Check the Start delimiter
    if(data.startsWith(0x7E)){
        //Turn API2 frame into API frame
        for(unsigned char j = 0; j < data.size(); j++){
            if (data.at(j)== 0x7D){
                for(unsigned char k = j; k < data.size()-1; k++){
                    data[k] = data.at(k+1);
                }
                data[j] = (data.at(j) ^ 0x20);
                add++;
            }
        }

        //Check if its posible get PayloadLength
        if(data.size()>3){
            /*Check payload size (MSBPayloadLength<<8|LSBPayLoadLength|+add+4bits)
              4 bits more because Start delimiter, MSB,LSB Lenght and Checksum are
              not considered.
             */
            Length = (data.at(1)&0xFF)<<8|data.at(2)+4+add;
            //Check if data Frame is complete
            if(Length == data.size()){
                //Review checksum
                for( unsigned char k = 3; k < data.size() - add; k++){
                    checksum = checksum + data.at(k);
                }
                //If checksum is ok
                if (checksum == 0xFF){
                    //Assign length, frameID and type
                    this -> m_cLength = Length;
                    this -> m_cFrameID = data.at(4);
                    this -> m_cFrameType = (unsigned char)data.at(3);
                    //Frame Type Handle
                    switch(m_cFrameType){
                    case 0x8B:
                        //qDebug() << "Transmit Status";
                        this -> m_cStatus = data.at(8);
                        this -> m_bAtCommand = {};
                        this -> m_bDestAddr = {};
                        this -> m_bPayload = {};
                        break ;
                    case 0X88 :
//                        qDebug() << "AT Command Response";
                        this -> m_cStatus = data.at(7);
                        this -> m_bAtCommand = data.mid(5,2);
                        //Check if there are Payload
                        if (Length > 5)
                            this -> m_bPayload = data.mid(8,Length-9-add);
                        else
                            this -> m_bPayload = {};
                        this -> m_bDestAddr = {};
                        break ;
                    case 0x97 :
//                        qDebug() << "Remote AT Command Response";
                        this -> m_cStatus = data.at(17);
                        this -> m_bAtCommand = data.mid(15,2);
                        this -> m_bDestAddr = data.mid(5,8);
                        //Check if there are payload
                        if (Length > 15)
                            this -> m_bPayload = data.mid(18,Length-19-add);
                        else
                            this -> m_bPayload = {};
                        break ;
                    case 0x90:
//                        qDebug() << "Receive Packet";
                        this -> m_cStatus = data.at(14);
                        this -> m_bAtCommand = {};
                        this -> m_bDestAddr = data.mid(4,8);
                        if (Length > 12)
                            this -> m_bPayload = data.mid(15,Length-16-add);
                        else
                            this -> m_bPayload = {};
                        break;
                    default:
                        this -> m_cStatus = {};
                        this -> m_bAtCommand = {};
                        this -> m_bDestAddr = {};
                        this -> m_bPayload = {};
                        break;
                    }
                }
                hextoString = QString::fromLatin1(m_bPayload.toHex());
//                qDebug()<<"Payload:"<< hextoString;
                return true;
            } else {
                return false;
            }
        } else {
            return false;
        }
    }
    return true;
}

/* char API::getFrameType()
 * Return the frame Type to identify the incoming data.
 * Inputs:
 */
char API::getFrameType()
{
    return m_cFrameType;
}

/* char API::getFrameID()
 * Return the frame ID.
 * Inputs:
 */
char API::getFrameID()
{
    return m_cFrameID;
}

/* char API::getTransmitRequestStatus()
 * If the return frame is Transmit status Type
 * Return the status.
 * Inputs:
 */
char API::getTransmitRequestStatus()
{
    if (m_cFrameType == 0x8B){
        return m_cStatus;
    } else {
        return 0x00;
    }
}

/* QByteArray API::getRemoteATcommand()
 * If the return frame is a Remote AT response Type
 * Return the AT command in a QByteArray.
 * Inputs:
 */
QByteArray API::getRemoteATcommand()
{
    if(m_cFrameType == 0x97){
        return this -> m_bAtCommand;
    }
    return {};
}

/* char API::getRemoteATadrress()
 * If the return frame is a Remote AT response Type
 * Return the source address in a QByteArray.
 * Inputs:
 */
QByteArray API::getRemoteATadrress()
{
    if(m_cFrameType == 0x97){
        return this -> m_bDestAddr;
    }
    return {};
}

/* char API::getRemoteATpayload()
 * If the return frame is a Remote AT response Type
 * Return the associated payload in a QByteArray.
 */
QByteArray API::getRemoteATpayload()
{
    if(m_cFrameType == 0x97){
        return this -> m_bPayload;
    }
    return {};
}

/* char API::getRemoteATstatus()
 * If the return frame is a Remote AT response Type
 * Return the status.
 */
char API::getRemoteATstatus()
{
    if (m_cFrameType == 0x97){
        return m_cStatus;
    } else {
        return 0x00;
    }
}

/* char API::getLocalATcommand()
 * If the return frame is an AT Response Type
 * Return the AT command in a QByteArray.
 */
QByteArray API::getLocalATcommand()
{
    if(m_cFrameType == 0x88){
        return this -> m_bAtCommand;
    }
    return {};
}

/* char API::getLocalATpayload()
 * If the return frame is an AT Response Type
 * Return the associated payload in a QByteArray.
 */
QByteArray API::getLocalATpayload()
{
    if(m_cFrameType== 0x88){
        return this -> m_bPayload;
    }
    return {};
}

char API::getRecivedPackageOptions()
{
    if(m_cFrameType== 0x90){
        return this -> m_cStatus;
    }
    return 0x00;
}

QByteArray API::getRecivedPackageAdrress()
{
    if(m_cFrameType== 0x90){
        return this -> m_bDestAddr;
    }
    return {};
}

QByteArray API::getRecivedPackagePayload()
{
    if(m_cFrameType== 0x90){
        return this -> m_bPayload;
    }
    return {};
}

/* char API::getLocalATstatus()
 * If the return frame is an AT Response Type
 * Return the status.
 */
char API::getLocalATstatus()
{
    if (m_cFrameType== 0x88){
        return m_cStatus;
    } else {
        return 0x00;
    }
}

/* char API::getLocalATstatus()
 * Return frame Length.
 */
int API::getFrameLength()
{
    return m_cLength;
}

/* void API::on_Received_Data()
 * Recive data Handle
 */
void API::onReceivedData()
{
    //Local variables
    QByteArray rawData;
    QString hextoString;
    bool split = false;

    //Copy incoming data into m_bReadData and evaluate split condition
    m_bReadData.append(m_qSerial->readAll());
    if(!m_bReadData.isEmpty()){
        split = (m_bReadData.count(0x7E)>1);

        //Split combined frames
        if(split){
            rawData = m_bReadData.right(m_bReadData.size() - m_bReadData.indexOf(0x7E,2));
            m_bReadData.truncate(m_bReadData.indexOf(0x7E,2));
        }

        //Check if incoming data frame is complete
        if (receivedFrame(m_bReadData)){
            qDebug()<<"<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<";
            QString hextoString = QString::fromLatin1(m_bReadData.toHex());
            qDebug()<<"Received API:"<<hextoString;
            qDebug()<<"<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<";
            emit XbeeDataReady();          //Emit signal
            m_bReadData.clear();           //Clear m_ReadData variable
        }

        //Get the another frame
        if(split){
            m_bReadData = rawData;
            rawData.clear();
        }
    }
}
