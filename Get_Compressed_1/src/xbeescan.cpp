#include "include/xbeescan.h"

#include <QtSerialPort/QSerialPort>
#include <QSettings>
#include <QList>
#include <QDebug>

XbeeScan::XbeeScan(QObject *parent)
    : QThread(parent)
{
    QSettings settings("config/qsettings.ini", QSettings::IniFormat);//.ini for windows and any applications of GNU/LINUX
    m_iDevices = settings.value("Devices").toInt();
    m_iAttemps = settings.value("Attempt").toInt();
}

XbeeScan::~XbeeScan(){
    m_bIncommingData = false;
}

void XbeeScan::doSetup(API &Xbee, QTimer &timer)
{
    //Set API pointer and Timer pointer
    m_API = &Xbee;
    m_timer = &timer;

    //Connect signals to timer
    connect(this, SIGNAL(stopTimer()),m_timer,SLOT(stop()));
    connect(m_timer, SIGNAL(timeout()),this,SLOT(resendCommand()));
    connect(this, SIGNAL(startTimer(int)),m_timer,SLOT(start(int)));
    connect(m_API, SIGNAL(XbeeDataReady()), this, SLOT(serialHandler()));
}

void XbeeScan::resetScan()
{
    //m_iAttach = 0;
}

bool XbeeScan::sendLocalFN(QByteArray MyMAC)
{
    //Local variables
    QByteArray payload;             //Empty payload
    QByteArray AT_FN("FN");         //FN command to send
    int counter = 0;                //Incomming devices counter
    int attemps = 0;                //Attemp data
    bool ok = true;                 //Ok flag

    // Send FN command (frame ID has to be 1) and start timer
    emit requestLocalAT(AT_FN, payload, payload.size(), 1);
    emit startTimer(m_TimeOut);

    // Wait for (m_iDevices-1) response devices
    while (counter < m_iDevices-1) {
        // If incoming data
        if(m_bIncommingData){
            storeSlaveMatrix(MyMAC, counter);
            m_bIncommingData = false;
            // Restart timer
            emit startTimer(m_TimeOut);
            emit updateCounter(counter);
            counter++;
        }
        //If timeout is trigger
        if(m_bResend){
            //Clear variables
            counter = 0;
            attemps++;
            m_bResend = false;
            //Resend command
            emit requestLocalAT(AT_FN, payload, payload.size(), 1);
        }
        //If number of attemps is reached
        if(attemps >= m_iAttemps){
            ok = false;
            break;
        }
    }
    emit stopTimer();
    return ok;
}

bool XbeeScan::sendRemoteFN(QByteArray qMasterID)
{
    //Local variables
    bool ok = true;
    int attemps = 0;
    int counter1 = 0;
    QByteArray data;
    unsigned char frameid;
    QByteArray AT_FN("FN");
    int counter = m_iDevices-1;

    while (counter < m_iDevices*(m_iDevices-1)){
        //Update frame ID
        frameid = (counter)/(m_iDevices-1)+1;
        qDebug() << "frameid:" << (int)frameid;
        //Send remote FN data and start timer
        emit requestRemoteAT(AT_FN, data, data.size(), m_sMatrix[frameid-2][1], frameid);
        emit startTimer(m_TimeOut);

        // Wait for AT remote response
        while (counter1 < m_iDevices-1) {
            // If incoming data
            if(m_bIncommingData){
                // Update flags and store into data matrix
                storeSlaveMatrix(qMasterID,counter+counter1);
                m_bIncommingData = false;
                counter1 ++;
                // Restart timer
                emit startTimer(m_TimeOut);
            }

            //If timeout is trigger
            if(m_bResend){
                //Clear variables
                counter1=0;
                attemps++;
                m_bResend=false;
                //Resend command
                qDebug()<<"Resend:";
                emit requestRemoteAT(AT_FN, data, data.size(), m_sMatrix[frameid-2][1], frameid);
            }

            //If number of attemps is reached
            if(attemps >= m_iAttemps){
                ok = false;
                break;
            }
        }

        if(ok){
            //Update counters
            counter1 = 0;
            counter += (m_iDevices-1);
        } else {
            break;
        }
    }
    emit stopTimer();
    return ok;
}

void XbeeScan::storeSlaveMatrix(QByteArray qMasterID, int index)
{
    //Local variables
    char FrameType;
    QByteArray RSSI;
    QByteArray SlaveNI;
    QByteArray SlaveMAC;
    // QByteArray AtCommand;

    //Get frame ID Frame type and at command
    FrameType = m_API->getFrameType();
    char frameId = m_API->getFrameID();

    //Display Data
    switch (FrameType){
    //Local AT Command response
    case '\x88':
        //Check AT command
        if(m_API->getLocalATcommand() == "FN"){
            RSSI = getFNdata( m_API->getLocalATpayload(), SlaveMAC, SlaveNI, m_API->getLocalATpayload().size());
            //store the first MAC MASTER IN ASCII
            if(frameId == 1){
                m_sMatrix[index][0] = qMasterID;
            }else{
                m_sMatrix[index][0] = m_sMatrix[(int)frameId-2][1];
            }
            m_sMatrix[index][1] = SlaveMAC;
            m_sMatrix[index][2] = RSSI;
        }
        break;
        //Remote AT Command response
    case '\x97':
        //Check AT command
        if(m_API->getRemoteATcommand() == "FN"){
            RSSI = getFNdata( m_API->getRemoteATpayload(), SlaveMAC, SlaveNI, m_API->getRemoteATpayload().size());
            m_sMatrix[index][0] = m_sMatrix[(int)frameId-2][1];
            m_sMatrix[index][1] = SlaveMAC;
            m_sMatrix[index][2] = RSSI;
        }
        break;
    }
}

QByteArray XbeeScan::getmyOwnMAC()
{
    //Local variables
    QByteArray payload;
    QByteArray myOwnMAC;
    QByteArray AT_SH("SH");
    QByteArray AT_SL("SL");
    char FrameID = 1;
    //Send SH and SL commands
    emit requestLocalAT(AT_SH, payload, payload.size(), FrameID);
    while (!m_bIncommingData){}           //Wait for incoming data
    m_bIncommingData = false;

    myOwnMAC=m_API->getLocalATpayload();

    emit requestLocalAT(AT_SL, payload, payload.size(), FrameID);
    while (!m_bIncommingData){}           //Wait for incoming data
    m_bIncommingData = false;
    //Merge SH and SL
    myOwnMAC.append(m_API->getLocalATpayload());

    //Return myMAC
    return myOwnMAC;
}

void XbeeScan::sendDiscoveryOptions()
{
    //Local variables
    QByteArray rssiAvaiable;
    rssiAvaiable.resize(1);
    rssiAvaiable[0] = 0x04;
    QByteArray AT_NO("NO");

    //Send NO = 04
    emit requestLocalAT(AT_NO, rssiAvaiable, rssiAvaiable.size(), 1);
    while (!m_bIncommingData){}           //Wait for incoming data
    m_bIncommingData = false;
}

void XbeeScan::serialHandler()
{
    // Local variables
    QByteArray AT_NO("NO");

    if ((m_API->getLocalATstatus() == '\x00') || (m_API->getRemoteATstatus()=='\x00')){
        if (m_API->getLocalATcommand() == AT_NO || !(m_API->getLocalATpayload().isEmpty()) || m_API->getRemoteATcommand() == AT_NO || !(m_API->getRemoteATpayload().isEmpty())){
            m_bIncommingData = true;
            qDebug()<<"Received signal...";
        }
    }
}

void XbeeScan::resendCommand()
{
    m_bResend = true;
}

void XbeeScan::run()
{

//    QSettings settings("config/qsettings.ini", QSettings::IniFormat);
//    m_iDevices = settings.value("Devices").toInt();

    //qDebug() << "set1" << m_iDevices;

    //Local variables
//    QByteArray RSSI;
//    QByteArray SlaveNI;
//    QByteArray SlaveMAC;
//    QByteArray AtCommand;
    QByteArray qMasterID;
    QList<QByteArray> xbeeList;
    //char average;
    //bool complete = false;
    //unsigned int LargestInd;
    //unsigned int m_iLargest=0;
    //int m_iTotal = m_iDevices*(m_iDevices-1);
    //bool order = false;

//    connect(m_API, SIGNAL(XbeeDataReady()), this, SLOT(serialHandler()));
    qMasterID = getmyOwnMAC();
    QString hextoString = QString::fromLatin1(qMasterID.toHex());
    qDebug()<< "My ID: " << hextoString;

    sendDiscoveryOptions();

    //if(sendLocalFN(qMasterID))
    //    complete = true;
    // if(sendRemoteFN(qMasterID))
    //     order = true;

    qDebug() << "Devices xbeescan " << m_iDevices;

    //If all data is recived start order algorithm
    if (sendLocalFN(qMasterID)) {
        //find all devices
        //Store order matrix in a qlist
        for(int i = 0; i < m_iDevices-1; i++){
            xbeeList.append(m_sMatrix[i][1]);
            //display m_sMatrix matrix
            qDebug() << "MASTER:" << qByte2ASCII(m_sMatrix[i][0], 8);
            qDebug() << "Slave :" << qByte2ASCII(m_sMatrix[i][1], 8);
            qDebug() << "RSSI  :" << qByte2ASCII(m_sMatrix[i][2], 1);
        }
        /*
        QVector<QByteArray> m_lAll_devices;
        m_lAll_devices.resize(m_iDevices);
        m_lAll_devices[0] = m_sMatrix[0][0];
        for(int l=0;l<m_iDevices-1;l++){
            m_lAll_devices[l+1] = m_sMatrix[l][1];
        }
*/
        emit xbeeScanComplete(xbeeList);
        /*
        //find the Master
        for(int i = m_iDevices - 1; i < m_iTotal;i=i+(m_iDevices-1)){
            for(int j = 0 ; j < m_iDevices; j++){
                if(m_sMatrix[i][1] != m_lAll_devices[j] && m_sMatrix[i+1][1] != m_lAll_devices[j]){
                    m_sMatrix[i][0] = m_lAll_devices[j];
                    m_sMatrix[i+1][0] = m_lAll_devices[j];
                }
            }
        }

        //Find Averages
        int m_iAverage[m_iTotal];
        int m_iIndex[m_iTotal];
        for(int i = 0; i< m_iTotal;i++){
            for(int j = 0; j < m_iTotal; j++){
                if(m_sMatrix[i][0] == m_sMatrix[j][1] && m_sMatrix[i][1] == m_sMatrix[j][0]){
                    m_iAverage[i] = (m_sMatrix[i][2].at(0)+m_sMatrix[j][2].at(0))/2;
                    average = m_iAverage[i];
                    m_sMatrix[i][2].replace(0 ,1 , &average);
                    m_sMatrix[j][2].replace(0 ,1 , &average);
                    m_iIndex[i] = i;
                    //                    qDebug() << "The RSSI1 is: " << qByte2ASCII(m_sMatrix[i][0],8);
                    //                    qDebug() << "The RSSI2 is: " << qByte2ASCII(m_sMatrix[j][1],8);
                    //                    qDebug() << "The RSSI1 is: " << qByte2ASCII(m_sMatrix[i][1],8);
                    //                    qDebug() << "The RSSI2 is: " << qByte2ASCII(m_sMatrix[j][0],8);
                    //                    qDebug() << "The average is: " << m_iAverage[i];
                }
            }
        }

        //Find the largest indice
        int n = sizeof(m_iAverage) / sizeof(m_iAverage[0]);
        int m_iLargest = m_iAverage[0];
        int a;

        for(int i = 1; i < n; i++){
            if(m_iLargest > m_iAverage[i]){
                m_iLargest = m_iAverage[i];
                a = i;
            }
        }
        //qDebug() << "Largest element = " << m_iLargest;

        //        //Get the mean RSSI and find the largest
        //        //for(int i = 0 ; i < m_iTotal;i++){
        //        for(int i = m_iDevices - 1; i < m_iTotal;i=i+(m_iDevices-1)){
        //            //for(int j = 0 ; j < m_iTotal; j++){
        //            for(int j = 0 ; j < m_iTotal; j++){
        //                if(m_sMatrix[i][0]==m_sMatrix[j][1] && m_sMatrix[j][0]==m_sMatrix[i][1]){
        //                    average = (m_sMatrix[i][2].at(0)+m_sMatrix[j][2].at(0))/2;
        //                    m_sMatrix[i][2].replace(0 ,1 , &average);
        //                    m_sMatrix[j][2].replace(0 ,1 , &average);
        //                    if(m_sMatrix[j][1] != m_sMatrix[0][0] && m_sMatrix[i][1] != m_sMatrix[0][0]){
        //                    //Find the largest element
        //                    if(m_iLargest < (unsigned int) average){
        //                        m_iLargest = average;
        //                       LargestInd = i;
        //                   }
        //                   }
        //                }
        //            }

        //        }

        //        qDebug() << "Largest element = " << m_sMatrix[LargestInd][0] <<","<< m_iLargest;

        //qDebug() << "start 0";
        //Copy all the elements with master
        QByteArray order[m_iDevices-1][3];
        int g=0;//listo
        //qDebug() << "start 0.1";
        for(int c = 0; c < m_iTotal; c++){
            //if(m_sMatrix[m_iIndex[a]][0] == m_sMatrix[c][0]){
            if(m_sMatrix[LargestInd][0] == m_sMatrix[c][0]){
                for(int d = 0; d < 3 ; d++){
                    order[g][d] = m_sMatrix[c][d];
                }
                g++;
            }
        }
        //qDebug() << "start 1";

        // Order elements
        QByteArray aux4[3];
        int MAX = m_iTotal/m_iDevices;
        for(int i = 0   ; i <   MAX ; ++i){
            for(int j = i+1 ;   j   <   MAX; ++j){
                if(order[j][2] < order[i][2]){
                    for(int k = 0; k < 3; k++){
                        aux4[k] = order[i][k];
                        order[i][k] = order[j][k];
                        order[j][k] = aux4[k];
                    }
                }
            }
        }

        //Store order matrix in a qlist
        for(int i=0; i<MAX; i++){
            xbeeList.append(order[i][1]);
            //display order matrix
            qDebug()<<"MASTER:"<<qByte2ASCII(order[i][0],8);
            qDebug()<<"Slave :"<<qByte2ASCII(order[i][1],8);
            qDebug()<<"RSSI  :"<<qByte2ASCII(order[i][2],1);
        }
        complete = true;
        emit xbeeOrderFinish(xbeeList);
        if(complete == true){
            emit xbeeComplete(complete);
        }

    }else{
        qDebug()<<"No response...";
    }
    */
    } else {
        emit xbeeScanIncomplete();
        qDebug()<<"ERROR: No enough devices finded! Check m_iDevices variable..";
    }
}

//Turn QByte array into ASCCII to show MAC address
QString XbeeScan::qByte2ASCII(QByteArray qArray, int arraySize)
{
    //Local variables
    char a[3]={} ;                  //WHy 3?
    char array[arraySize];
    QString diplayData;
    // Convert into char array
    memcpy(array, qArray.data(), arraySize);
    for(int p = 0; p < arraySize; p++){
        sprintf(a, "%X",(unsigned char)array[p]);
        diplayData.append(a);
    }
    return diplayData;
}

QByteArray XbeeScan::getFNdata(QByteArray data, QByteArray &SlaveAddrr, QByteArray &SlaveNI, int Length)
{
    //Local vaiable
    QByteArray SlaveRSSI;
    //Get data
    SlaveAddrr = data.mid(2,8);
    SlaveNI = data.mid(10,Length-20);
    SlaveRSSI = data.mid(data.size()-1);
    return SlaveRSSI;
}

void XbeeScan::setNumberOfDev(int amount)
{
    m_iDevices = amount;
}
