#ifndef WMCONTROLSERVER_H
#define WMCONTROLSERVER_H

#include <QObject>

#include <QString>
#include <QStringList>
#include <QWebSocket>
#include <QWebSocketServer>
#include <QFile>
#include <QList>
#include <QMap>

#include "wmlogger.h"
#include "wmcontrolclient.h"
#include "wmprocess.h"

class WMCore;

class WMControlServer : public QObject
{
    Q_OBJECT
public:

    enum ProcessControlAction {
        Start,
        Stop,
        Restart
    };

    explicit WMControlServer(int serverPort, WMCore *core = 0);
    ~WMControlServer();

    void sendClientCommand(WMControlClient *client, QString command);
    void sendErrorMessage(WMControlClient *client, int code, QStringList args = QStringList());

    void stop();

private:

    WMCore *core;
    QWebSocketServer *server;
    int serverPort;

    QMap<int, QString> errorCodes;
    QList<WMControlClient *> clients;

    void log(QString message, WMLogger::LogLevel logLevel = WMLogger::Debug, QString component = "wserv");

signals:
    void processActionRequired(QString, WMProcess::ProcessType, ProcessControlAction);

private slots:
    void onNewClientConnection();

    void onClientCommand(QString message);
    void onClientDisconnect();

public slots:
    void onServerExit();
};

#endif // WMCONTROLSERVER_H
