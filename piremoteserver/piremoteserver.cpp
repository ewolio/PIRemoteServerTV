#include "piremoteserver.h"

PIRemoteServer* PIRemoteServer::piRemoteServer = NULL;

PIRemoteServer::PIRemoteServer(): QObject()
{
    qWarning()<<"PIRemoteServer 0.0";
    _interface = new RemoteServerInteface(this);

    // -----   App    ------

    addApp("System",true)->add(new ProcessAction("exec" ,"%1"));


}

//-------- Interface control  ----------------

void PIRemoteServer::dataReceived(quint8 dataType, QByteArray data, Socket *socket)
{
    qWarning()<<socket->getName()<<": "<<dataType<<"|"<<data;

    switch(dataType){
    case CHAR:         //char*
        qWarning()<<socket->getName()<<": "<<data.constData();
        break;
    case STRING:         //QString utf8
        qWarning()<<socket->getName()<<": "<<QString::fromUtf8(data);
        break;
    }

    if(dataType >= APPS_SYNC){
        //Apps
        if(dataType==APPS_SYNC){
            if(data.startsWith('i')){ //apps tree request
                qWarning()<<"sending interface";
                _interface->sendTo(APPS_SYNC, appStructure().toUtf8(), socket);
            }
        }else{
            quint8 appID = dataType - APPS;
            appTrigger(socket->getID(), appID, (quint8)data.at(0), data.right(data.size()-1));
        }

    }else if(dataType>=MODELS_SYNC){
        //Model modif

    }
}



//--------- App control  ---------------------
bool PIRemoteServer::registerApp(App *app)
{

    if(appsList.size()>= 255-APPS) //limitation du nombre d'app par le protocole de communication
        return false;
    if(!app->registeredAs((quint8)appsList.size()))
        return false;

    connect(app, SIGNAL(returnStatement(int,quint8,quint8,QString)), this, SLOT(appReplied(int,quint8,quint8,QString)));
    appsList.append(app);
    return true;
}

QString PIRemoteServer::appStructure()
{
    QString r;
    for (int i = 0; i < appsList.size(); ++i)
        r+=appsList.at(i)->toString()+"\n";

    return r;
}
App *PIRemoteServer::addApp(QString appName, bool verbose)
{
    App* app = new App(appName, verbose, this);
    registerApp(app);
    return app;
}

bool PIRemoteServer::appTrigger(int socketID, quint8 appID, quint8 actionID, QString args)
{
    if(appID >= appsList.size())
        return false;

    return appsList.at(appID)->trigger(socketID, actionID, args);
}

void PIRemoteServer::appReplied(int socketToReply, quint8 appID, quint8 actionID, QString statement)
{
    _interface->sendTo(APPS+appID, actionID + statement.toUtf8(), socketToReply);
}





/***************************************************************
 * ********************  RemoteServerInteface ******************
 * ************************************************************/


RemoteServerInteface::RemoteServerInteface(QObject *parent) : QObject(parent)
{
    server = new QTcpServer(this);
    port = 31415;

    connect(server, SIGNAL(newConnection()), this, SLOT(newConnection()));
    qWarning()<<"LAN:"<<QNetworkInterface().allAddresses();
    server->listen(QHostAddress::Any, port);

}

void RemoteServerInteface::newConnection()
{
    int id = 0;
    if(!clients.isEmpty())
        id = clients.lastKey()+1;

    Socket* socket = new Socket(id, this, server->nextPendingConnection());
    connect(socket, SIGNAL(connexionLost(int)), this, SLOT(socketDisconnected(int)));
    connect(socket, SIGNAL(dataReceived(QByteArray,Socket*)), this, SLOT(dataReceived(QByteArray,Socket*)));

    clients.insert(id, socket);


    qWarning()<<"new connexion attempt...";
}


void RemoteServerInteface::socketDisconnected(int idSocket)
{
    clients.value(idSocket)->disconnectFromHost();
    qWarning()<<clients.value(idSocket)->getName()<<" disconnected";
    delete clients.value(idSocket);
    clients.remove(idSocket);
}

void RemoteServerInteface::sendTo(QString message, int idSocket)
{
    if(!clients.contains(idSocket))
        return;

    clients.value(idSocket)->send(STRING+message.toUtf8());
}

void RemoteServerInteface::send(QString message)
{
    QList<int> keys = clients.keys();

    foreach (int id, keys) {
        sendTo(message, id);
    }
}

void RemoteServerInteface::sendTo(quint8 dataType, QByteArray data, int idSocket)
{
    if(!clients.contains(idSocket))
        return;

    sendTo(dataType, data, clients.value(idSocket));
}

void RemoteServerInteface::sendTo(quint8 dataType, QByteArray data, Socket *socket)
{
    qWarning()<<"Sending ("<<socket->getName()<<"): "<<dataType<<"|"<<data;
    data.insert(0, dataType);
    socket->send(data);
}

void RemoteServerInteface::send(quint8 dataType, QByteArray data)
{
    QList<int> keys = clients.keys();

    foreach (int id, keys) {
        sendTo(dataType, data, id);
    }
}

void RemoteServerInteface::dataReceived(QByteArray data, Socket* socket)
{
    quint16 dataType = data.at(0);
    data = data.right(data.size()-1);

    PIRServer->dataReceived(dataType, data, socket);
}


//***********************************************************************
//***************************    Socket    ******************************
//***********************************************************************

Socket::Socket(int idSocket, RemoteServerInteface *server, QTcpSocket *socket):QObject(server)
{
    socketID = idSocket;
    name = "";
    blockSize = 0;

    this->socket = socket;
    connect(socket, SIGNAL(disconnected()), this, SLOT(emitConnexionLost()));
    connect(socket, SIGNAL(readyRead()), this, SLOT(readData()));
}

void Socket::send(QByteArray data)
{
    if(name=="")
        return;

    QByteArray block;
    QDataStream out(&block, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_0);

    out<<(quint16)data.size();
    out<<data;

    socket->write(block);
}

void Socket::readData()
{
    while(1){
    QDataStream in(socket);
    in.setVersion(QDataStream::Qt_4_0);

    if (blockSize == 0) {
        if (socket->bytesAvailable() < (int)sizeof(quint16))
            return;

        in >> blockSize;
    }

    if (socket->bytesAvailable() < blockSize)
        return;



    QByteArray data;
    in >> data;

    if(name==""){
        if(data.startsWith('P')){
            name = QString::fromUtf8(data.remove(0,1));
            send(QByteArray("PI"));
            qWarning()<<name<<" registered!";
        }
    }else{
        emit(dataReceived(data, this));
    }

    blockSize = 0;
    }
}


