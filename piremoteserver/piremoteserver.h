#ifndef REMOTESERVER_H
#define REMOTESERVER_H

#include <QObject>
#include <QtNetwork>
#include "appinterface.h"

#define PIRServer PIRemoteServer::server()

#define CHAR 0x00
#define STRING 0x01
#define APPS 0xC9
#define APPS_SYNC 0xC8
#define MODELS 0x65
#define MODELS_SYNC 0x64

class Socket;
class RemoteServerInteface;


class PIRemoteServer: public QObject{
    Q_OBJECT
public:
    static PIRemoteServer* server(){return piRemoteServer;}
    static void initServer(){piRemoteServer = new PIRemoteServer();}


    //Interface
    RemoteServerInteface* interface() const {return _interface;}
public slots:
    void dataReceived(quint8 dataType, QByteArray data, Socket* socket);


    //App
    App* addApp(QString appName, bool verbose = false);
    bool appTrigger(int socketID, quint8 appID, quint8 actionID, QString args="");
    bool registerApp(App* app);
public slots:
    void appReplied(int socketToReply, quint8 appID, quint8 actionID, QString statement);
private:
    QString appStructure();


protected:
    explicit PIRemoteServer();

private:
    static PIRemoteServer* piRemoteServer;

    QList<App*> appsList;

    RemoteServerInteface* _interface;

};

class RemoteServerInteface : public QObject
{
    Q_OBJECT
public:
    explicit RemoteServerInteface(QObject *parent = 0);

public slots:
    void newConnection();
    void socketDisconnected(int idSocket);

    void sendTo(QString message, int idSocket);
    void send(QString message);

    void sendTo(quint8 dataType, QByteArray data, int idSocket);
    void sendTo(quint8 dataType, QByteArray data, Socket* socket);
    void send(quint8 dataType, QByteArray data);

    void dataReceived(QByteArray data, Socket* socket);

protected:
    QTcpServer* server;


private:
    quint16 port;
    QMap<int, Socket* > clients;


};

class Socket: public QObject{

    Q_OBJECT

public:
    Socket(int idSocket, RemoteServerInteface *server, QTcpSocket* socket);

    QString getName(){return name;}
    int getID(){return socketID;}

public slots:
    void send(QByteArray data);
    void disconnectFromHost(){socket->disconnectFromHost();}

signals:
    void dataReceived(QByteArray message, Socket* socket);
    void connexionLost(int idSocket);

protected slots:
    void readData();
    void emitConnexionLost(){emit connexionLost(socketID);}
private:
    QString name;
    int socketID;

    quint16 blockSize;

    QTcpSocket* socket;
};

#endif // REMOTESERVER_H
