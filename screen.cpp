#include "screen.h"

Screen::Screen(QObject *parent): App("omxPlayers", true, parent)
{
    generalVolume = 0;
}

void Screen::setVolume(QString volume){
    bool ok;
    int v = volume.toInt(&ok);
    if(ok)
        setVolume(v);
}

void Screen::setVolume(int volume){
    generalVolume = volume;
    foreach(Window* w, windows)
        w->updateVolume();
}

void Screen::setScreenOn(QString on){
    if(on.contains("on"))
        setScreenOn(true);
    else if(on.contains("off"))
        setScreenOn(false);
}

void Screen::setScreenOn(bool on){
    if(on == _on)
        return;

    if(on){
        QProcess().execute("tvservice -o");
    }else{
        QProcess().execute("tvservice -p");
    }

    _on = on;
}


/**************************************************
 *                     Window                     *
 * ***********************************************/

Window::Window(QString windowType, Screen *screen): QObject(screen)
{
    _type = windowType;
    this->screen = screen;
    _x1=_x2=_y1=_y2=-1;
    _fullscreen = true;
    previousVolume = 0;
    _volume = 0;
}

void Window::setVolume(int volume){
    previousVolume=  getRealVolume();
    _volume = volume;

    updateVolume();
}

void Window::updateVolume()
{
    onVolumeChanged(previousVolume);
    previousVolume = _volume;
}


/**************************************************
 *                    OmxPlayer                   *
 * ***********************************************/

OmxPlayer::OmxPlayer(Screen *screen): Window("omx", screen)
{

}

void OmxPlayer::setStream(QString streamName){
    _stream = streamName;

    if(isPlaying()){
        stop();
        play();
    }
}


void OmxPlayer::play(int startTime){
    if(isPlaying())
        return;

    QString cmd = "omxplayer "+_stream+" --live --vol "+QString().setNum(getRealVolume()*300)+" ";

    if(!isFullscreen())
        cmd += "--win \""+QString().setNum(x1())+" "+QString().setNum(x2())+" "+QString().setNum(y1())+" "+QString().setNum(y2())+"\" ";

    if(startTime!=0){
        int h=0, m=0, s=0;
        h = startTime/3600;
        m = startTime/60%60;
        s = startTime%60;
        cmd += "--pos ";
    }

    _omxplayer->execute(cmd);
    _isPlaying = true;
}

void OmxPlayer::togglePause(){
    if(isPlaying())
        _omxplayer->write("p");
}
void OmxPlayer::stop(){
    if(isPlaying())
        _omxplayer->write("q");
}

void OmxPlayer::onVolumeChanged(int previousVolume){
    if(!isPlaying())
        return;



    while(previousVolume != getRealVolume())
        if(previousVolume<getRealVolume()){
            previousVolume++;
            _omxplayer->write("+");
        }else{
            previousVolume--;
            _omxplayer->write("-");
        }
}

void OmxPlayer::onDimensionChanged()
{
    if(!isPlaying())
        return;
    stop();
    play();

    ///TODO relancer au bon instant
}

void OmxPlayer::volumeUp(){
    setVolume(getVolume()+1);
}
void OmxPlayer::volumeDown(){
    setVolume(getVolume()-1);
}

void OmxPlayer::forward(){
    if(isPlaying())
        _omxplayer->write("^[[C");
}
void OmxPlayer::backward(){
    if(isPlaying())
        _omxplayer->write("^[[D");
}
