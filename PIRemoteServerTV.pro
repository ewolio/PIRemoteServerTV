QT += core network
QT -= gui

target.path = /home/pi
INSTALLS += target

CONFIG += console
CONFIG -= app_bundle

TEMPLATE = app

SOURCES += main.cpp \
    piremoteserver/piremoteserver.cpp \
    piremoteserver/appinterface.cpp \
    xdotoolinterface.cpp \
    screen.cpp

HEADERS += \
    piremoteserver/piremoteserver.h \
    piremoteserver/appinterface.h \
    xdotoolinterface.h \
    screen.h

