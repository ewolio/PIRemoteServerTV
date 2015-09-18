#ifndef OMXPLAYERS_H
#define OMXPLAYERS_H

#include <QObject>
#include "piremoteserver/piremoteserver.h"

class Window;

class Screen: public App
{
    Q_OBJECT
public:
    Screen(QObject *parent = 0);
    ~Screen();

    bool isScreenOn(){return _on;}
    int getVolume(){return generalVolume;}
signals:
    void generalVolumeChanged();

public slots:
    void setVolume(QString volume);
    void setVolume(int volume);

    void setScreenOn(QString on);
    void setScreenOn(bool on);

    void addWindow(QString w);
    void addWindow(Window* w);

    void sendToWindow(QString message);

protected:
    int generalVolume;
    bool _on;

    QList<Window*> windows;
    QList<QThread*> windowsThread;
};

class Window: public QObject{
    Q_OBJECT
public:
    Window(QString windowType);
    void registerWindow(Screen* screen, int windowID);

    QString getWindowType(){return _type;}
    int getWindowID(){return windowID;}

    bool isFullscreen(){return _fullscreen;}
    int x1(){return _x1;}
    int x2(){return _x2;}
    int y1(){return _y1;}
    int y2(){return _y2;}

    int getVolume(){return _volume;}
    int getRealVolume(){return screen==NULL?_volume:_volume + screen->getVolume();}

public slots:
    void setVolume(int volume);
    void updateVolume();

    void messageReceived(QString message);

protected:
    virtual void onDimensionChanged(){}
    virtual void onVolumeChanged(){}
    virtual void onInstructionReceived(QString instruction){}

    Screen *screen;

private:
    int _x1, _x2, _y1, _y2;
    bool _fullscreen;
    int windowID;

    QString _type;
    int _volume;
};

class OmxPlayer: public Window{
    Q_OBJECT
public:
    OmxPlayer();
    ~OmxPlayer();

    bool isPlaying(){return _omxplayer->state()==QProcess::Running;}
public slots:
    void setStream(QString streamName);

    void play(int startTime=0);
    void togglePause();
    void stop();

    void startPlayer();
    void reloadPlayer();

    void playerStopped();

    void volumeUp();
    void volumeDown();
    void forward();
    void backward();

protected:
    void onVolumeChanged();
    void onDimensionChanged();
    void onInstructionReceived(QString instruction);

    QString _stream;

    int _startingTime, previousVolume;

    QProcess* _omxplayer;
    QProcess* _sidePlayerSoftware;

    QString _sidePlayerCmd;
};


#endif // OMXPLAYERS_H
