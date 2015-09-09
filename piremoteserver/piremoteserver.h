#ifndef REMOTESERVER_H
#define REMOTESERVER_H

#include <QObject>
#include <QtNetwork>

class Socket;

class PIRemoteServer : public QObject
{
    Q_OBJECT
public:
    explicit PIRemoteServer(QObject *parent = 0);

signals:

    void received(QString message);

public slots:
    void newConnection();
    void socketDisconnected(int idSocket);

    void sendTo(QString message, int idSocket);
    void send(QString message);

    void messageReceived(QString message, int idSocket);

protected:
    QTcpServer* server;


private:
    quint16 port;

    QMap<int, Socket* > clients;


};

class Socket: public QObject{

    Q_OBJECT

public:
    Socket(int idSocket, PIRemoteServer *server, QTcpSocket* socket);

    QString getName(){return name;}
    int getID(){return socketID;}

public slots:
    void send(QString message);
    void disconnectFromHost(){socket->disconnectFromHost();}

signals:
    void messageReceived(QString message, int idSocket);
    void connexionLost(int idSocket);

protected slots:
    void readData();
    void emitConnexionLost(){emit connexionLost(socketID);}
private:
    QString name;
    int socketID;

    QTcpSocket* socket;
};

#endif // REMOTESERVER_H
