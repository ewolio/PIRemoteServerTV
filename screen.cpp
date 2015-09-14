#include "screen.h"

Screen::Screen(QObject *parent): App("screen", true, parent)
{
    generalVolume = 0;

    add(new SlotAction("volume", this, "setVolume"));
    add(new SlotAction("screen", this, "setScreenOn"));
    add(new SlotAction("addWindow", this,"addWindow"));
    add(new SlotAction("window", this, "sendToWindow"));
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

void Screen::addWindow(QString w)
{
    if(w.isEmpty())
        return;

    QStringList info = w.split("|");
    if(info.at(0).startsWith("omxplayer")){
        addWindow(new OmxPlayer());
        qWarning()<<"Player added!";
    }

}

void Screen::addWindow(Window *w){
    w->registerWindow(this, windows.size());
    windows.append(w);
}

void Screen::sendToWindow(QString message)
{
    int idSep = message.indexOf('|');

    int windowID = message.left(idSep).toInt();

     qWarning()<<windowID<<"->"<<message;

    if(windowID>= windows.size())
        return;

    message = message.mid(idSep+1);
    windows.at(windowID)->messageReceived(message);




}

/**************************************************
 *                     Window                     *
 * ***********************************************/

Window::Window(QString windowType): QObject()
{
    _type = windowType;
    _x1=_x2=_y1=_y2=-1;
    _fullscreen = true;
    previousVolume = 0;
    _volume = 0;
}

void Window::registerWindow(Screen *screen, int windowID)
{
    this->screen = screen;
    this->windowID = windowID;
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

void Window::messageReceived(QString message)
{
    QStringList instructions = message.split(";", QString::SkipEmptyParts);

    foreach (QString instruction, instructions) {
        if(instruction.startsWith("volume: ")){
            instruction.remove("volume:");
            instruction.trimmed();
            bool ok;
            int v = instruction.toInt(&ok);
            if(ok)
                setVolume(v);
        }else
            onInstructionReceived(message);
    }



}


/**************************************************
 *                    OmxPlayer                   *
 * ***********************************************/

OmxPlayer::OmxPlayer(): Window("omx")
{
    _startingTime = 0;
    _omxplayer = new QProcess(this);
    _sidePlayerSoftware = new QProcess(this);
    connect(_omxplayer, SIGNAL(aboutToClose()), this, SLOT(playerStopped()));

    _sidePlayerCmd = "";
    _reloading = false;
   _isPlaying = false;
}

void OmxPlayer::setStream(QString streamName){

    _stream = streamName;
    _sidePlayerCmd = "";
    qWarning()<<"stream:"<<_stream;
    if(streamName.startsWith("http://www.twitch.tv/")){
        _sidePlayerCmd = "livestream "+streamName+" best --player-external-http-port "+QString().setNum(31414-getWindowID());
        _stream = "http://localhost:"+QString().setNum(31414-getWindowID());
    }else if(streamName.startsWith("https://www.youtube.com/watch")){
        connect(_sidePlayerSoftware, SIGNAL(readyRead()), this, SLOT(youtubeDLReadyRead()));
        _sidePlayerSoftware->start("youtubedl -g "+streamName);
        qWarning()<<"executing youtubedl...";
        return;
    }

    if(isPlaying()){
        stop();
        play();
    }
}


void OmxPlayer::play(int startTime){
    if(isPlaying())
        return;

    if(_sidePlayerCmd!="")
        _sidePlayerSoftware->start(_sidePlayerCmd);

    _startingTime = startTime;

    QTimer::singleShot(500,  this, SLOT(startPlayer()));

    _isPlaying = true;
}

void OmxPlayer::togglePause(){
    if(!isPlaying())
        return;

    _omxplayer->write("p");
    qWarning()<<"togglingPause";
}
void OmxPlayer::stop(){
    if(!isPlaying())
        return;
    if(_omxplayer->isOpen())
        _omxplayer->write("q");

    _sidePlayerSoftware->kill();

    _isPlaying = false;
}

void OmxPlayer::startPlayer()
{
    QString cmd = "omxplayer "+_stream+" --live --vol "+QString().setNum(getRealVolume()*300)+" ";

    if(!isFullscreen())
        cmd += "--win \""+QString().setNum(x1())+" "+QString().setNum(x2())+" "+QString().setNum(y1())+" "+QString().setNum(y2())+"\" ";

    if(_startingTime!=0){
        int h=0, m=0, s=0;
        h = _startingTime/3600;
        m = _startingTime/60%60;
        s = _startingTime%60;
        cmd += "--pos ";
    }

    _omxplayer->start(cmd);
    qWarning()<<"start reading: "<<cmd;
}

void OmxPlayer::reloadPlayer(){
    _reloading = true;
    _omxplayer->write("q");
}

void OmxPlayer::playerStopped()
{
    if(_reloading){
        startPlayer();
        _reloading = false;
    }else
        stop();
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

   reloadPlayer();


}

void OmxPlayer::onInstructionReceived(QString instruction){
    instruction.trimmed();
    qWarning()<<"instruction:"<<instruction;
    if(instruction == "play")
        play();
    else if(instruction == "togglePause")
        togglePause();
    else if(instruction.startsWith("play:")){
        instruction.remove("play:");
        instruction.trimmed();
        bool ok;
        int time = instruction.toInt(&ok);
        if(ok)
            play(time);
    }else if(instruction == "forward")
        forward();
    else if(instruction == "backward")
        backward();
    else if(instruction == "stop")
        stop();
    else if(instruction.startsWith("stream:")){
        instruction.remove("stream:");
        instruction.trimmed();
        setStream(instruction);
    }

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


void OmxPlayer::youtubeDLReadyRead(){
    qWarning()<<"data readable";
    if(!_sidePlayerSoftware->canReadLine())
        return;

    setStream(_sidePlayerSoftware->readLine());
    disconnect(_sidePlayerSoftware, SIGNAL(readyRead()), this, SLOT(youtubeDLReadyRead()));
}
