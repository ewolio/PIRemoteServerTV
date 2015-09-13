#include <QCoreApplication>
#include <QNetworkInterface>

#include "xdotoolinterface.h"
#include "screen.h"
#include "piremoteserver/piremoteserver.h"

//sudo mount -o loop,offset=62914560 /home/gaby/Installation/RaspberryPi/raspbienQt.img /mnt/rasp-pi-rootfs/

void test(QString t){
    qWarning()<<"test"<<t;
}


int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    //PIRemoteServer* server = PIRemoteServer::server();
    PIRemoteServer::initServer();

    PIRServer->registerApp(new XdotoolInterface());
    PIRServer->registerApp(new Screen());



    return a.exec();
}



