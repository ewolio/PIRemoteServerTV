#include "appinterface.h"

App::App(QString name, bool verbose, QObject* parent): QObject(parent), _name(name)
{
    _appID = 255;
    this->verbose = verbose;
}

bool App::registeredAs(quint8 appID)
{
    if(this->_appID != 255)
        return false;
    qWarning()<<"Register: "<<_name<<" -> "<<appID;
    this->_appID = appID;
    return true;
}

void App::registerAction(AppAction *action)
{
    action->registered(this, actionList.size());
    actionList.append(action);
    connect(action, SIGNAL(returnStatement(quint8,QString)), this, SLOT(returnStatementEmited(quint8,QString)));
}

QString App::toString()
{
    QString r;
    r.setNum(_appID);
    r.append(":"+_name+"|");

    for (int i = 0; i < actionList.size(); ++i)
        r.append(QString().setNum(actionList.at(i)->getCode())+":"+actionList.at(i)->getName()+"|");

    return r;
}

bool App::trigger(int socketID, quint8 actionID, QString args)
{
    if(actionID == 255){
        if(!socketToReply.contains(socketID))
            socketToReply.append(socketID);
        return true;
    }

    qWarning()<<"Trigger: "<<_name;

    if(actionID >= actionList.size())
        return false;

    actionList.at(actionID)->trigger(args);

    if(verbose)
        qWarning()<<_name<<"."<<actionList.at(actionID)->getName()<<"("<<args<<")";

    return true;
}

void App::returnStatementEmited(quint8 actionID, QString statement)
{
    for (int i = 0; i < socketToReply.size(); ++i)
        emit returnStatement(socketToReply.at(i), _appID, actionID, statement);

}


/****************************************************
 *              AppActions
 * **************************************************/

AppAction::AppAction(QString name):QObject(), _name(name)
{
    _code = 255;
    _app = NULL;
}

void AppAction::registered(App *app, quint8 code)
{
    if(_app!=NULL) //déjà enregistrer -> annulation
        return;
    this->_app = app;
    this->_code = code;
}
void AppAction::trigger(QString args){
    qWarning()<<"AppAction::triggers";
    action(args);
}



//---------- Call Back  --------------------
CallBackAction::CallBackAction(QString name, void (*cb)()): AppAction(name)
{
    this->cb = cb;
    this->cbArg = NULL;
    this->cbR = NULL;
    this->cbRArg = NULL;
}
CallBackAction::CallBackAction(QString name, void (*cb)(QString)): AppAction(name)
{
    this->cb = NULL;
    this->cbArg = cb;
    this->cbR = NULL;
    this->cbRArg = NULL;
}
CallBackAction::CallBackAction(QString name, QString (*cb)()): AppAction(name)
{
    this->cb = NULL;
    this->cbArg = NULL;
    this->cbR = cb;
    this->cbRArg = NULL;
}
CallBackAction::CallBackAction(QString name, QString (*cb)(QString)): AppAction(name)
{
    this->cb = NULL;
    this->cbArg = NULL;
    this->cbR = NULL;
    this->cbRArg = cb;
}

void CallBackAction::action(QString args){
    if(cbArg!=NULL)
        return (*cbArg)(args);
    if(cb!=NULL)
        return (*cb)();
    if(cbR!=NULL)
        emit returnStatement(_code, (*cbR)());
    else
        emit returnStatement(_code, (*cbRArg)(args));
}


//-----------  Slot  --------------------

SlotAction::SlotAction(QString name, QObject *obj, const char *member): AppAction(name)
{
    this->obj = obj;
    this->member = member;
}

void SlotAction::action(QString args)
{
    QMetaObject::invokeMethod(obj, member, Q_ARG(QString, args));
}


//------------  Process  -----------------


ProcessAction::ProcessAction(QString name, QString cmd): AppAction(name)
{
    _cmd = cmd;
    dataMapper = new QSignalMapper(this);
    endMapper = new QSignalMapper(this);

    connect(dataMapper, SIGNAL(mapped(int)), this, SLOT(dataReadable(int)));
    connect(endMapper, SIGNAL(mapped(int)), this, SLOT(endSignal(int)));
}


void ProcessAction::dataReadable(int processID)
{
    if(!processes.contains(processID))
        return;

    QProcess* p = processes.value(processID);
    while(p->canReadLine())
        emit returnStatement(_code, p->readLine());
}

void ProcessAction::endSignal(int processID)
{
    if(!processes.contains(processID))
        return;

    delete processes.value(processID);
    processes.remove(processID);
}



void ProcessAction::action(QString args)
{
    QStringList arg = args.split('\\');

    QString execCmd = _cmd;

    while(execCmd.contains('%')){
        int idToReplace = execCmd.indexOf('%');
        int argID = 0;
        execCmd.remove(idToReplace,1);
        while(idToReplace < execCmd.size()){
            if(QChar(execCmd.at(idToReplace)).isNumber()){
                argID = argID*10 + QChar(execCmd.at(idToReplace)).digitValue();
                execCmd.remove(idToReplace,1);
            }else
                break;
        }

        if(arg.size()<=argID)
            execCmd.insert(idToReplace, arg.at(argID));
    }


    QProcess* p = new QProcess(this);
    arg = execCmd.split(" ",QString::SkipEmptyParts);
    arg.removeFirst();
    processes.insert(processes.lastKey()+1, p);
    connect(p, SIGNAL(readyRead()), dataMapper, SLOT(map()));
    dataMapper->setMapping(p, processes.lastKey());
    connect(p, SIGNAL(finished(int)), endMapper, SLOT(map()));
    endMapper->setMapping(p, processes.lastKey());

    p->execute(execCmd.split(" ", QString::SkipEmptyParts).first(), arg);
}


