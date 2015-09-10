#ifndef XDOTOOLINTERFACE_H
#define XDOTOOLINTERFACE_H

#include <QObject>
#include "piremoteserver/piremoteserver.h"

class XdotoolInterface : public App
{
    Q_OBJECT
public:
    explicit XdotoolInterface(QObject *parent = 0);

signals:

public slots:
    void moveMouse(QString arg);
    void click(QString);
};

#endif // XDOTOOLINTERFACE_H
