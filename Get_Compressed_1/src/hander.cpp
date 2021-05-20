#include "include/hander.h"
#include <QFile> 

Hander::Hander()
{
    //Create config folder if it is not exist
    if(!QDir("config").exists()){
        QDir().mkpath("config");
    }

    //Init QSettings
    m_qSettings = new QSettings("config/qsettings.ini", QSettings::IniFormat);
    //found positives negatives
    //m_qFilPosNeg = new QFile("qFalsePositives.txt");

    //Init qsetting if it is not exist
    if(!QFile("config/qsettings.ini").exists()){
        for(int i = 0; i < m_lNameGets.size(); i++){
            m_qSettings->setValue("GET/"+m_lNameGets[i], "ffff");
        }
        m_qSettings->setValue("Devices", "4");
        m_qSettings->setValue("PortCom", "ttyUSB0");
        m_qSettings->setValue("Attempt", "30");
        m_qSettings->setValue("Threshold","4");
        m_iDevicesNetwork = m_qSettings->value("Devices").toInt();
        m_iThresholdSleep = m_qSettings->value("Threshold").toInt();
    }

    iniFileFalsePositive();

    m_timer = new QTimer();
    mScan = new XbeeScan(this);
    mScan->doSetup(m_API, *m_timer);

    // Connect functions
    connect(mScan, SIGNAL(requestLocalAT(QByteArray, QByteArray, int, char)), &m_API, SLOT(sendLocalAT(QByteArray, QByteArray, int, char)));
    connect(mScan, SIGNAL(requestRemoteAT(QByteArray, QByteArray, int, QByteArray, char)), &m_API,SLOT(sendRemoteAT(QByteArray, QByteArray, int, QByteArray, char)));
    // Signal that stores the xbee order
    connect(mScan, SIGNAL(xbeeScanComplete(QList<QByteArray>)), this, SLOT(getXbeeList(QList<QByteArray>)));
    //connect(mScan,SIGNAL(xbeeComplete(bool),this,SLOT(getXbeeComplete(bool)))  ;
    connect(mScan, SIGNAL(updateCounter(int)), this, SLOT(updateProgressBarCounter(int)));

    //timer "Get" yellow to transparent
    connect(this,SIGNAL(stopTimerSleep()),&m_timerSleep,SLOT(stop()));
    connect(&m_timerSleep,SIGNAL(timeout()),this,SLOT(statusSleep()));
    connect(this,SIGNAL(receive_data(int)),&m_timerSleep,SLOT(start(int)));

    //timer "Get" red to transparent
    connect(this,SIGNAL(stopTimer()),&m_timerGetInd,SLOT(stop()));
    connect(&m_timerGetInd, SIGNAL(timeout()), this,SLOT(statusGet()));
    connect(this,SIGNAL(change_check2(int)),&m_timerGetInd,SLOT(start(int)));
    //si vuelve a ser conectado en esos treinta segundos, cancelar el timer con un stop

    //Number total of devices in the red
    //m_iDevicesNetwork = mScan->getNumberOfDevices();
    m_iDevicesNetwork = getNumberOfDevices_Hander();

    if(QFile("config/qsettings.ini").exists()){
        loadSettings(true);
    }
}

Hander::~Hander()
{
    delete mScan;
    delete m_timer;
}

void Hander::init(QString port)
{
    qDebug()<<"PORT COM from system" << port << "connected";
    //qDebug()<<"PORT COM from .ini" << settings.value("PortCom").toString();
    //m_API.init(settings.value("PortCom").toString());
    m_qSettings->setValue("PortCom", port);
    m_API.init(port);
    connect(&m_API, SIGNAL(XbeeDataReady()), this, SLOT(updatePayload()));
    //    qDebug() << "flag" << bandera;
    //    if(bandera == true){
    //        sendSleepCommand();
    //    }
    m_qPort = port;
}

void Hander::iniFileFalsePositive(){

    QString rut = "analysis";
    if(!QDir(rut).exists()){
        QDir().mkpath(rut);
        qDebug() << "The Dir" << rut << " was created";
    }

    //if(!QFile(rut+"/qFalsePositive.txt").exists()){
        m_qFilPosNeg.setFileName(rut + "/qFalsePositive.txt");
      //  qDebug() << "The File" << m_qFilPosNeg << " was created";
    //}

//    if(m_qFilPosNeg.pos() == 0){ // if the file is empty

        //    }
}

void Hander::sleep(int time)
{
    QEventLoop loop;
    QTimer t;
    t.connect(&t, &QTimer::timeout, &loop, &QEventLoop::quit);
    t.start(time);
    loop.exec();
}

void Hander::changeThreshold(int threshold)
{
    m_qSettings->setValue("Threshold",threshold);
    m_iThresholdSleep = threshold;
    qDebug() << m_qSettings->value("Threshold").toInt();
}

QString Hander::getLocalPayload()
{
    return m_Payload;
}

void Hander::scanGETS()
{

//    QString date = QDateTime::currentDateTime().toString("mm");
//    logger()->enableLogToFile(date);
    // Set XbeeScan and timer
    qDebug() << "devices" << m_iDevicesNetwork;
    // Scan devices
    qDebug() << "Escaneando Dispositivos";
    configurationXbee();

    for(int i = 0; i < 1000000 ; i++){
        qDebug() << i;
        sendSleepCommand();
        sleep(100);
    }

    mScan->setNumberOfDev(getNumberOfDevices_Hand());
    //qDebug() << "The value xbee is: " << SIGNAL(xbeeComplete(bool));
    mScan -> start();
}

void Hander::updatePayload()
{
    emit receive_data(m_iTimeSleep);
    m_Payload = m_API.getRemoteATpayload();
    QByteArray AT_NI("NI");
    QByteArray AT_voltage("%V");
    QByteArray remoteAT;
    QByteArray recivedPacket;

    QString recivedAxes;
    double recivedVoltaje;
    remoteAT = m_API.getRemoteATcommand();

    if (m_API.getRecivedPackagePayload().size() == 8){
        if((unsigned char)m_API.getFrameType()== 0X97){
            if (remoteAT == AT_NI){
                m_bSignal_NI = true;
            }else if (remoteAT == AT_voltage) {
                m_iVoltage = voltageInterpreter(m_API.getRemoteATpayload());
                emit voltageChanged();
            }
        } else if((unsigned char)m_API.getFrameType() == 0X90){
            QByteArray bMacSlave;
            recivedPacket = m_API.getRecivedPackagePayload();
            bMacSlave = m_API.getRecivedPackageAdrress();
            recivedAxes = accInterpreter(recivedPacket.left(6));
            recivedVoltaje = voltageInterpreter(recivedPacket.right(2));

//            qDebug() << "====== Data ======";
//            QString hextoString = QString::fromLatin1(bMacSlave.toHex());
//            qDebug() << "Slave MAC:" << hextoString;
//            qDebug() << "Accel data: " << recivedAxes;
//            qDebug() << "Voltaje: " << recivedVoltaje;
//            qDebug() << "====================";
//            qDebug() << "Recibiendo Data(MAC, Axes, Voltaje) por el puerto Serial";

            if(!m_lmyXbeeList.isEmpty()){

                //save in the files .csv
                int i = m_lmyXbeeList.indexOf(bMacSlave);
                if(i >= 0){
                    QString m_sDirection = m_lNameGets[i] + "_" + bMacSlave.toHex();
                    m_Logger.messageHandler(m_sDirection, recivedAxes, recivedVoltaje,bMacSlave.toHex());
                }
                //deteccion of the missing "GET"
                detection(recivedPacket, bMacSlave);

            } else {
                qDebug() << "The principal matrix is empty";
            }
        }
        emit payloadChanged();
        emit dataChanged();

        //    qDebug()<<"Check vector";
        //    for(int h = 0; h < 3; h++){
        //        qDebug()<<check[h];
        //    }
    }
}

void Hander::detection(QByteArray recivedPacket, QByteArray macSlave)
{

    QString actualGet = "";
    QString get = "";
    QString dataGet = "";

    //put the get's found in state 'Puesto'
    float sum = 0;
    for(int i = 0; i < m_iDevicesNetwork - 1; i++){ //<------ //9
        if(macSlave == m_lmyXbeeList[i]){
            qDebug() << m_lNameGets.at(i);
            m_lmyVoltList[i] = voltageInterpreter(recivedPacket.right(2));
            m_lmyAccDataList[i] = accInterpreter(recivedPacket.left(6));
            m_sCheck[i] = m_lStateGet[0];
            if (m_iNext == 0)m_Pibot1 = macSlave;
            if (m_iNext == 1)m_Pibot2 = macSlave;
            m_iNext++;
        }
        sum += voltageInterpreter(recivedPacket.right(2));
    }

    //Calculate average of Bateries
    float average = sum/static_cast<float>(m_iDevicesNetwork-1);
    m_iVoltage = average;

    //count differents gets
    if(m_iNext == 2) m_iNext = 0;
    if(m_Pibot1 != m_Pibot2) m_iCollector++;

    //send once the signals
    int j = 0;

    for(int i = 0 ; i < m_iDevicesNetwork - 1; i++){ //<------ //9

        //add if the get is "caido" or "indeterminado"
        if(m_sCheck[i] == m_lStateGet[1] || m_sCheck[i] == m_lStateGet[2]){
            //qDebug() << "No Complete the array in : " << i;
            j++;
        } else if(m_sCheck[i] == m_lStateGet[0]) {
            //si un get vuelve a ser detectado, que salga de la lista de gets caidos
            get = m_lNameGets[i];
            dataGet = m_qSettings->value("GET/"+ get).toString();
            actualGet = get + "->" + dataGet;
            if(m_lGetFall.contains(actualGet)){
                m_lGetFall.removeOne(actualGet);
                //emit stopTimer();
            }
        }
    }    

    //try connect to get fallen
    if(m_iCollector > (m_iDevicesNetwork - 1)*(m_iCyclesDetection - 1) && m_iCollector < (m_iDevicesNetwork - 1)*m_iCyclesDetection){
        for(int i = 0 ; i < m_iDevicesNetwork - 1; i++){ //<------ //9  //here
            //qDebug() << m_lNameGets[i] << "is" <<m_sCheck[i];
            if(m_sCheck[i] == m_lStateGet[1]){
                QByteArray data("S");
                m_API.sendTransmitRequest(data,data.size(),m_lmyXbeeList[i],5);
                //qDebug() << " Try Connect: " << m_lNameGets[i];
            }
        }
    }

    emit receive_data(m_iTimeSleep);
    //qDebug() << "start timer sleep" << 0;

    //qDebug() << "j" << j;

    //indicate if all the array is true when j = 0 (all Gets are in network)
    if(j == 0){
        emit change_check();
        //reset state gets in general
        qDebug() << "All in the network";
        for(int i = 0 ; i < m_iDevicesNetwork - 1; i++){
            m_sCheck[i] = m_lStateGet[1];
        }
        m_iCollector = 0;
    } else if(m_iCollector == (m_iDevicesNetwork - 1)*m_iCyclesDetection/*27 --9*6*//*(m_iDevicesNetwork - 1)*3*/ && j!= 0){ //<------ //93 //cyles
        //System is sleeping
//        if( j >= m_iThresholdSleep){ //en el config
//            system_state(m_qStatesSleep[3]);
//            qDebug() << "The system is sleeping";
//            emit change_sleeping();
//            return;
//        }

        int cont = 0;
        int cont2 = 0;
        //qDebug() << "ok0";
        //detect the gets fallen
        for(int i = 0 ; i < m_iDevicesNetwork - 1; i++){ //<------ //9  //here
            if(m_sCheck[i] == m_lStateGet[1]){ //se compara con el caido , aqui se podria agregar al que no se encuentra
                //qDebug() << "ok1";
                system_state(m_qStatesSleep[2]);
                //detect the date, get and mac of the Get fallen
                QString date_PosNes = QDateTime::currentDateTime().toString("yyyy-MMMM-dd hh:mm:ss");
                get = m_lNameGets[i];
                dataGet = m_qSettings->value("GET/"+ get).toString();
                actualGet = get + "->" + dataGet;
                read();
                if(!m_lGetFall.contains(actualGet)){
                    write(date_PosNes + "->" + actualGet);
                    m_lGetFall.append(actualGet);
                }

                //qDebug() << "ok2";
                //qDebug() << "cont" << cont;
                //para que envie solo una vez el signal y asi se plotee solo una vez
                if(cont == 0){

                    //qDebug() << "ok3";
                    for(int i = 0 ; i < m_iDevicesNetwork - 1; i++){
                        m_sCheckFinal[i] = m_sCheck[i];
                    }

                    //emit the signal and with getcheck evalue the state from the vector "m_sCheck"(here is the info of the get that is in place and the one that falls)
                    emit change_check();

                    QString result;
                    for(int i = 0; i < m_iDevicesNetwork - 1; i++){
                        if(m_sCheck[i] == m_lStateGet[0]){
                            result.append("1");
                        } else if(m_sCheck[i] == m_lStateGet[1]){
                            result.append("0");
                        } else if(m_sCheck[i] == m_lStateGet[2]){
                            result.append("-");
                        }
                    }
                    qDebug() << result;
                    //-------------------------------------------
                    qDebug() << "0 segundos";
                    emit change_check2(m_iTimeOutGet);

                    //put the the gets fallen , in state "indeterminado" after 'x' time
                    for(int i = 0 ; i < m_iDevicesNetwork - 1; i++){

                        if(m_sCheckFinal[i] == m_lStateGet[1]){
                            m_sCheckFinal[i] = m_lStateGet[2];
                        }
                        //qDebug() << "Get" << i << "<- "<< m_sCheckFinal[i];
                    }

                    //reset state gets in general
                    for(int i = 0 ; i < m_iDevicesNetwork - 1; i++){
                        if(m_sCheckFinal[i] == m_lStateGet[2]){
                            m_sCheck[i] = m_lStateGet[2];
                        } else {
                            m_sCheck[i] = m_lStateGet[1];
                        }
                    }
                    cont++;
                }
            } else if (m_sCheck[i] == m_lStateGet[2]){
                if(cont2 == 0){
                    emit change_check();
                    QString result;
                    for(int i = 0; i < m_iDevicesNetwork - 1; i++){
                        if(m_sCheck[i] == m_lStateGet[0]){
                            result.append("1");
                        } else if(m_sCheck[i] == m_lStateGet[1]){
                            result.append("0");
                        } else if(m_sCheck[i] == m_lStateGet[2]){
                            result.append("-");
                        }
                    }
                    qDebug() << result;
                    cont2++;
                }
            }
            //qDebug() << "ValueFallen" << i << m_sCheck[i];
        }
        m_iCollector = 0;
    }
}

//stado de los gets, en su lugar , caido o incierto
void Hander::statusGet()
{

    qDebug() << "paso" << m_iTimeOutGet/1000 << "segundos";
    emit stopTimer();
    //emit change_getState();//plotear el cambio

}

void Hander::statusSleep()
{
    qDebug() << "paso" << m_iTimeSleep/1000 << "segundos";
    system_state(m_qStatesSleep[3]);
    qDebug() << "The system is sleeping";
    emit change_sleeping();
    emit stopTimerSleep();
}

void Hander::system_state(QString state){
    m_bSystemSleeping = state;
}

QString Hander::read_systemstate(){
    return m_bSystemSleeping;
}

void Hander::write(QString message)
{
    if (!m_qFilPosNeg.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Append))
        return;

    if (m_qFilPosNeg.isOpen() && m_qFilPosNeg.isWritable()) {
        QTextStream logStream(&m_qFilPosNeg);
        logStream.setCodec("UTF-8");
        logStream << message + '\n';
    }
    m_qFilPosNeg.close();
}

void Hander::read()
{
    if (!m_qFilPosNeg.open(QIODevice::ReadOnly | QIODevice::Text))
        return;

    QString line = "";
    QString get = "";

    while (!m_qFilPosNeg.atEnd()) {
        line = m_qFilPosNeg.readLine();
        line = line.trimmed();
        get = line.split("->").at(1) + "->" + line.split("->").at(2);
        if(!m_lGetFall.contains(get)){
            m_lGetFall.append(get);
        }
    }

    //qDebug() << m_lGetFall;

    m_qFilPosNeg.close();

}

QString Hander::getCheck()
{
    return m_sCheck2;
}

QString Hander::getCheckFinal()
{
    return m_sCheck2Final;
}

QString Hander::setCheck(int i)
{
    m_sCheck2 = m_sCheck[i];
    return m_sCheck2;
}

QString Hander::setCheckFinal(int i)
{
    m_sCheck2Final = m_sCheckFinal[i];
    return m_sCheck2Final;
}

void Hander::getXbeeList(QList<QByteArray> Xbeelist)
{
    // Get Xbee List
    m_lmyXbeeList = Xbeelist;
    qDebug() << m_lmyXbeeList;
    // Save Settings
    saveSettings();
    m_bScanStatus = true;
    qDebug() << "Escaneado terminado";
}

void Hander::errorScan()
{
    m_bScanStatus = false;
}

void Hander::updateProgressBarCounter(int count)
{
    if (m_iCurrentGet != count){
        m_iCurrentGet = count;
        emit currentGetChanged();
    }

    double scalCounter = 100.0/(static_cast<double>(m_iDevicesNetwork)-1);
    m_iProgressBarCounter = static_cast<float>((count + 1)*scalCounter);
    emit updateProgressBar();
}

float Hander::voltageInterpreter(QByteArray payload)
{
    // Local variables
    int c;
    float d;
    //Calculate voltaje
    c = ((uint8_t)payload[0] << 8 )|( (uint8_t)payload[1] );
    d = (((1200 * c) + 512 )/1024) * 0.001;
    return d;
}

QString  Hander::accInterpreter(QByteArray payload)
{
    // Local variables
    int16_t ax, ay, az;
    QList<QString> valor[3];

    // Perform data conversion to int
    ax = (payload.at(1)& 0x0F) << 8 | payload.at(0);
    ay = (payload.at(3)& 0x0F) << 8 | payload.at(2);
    az = (payload.at(5)& 0x0F) << 8 | payload.at(4);

    if (ax > 2047) ax += -4096;
    if (ay > 2047) ay += -4096;
    if (az > 2047) az += -4096;

    // Return QString  //BMA's axes
    QString status = QString("%1, %2, %3")//ax,ay,az
            .arg(ax).arg(ay).arg(az);
    return status;
}

float Hander::getProgressBarCounter()
{
    return m_iProgressBarCounter;
}

void Hander::setData(int i)
{

    if(!m_lmyXbeeList.empty() && i < m_iDevicesNetwork - 1){
        if(m_lmyVoltList[i] <= 1 || m_lmyVoltList[i] > 5){
            m_iVoltage = 0;
        } else {
            m_iVoltage = m_lmyVoltList[i];
        }
        //qDebug() << m_iVoltage;

        //qDebug() << "Voltage" << m_iVoltage;
        if(m_lmyXbeeList[i] != ""){
            m_sMac = mScan->qByte2ASCII(m_lmyXbeeList[i], 8);
        } else {
            m_sMac = "";
        }
        //qDebug() << "MAC" << m_sMac;
        if(m_lmyAccDataList[i] != ""){
            m_sAccData = m_lmyAccDataList[i];
        } else if(/*m_lmyAccDataList[i] == "" && setCheck(i) == false &&*/ read_systemstate() == m_qStatesSleep[2]){
            m_sAccData = "Get Caido!";
        } else if (m_lmyAccDataList[i] == "" && read_systemstate() == m_qStatesSleep[3]){
            m_sAccData = "En reposo";
        } else if(read_systemstate() == m_qStatesSleep[0]){
            m_sAccData = "Sin inicializar";
        } else {
            m_sAccData = "";
        }
        //qDebug() << "read" << read_systemstate();
        //qDebug() << "put" << m_qStatesSleep[1];

        qDebug() << "Data has been received from GET: " << i;
        emit dataChanged();
    }

}

QList<QString> Hander::name_ports()
{
    //    QList<QString> valor[3];
    qDebug() << "Ports found";
    return m_API.ports();
}

void Hander::list_ports()
{    
    m_ListPort = QVariant::fromValue(m_API.ports());
    //qDebug() << "emitiendo ports";
    emit listPortChanged();
}

void Hander::close_port()
{
    m_API.close();
    qDebug() << "Port close";
}

void Hander::configurationXbee()
{
    QByteArray m_bConf[2] = {"SM","AP"};
    const char m_bConfValue[2] = {0, 2};//0 No Sleep 1 Sleep, 1 Without Scapes 2 With Scapes;
    int m_iSizeConf = sizeof(m_bConfValue);

    if (m_lmyXbeeList.size() == 0)
        return;

    if(ubication_port() != ""){
        for(int i = 0; i < m_iSizeConf; i++){
            qDebug() << "Slaves Xbees was configured with: " << m_bConf[i] << " = " << int(m_bConfValue[i]);
            for(int j = 0 ; j < m_lmyXbeeList.size() ; j++){
                m_API.sendRemoteAT(m_bConf[i],m_bConfValue[i],1,m_lmyXbeeList[j],1);
            }
        }
    } else {
        qDebug() << "Error configuring Xbees";
    }
}

void Hander::sendSleepCommand()
{
    //if(!m_lmyXbeeList.empty()){

    qDebug() << "size is:" << m_lmyXbeeList.size();
    QByteArray data("S");
    for(int i = 0 ; i < m_lmyXbeeList.size() ; i++){

        m_API.sendTransmitRequest(data,data.size(),m_lmyXbeeList[i],i+1);
        //qDebug() << m_lmyXbeeList[i];

        qDebug() << "Send Command Sleep to XBees Slaves";

    }

    for(int i = 0 ; i < 1000000000 ; i++){

    }

    qDebug() << "get transmit : " << m_API.getTransmitRequestStatus();
    qDebug() << "sendSleepCommand";
    //}
}

void Hander::sendResetCommand()
{
    QByteArray data("aaa");
    for(int j = 0; j < 40; j++){
        for(int i = 0 ; i < m_lmyXbeeList.size() ; i++){
            m_API.sendTransmitRequest(data,data.size(),m_lmyXbeeList[i],5);
            qDebug() << "Send Command Reset to XBees Slaves";
        }
    }
}

void Hander::prube()
{
}

void Hander::prube1(){

}

void Hander::saveSettings()
{
    //QSettings settings("qsettings.ini", QSettings::IniFormat);//.ini for windows and any applications of GNU/LINUX
    //settings.setValue("GET1","\0\x13\xa2\0\x41\xb7W\x96");

    QString hextoString;
    //    QByteArray valueHex2;

    qDebug() << "Save MAC of scanned devices:" << m_iDevicesNetwork;

    if (m_lmyXbeeList.size() == 0)
        return;

    for(int i = 0; i < m_iDevicesNetwork - 1; i++){
        hextoString = QString::fromLatin1(m_lmyXbeeList[i].toHex());
        //        settings.setValue(m_lNameGets[i], hextoString);
        m_qSettings->setValue("GET/" + m_lNameGets[i], hextoString);
        //qDebug() << m_sHextoString;
//        qDebug() << QString("%1: %2").arg(m_lNameGets[i]).arg(hextoString);
        //settings.setValue(m_lNameGets[i],m_lmyXbeeList[i]);
        //qDebug() << "QString?"<<m_lmyXbeeList[i];
    }

    m_qSettings->setValue("PortCom", m_qPort);

    qDebug() << "Save finished";
}

bool Hander::getScanStatus()
{
    return m_bScanStatus;
}

QString Hander::ubication_port()
{
    QString ubication = "";
    QString ubication_port =  m_qSettings->value("PortCom").toString();
    //qDebug() << m_API.ports().size() << "and" << ubication_port;
    for(int i = 0; i < m_API.ports().size(); i++){
        if(m_API.ports()[i] == ubication_port){
            ubication = ubication_port;
        }
    }

    //qDebug() << "this is the ubication" << ubication;

    return ubication;
}

QString Hander::DevicesIni()
{
    //qDebug() << "DevicesIni: "<< m_qSettings->value("Devices").toString();
    int devIni = m_qSettings->value("Devices").toInt();
    return QString::number(devIni - 1);

    //return m_qSettings->value("Devices").toString();

}

QString Hander::AttemptIni()
{
    //qDebug() << "AttemptIni: "<< m_qSettings->value("Attempt").toString();
    return m_qSettings->value("Attempt").toString();
}

QString Hander::installIni()
{
    QString install = m_qSettings->value("Install").toString();
    if(install != ""){
        return install;
    } else {
        return "No establecido";
    }
}

void Hander::setDevicesIni(QString DeviceIni)
{
    m_qSettings->setValue("Devices", DeviceIni.toInt()+1);
    //m_qSettings->setValue("Devices", DeviceIni.toInt());
    //m_iDevicesNetwork = mScan->getNumberOfDevices();
    m_iDevicesNetwork = m_qSettings->value("Devices").toInt();    
    loadSettings(false);
    //qDebug()  << "device" << m_iDevicesNetwork;
}

void Hander::setAttemptIni(QString dataComIni)
{
    m_qSettings->setValue("Attempt", dataComIni);
    //qDebug()  << "attemptIni" << dataComIni;
}

//void Hander::setInstallIni(QString dataInstallIni)
//{
//   m_qSettings->setValue("Install",dataInstallIni);
//}

QString Hander::setDateInstallIni()
{

    QString dateInstallIni = QDateTime::currentDateTime().toString("dd/MM/yyyy");
    m_qSettings->setValue("Install",dateInstallIni);
//    QDate dateFormate = QDate::fromString(dateInstallIni,"dd/MM/yyyy");
//    QString dateFinal = dateFormate.toString("dd 'de' MM 'del' yyyy");
    return dateInstallIni;
}

void Hander::setStatus(bool status)
{
    if (status == m_bStatus)
        return;
    //QSettings settings2("qsettings2.ini", QSettings::IniFormat);//.ini for windows and any applications of GNU/LINUX
}

void Hander::loadSettings(bool loadProgram)
{
    //    QSettings settings("qsettings.ini", QSettings::IniFormat);//.ini for windows and any applications of GNU/LINUX
    if(!m_lmyXbeeList.empty()){
        m_lmyXbeeList.clear();
        //  m_lmyXbeeList2.clear();
    }

    QString value;
    QByteArray valueHex;
    //    QString showResult;

    qDebug() << "Upload the data of the MAC's saved in the interface";
    m_iDevicesNetwork = m_qSettings->value("Devices").toInt();
    qDebug() << "Devices: "<< m_qSettings->value("Devices").toString();

    for(int i = 0; i < m_iDevicesNetwork - 1; i++){

        value = m_qSettings->value("GET/"+m_lNameGets[i]).toString();   //set the Settings' value
        qDebug() << "is <b>" << value;
        valueHex = QByteArray::fromHex(value.toLatin1()); //Transform the values in Hex
        m_lmyXbeeList.append(valueHex);

    }

    if(loadProgram == true){
        //connecting the port from the file .ini
        QString port_ini = m_qSettings->value("PortCom").toString();

        if(ubication_port() != ""){
            qDebug()<<"PORT COM from .ini" << port_ini << "connected";
            init("COM11");
            //configurationXbee();
        } else {
            qDebug() << "Error connecting to port" << port_ini;
        }
    }

    qDebug() << "Upload finished";
}

//void Hander::closeEvent(QCloseEvent *event)
//{
//    //void Hander::closeEvent(QCloseEvent *event){
//    qDebug() << "The programa is closing";
//    saveSettings();
//    event->accept();
//}
