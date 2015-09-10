#ifndef APPINTERFACE_H
#define APPINTERFACE_H

#include <QtCore>

class AppAction;
class App;


class App : public QObject
{
    Q_OBJECT
public:
    explicit App(QString _name, bool verbose=false, QObject *parent = 0);
    bool registeredAs(quint8 _appID);


    void registerAction(AppAction* action);
    App* add(AppAction* action){registerAction(action); return this;}

    QString getName() const {return _name;}
    quint8 getAppID() const {return _appID;}
    QList<AppAction*> getActionList() const {return actionList;}
    bool isVerbose() const {return verbose;}

    QString toString();

signals:
    void returnStatement(int socketToReply, quint8 appID, quint8 actionID, QString statement);

public slots:
    bool trigger(int socketID, quint8 actionID, QString args="");
    void returnStatementEmited(quint8 actionID, QString statement);

private:

    const QString _name;
    quint8 _appID;
    QList<AppAction*> actionList;

    QList<int> socketToReply;

    bool verbose;
};

class AppAction: public QObject{
    Q_OBJECT
public:
    explicit AppAction(QString _name);

    QString getName() const {return _name;}
    quint8 getCode() const {return _code;}

    void registered(App* _app, quint8 _code);

signals:
    void returnStatement(quint8 actionID, QString r);

public slots:
    void trigger(QString args);

protected:
    const QString _name;
    quint8 _code;
    App* _app;

    virtual void action(QString args) = 0;
};

class CallBackAction: public AppAction{
Q_OBJECT
public:
    explicit CallBackAction(QString _name, void (*cb)());
    explicit CallBackAction(QString _name, void (*cb)(QString));
    explicit CallBackAction(QString _name, QString (*cb)());
    explicit CallBackAction(QString _name, QString (*cb)(QString));

private:

    void (*cb)();
    void (*cbArg)(QString);
    QString (*cbR)();
    QString (*cbRArg)(QString);

    void action(QString args);
};

class SlotAction: public AppAction{
    Q_OBJECT
public:
    explicit SlotAction(QString _name, QObject *obj, const char* member);

private:

    QObject *obj;
    const char* member;

    void action(QString args);
};

class ProcessAction: public AppAction{

    Q_OBJECT
public:
    explicit ProcessAction(QString name, QString cmd);


public slots:
    void dataReadable(int processID);
    void endSignal(int processID);

private:

    QObject *obj;
    const char* member;

    QString _cmd;

    void action(QString args);

    QSignalMapper *dataMapper, *endMapper;
    QMap<int, QProcess*> processes;
};

#endif // APPINTERFACE_H
