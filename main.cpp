#include <QCoreApplication>
#include <QNetworkInterface>

#include "piremoteserver/piremoteserver.h"

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    PIRemoteServer* server = new PIRemoteServer();

    qWarning()<<"PIRemoteServer 0.0";
    qWarning()<<QNetworkInterface().allAddresses().at(2).toString();


    return a.exec();
}

