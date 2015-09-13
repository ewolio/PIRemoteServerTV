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

    bool isScreenOn(){return _on;}
    int getVolume(){return generalVolume;}
signals:

public slots:
    void setVolume(QString volume);
    void setVolume(int volume);

    void setScreenOn(QString on);
    void setScreenOn(bool on);


protected:
    int generalVolume;
    bool _on;

    QList<Window*> windows;
};

class Window: public QObject{
    Q_OBJECT
public:
    Window(QString windowType, Screen* screen);

    QString getWindowType(){return _type;}

    bool isFullscreen(){return _fullscreen;}
    int x1(){return _x1;}
    int x2(){return _x2;}
    int y1(){return _y1;}
    int y2(){return _y2;}

    void setVolume(int volume);
    void updateVolume();
    int getVolume(){return _volume;}

    int getRealVolume(){return _volume + screen->getVolume();}

protected:
    virtual void onDimensionChanged(){}
    virtual void onVolumeChanged(int previousRealVolume){}

    Screen *screen;

private:
    int _x1, _x2, _y1, _y2;
    bool _fullscreen;


    QString _type;
    int _volume, previousVolume;
};

class OmxPlayer: public Window{
    Q_OBJECT
public:
    OmxPlayer(Screen* screen);

    bool isPlaying(){return _isPlaying;}
public slots:
    void setStream(QString streamName);

    void play(int startTime=0);
    void togglePause();
    void stop();
    void volumeUp();
    void volumeDown();

    void forward();
    void backward();

protected:
    void onVolumeChanged(int previousVolume);
    void onDimensionChanged();

    QString _stream;
    bool _isPlaying;

    QProcess* _omxplayer;
};


#endif // OMXPLAYERS_H
