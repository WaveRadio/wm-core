#ifndef WMCONTROLSERVER_H
#define WMCONTROLSERVER_H

#include <QObject>

#include <QString>
#include <QStringList>
#include <QWebSocket>
#include <QWebSocketServer>
#include <QFile>
#include <QList>

#include "wmlogger.h"
#include "wmcontrolclient.h"

class WMControlServer : public QObject
{
    Q_OBJECT
public:
    explicit WMControlServer(int serverPort, QObject *parent = 0);
    ~WMControlServer();

    void stop();

private:

    QWebSocketServer *server;
    int serverPort;

    QList<WMControlClient *> clients;

    void log(QString message, WMLogger::LogLevel logLevel = WMLogger::Debug, QString component = "wserv");

signals:

private slots:
    void onNewClientConnection();

    void onClientCommand(QString message);
    void onClientDisconnect();

public slots:
    void onServerExit();
};

#endif // WMCONTROLSERVER_H
