#include "screen.h"

Screen::Screen(QObject *parent): App("screen", true, parent)
{
    generalVolume = 0;

    add(new SlotAction("volume", this, "setVolume"));
    add(new SlotAction("screen", this, "setScreenOn"));
    add(new SlotAction("addWindow", this,"addWindow"));
    add(new SlotAction("window", this, "sendToWindow"));
}

Screen::~Screen()
{
    foreach(QThread* t, windowsThread){
        t->quit();
        t->wait();
    }
}

void Screen::setVolume(QString volume){
    bool ok;
    int v = volume.toInt(&ok);
    if(ok)
        setVolume(v);
}

void Screen::setVolume(int volume){
    generalVolume = volume;
    emit generalVolumeChanged();
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
    QThread *t = new QThread(this);
    connect(t, SIGNAL(finished()), w, SLOT(deleteLater()));
    connect(this, SIGNAL(generalVolumeChanged()), w, SLOT(updateVolume()));
    w->moveToThread(t);

    w->registerWindow(this, windows.size());


    windows.append(w);
    windowsThread.append(t);
    t->start(QThread::LowPriority);
}

void Screen::sendToWindow(QString message)
{
    int idSep = message.indexOf('|');

    int windowID = message.left(idSep).toInt();

     qWarning()<<windowID<<"->"<<message;

     if(windows.isEmpty())
         addWindow("omxplayer");

    if(windowID>= windows.size())
        return;

    message = message.mid(idSep+1);
    QMetaObject::invokeMethod( windows.at(windowID), "messageReceived", Q_ARG(QString, message));

}

/**************************************************
 *                     Window                     *
 * ***********************************************/

Window::Window(QString windowType): QObject()
{
    _type = windowType;
    _x1=_x2=_y1=_y2=-1;
    _fullscreen = true;
    _volume = 0;
}

void Window::registerWindow(Screen *screen, int windowID)
{
    this->screen = screen;
    this->windowID = windowID;
}

void Window::setVolume(int volume){
    _volume = volume;
    updateVolume();
}

void Window::updateVolume()
{
    onVolumeChanged();
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
}

OmxPlayer::~OmxPlayer()
{
    _omxplayer->kill();
    _sidePlayerSoftware->kill();
}

void OmxPlayer::setStream(QString streamName){

    _stream = streamName;
    _sidePlayerCmd = "";
    qWarning()<<"stream:"<<_stream;
    if(streamName.startsWith("http://www.twitch.tv/")){
        _sidePlayerCmd = "livestream "+streamName+" best --player-external-http-port "+QString().setNum(31414-getWindowID());
        _stream = "http://localhost:"+QString().setNum(31414-getWindowID());
    }else if(streamName.startsWith("https://www.youtube.com/watch")){
        QProcess youtubeDL;
        youtubeDL.start("youtube-dl -g "+streamName);
        youtubeDL.waitForStarted();
        qWarning()<<"Executing youtube-dl...";

        _stream="";

        do{
            qWarning()<<"reading"<<_stream;
            youtubeDL.waitForReadyRead();
            _stream+=youtubeDL.readAll();
            if(_stream.contains("\n")){
                youtubeDL.terminate();
                _stream.truncate(_stream.indexOf('\n'));
            }
        }while(youtubeDL.state()==QProcess::Running);
    }

    if(isPlaying()){
        stop();
        play();
    }
}


void OmxPlayer::play(int startTime){
    if(isPlaying())
        return;

    if(_sidePlayerCmd!=""){
        _sidePlayerSoftware->start(_sidePlayerCmd);
        _sidePlayerSoftware->waitForBytesWritten(2000);
    }

    _startingTime = startTime;
    startPlayer();
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
    _omxplayer->write("q");
    _omxplayer->waitForFinished(2000);
    _omxplayer->kill();

    _sidePlayerSoftware->kill();
}

void OmxPlayer::startPlayer()
{
    QString cmd = "omxplayer "+_stream+" --live --vol "+QString().setNum(getRealVolume()*300)+" ";
    previousVolume = getRealVolume();

    if(!isFullscreen())
        cmd += "--win \""+QString().setNum(x1())+" "+QString().setNum(x2())+" "+QString().setNum(y1())+" "+QString().setNum(y2())+"\" ";

    if(_startingTime!=0){
        int h=0, m=0, s=0;
        h = _startingTime/3600;
        m = _startingTime/60%60;
        s = _startingTime%60;
        cmd += "--pos "+
                (h>9?QString().setNum(h):"0")+QString().setNum(h)+":"+
                (m>9?QString().setNum(m):"0")+QString().setNum(m)+":"+
                (s>9?QString().setNum(s):"0")+QString().setNum(s);
    }

    _omxplayer->start(cmd);
    _omxplayer->waitForStarted();
    qWarning()<<"start reading: "<<cmd;
}

void OmxPlayer::reloadPlayer(){
    _omxplayer->write("q");
    _omxplayer->waitForFinished();
    startPlayer();
}

void OmxPlayer::playerStopped()
{
    if(_omxplayer->state()==QProcess::Running)
        return;

    stop();
}

void OmxPlayer::onVolumeChanged(){
    if(!isPlaying())
        return;
    qWarning()<<"Volume changed: "<<previousVolume<<"->"<<getRealVolume();
    while(previousVolume != getRealVolume()){
        if(previousVolume<getRealVolume()){
            previousVolume++;
            _omxplayer->write("+");
        }else{
            previousVolume--;
            _omxplayer->write("-");
        }

        while(_omxplayer->canReadLine())
            _omxplayer->waitForReadyRead();
        qWarning()<<"omxPlayer: "<<_omxplayer->readAll();

        if(previousVolume!=getRealVolume())
            QThread::msleep(100);
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
