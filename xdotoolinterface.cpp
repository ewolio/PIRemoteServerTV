#include "xdotoolinterface.h"

XdotoolInterface::XdotoolInterface(QObject *parent) : App("xdotool", false, parent)
{
    add(new SlotAction("moveMouseRelative", this, "moveMouse"));
    add(new SlotAction("click", this, "click"));
}

void XdotoolInterface::moveMouse(QString arg)
{
    QProcess().execute("xdotool mousemove_relative -- "+arg);
    qWarning()<<"execute: mousemove_relative "<<arg;
}

void XdotoolInterface::click(QString)
{
    QProcess().execute("xdotool click 1");
}

