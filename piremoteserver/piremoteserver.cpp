#include "piremoteserver.h"

PIRemoteServer::PIRemoteServer(QObject *parent) : QObject(parent)
{
    server = new QTcpServer(this);
    port = 31415;

    connect(server, SIGNAL(newConnection()), this, SLOT(newConnection()));

    server->listen(QHostAddress::Any, port);
}

void PIRemoteServer::newConnection()
{
    int id = 0;
    if(!clients.isEmpty())
        id = clients.lastKey()+1;

    Socket* socket = new Socket(id, this, server->nextPendingConnection());
    connect(socket, SIGNAL(connexionLost(int)), this, SLOT(socketDisconnected(int)));
    connect(socket, SIGNAL(messageReceived(QString,int)), this, SLOT(messageReceived(QString,int)));

    clients.insert(id, socket);


    qWarning()<<"new connexion attempt...";
}


void PIRemoteServer::socketDisconnected(int idSocket)
{
    clients.value(idSocket)->disconnectFromHost();
    qWarning()<<clients.value(idSocket)->getName()<<" disconnected";
    delete clients.value(idSocket);
    clients.remove(idSocket);
}

void PIRemoteServer::sendTo(QString message, int idSocket)
{
    if(!clients.contains(idSocket))
        return;

    clients.value(idSocket)->send(message);
}

void PIRemoteServer::send(QString message)
{
    QList<int> keys = clients.keys();

    foreach (int id, keys) {
        sendTo(message, id);
    }
}

void PIRemoteServer::messageReceived(QString message, int idSocket)
{
    emit received(message);
}


//***********************************************************************
//***************************    Socket    ******************************
//***********************************************************************

Socket::Socket(int idSocket, PIRemoteServer *server, QTcpSocket *socket):QObject(server)
{
    socketID = idSocket;
    name = "";
    this->socket = socket;
    connect(socket, SIGNAL(disconnected()), this, SLOT(emitConnexionLost()));
    connect(socket, SIGNAL(readyRead()), this, SLOT(readData()));
}

void Socket::send(QString message)
{
    if(name=="")
        return;

    socket->write(QString(message + "\n").toUtf8());
}

void Socket::readData()
{
    while(socket->canReadLine()){
        QString d = QString::fromUtf8(socket->readLine()).trimmed();
        qWarning()<<"message: "<<d;

        if(name==""){
            if(d.startsWith("register")){
                name = d.remove("register ");
                socket->write("registered\n");
                qWarning()<<name<<" registered!";
            }
        }else{
            emit(messageReceived(d, socketID));
        }
    }

}
