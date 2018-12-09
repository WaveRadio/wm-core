#include "wmcontrolserver.h"

WMControlServer::WMControlServer(int serverPort, QObject *parent) : QObject(parent), serverPort(serverPort)
{

    server = new QWebSocketServer(QString("WMCore/%1").arg(WMCORE_VERSION),
                                  QWebSocketServer::NonSecureMode,
                                  this);

    log ("Starting the server");
    if (!server->listen(QHostAddress::Any, serverPort))
    {
        log (QString("Could not start the server! Check if the port %1 isn't taken by another app or another WaveManager Core instance.")
                     .arg(serverPort), WMLogger::Error);
        exit(1);
    }

    connect (server, SIGNAL(newConnection()), this, SLOT(onNewClientConnection()));
    log (QString("Server is listening on port %1").arg(serverPort), WMLogger::Info);

}

WMControlServer::~WMControlServer()
{

}

void WMControlServer::stop()
{
    log ("Stopping the control server", WMLogger::Info);
    server->close();
}

void WMControlServer::onNewClientConnection()
{
    while (server->hasPendingConnections())
    {
        log ("A new client connected", WMLogger::Info);

        QWebSocket *sock = server->nextPendingConnection();

        WMControlClient *client = new WMControlClient(sock);

        connect(client, SIGNAL(newCommandReceived(QString)), this, SLOT(onClientCommand(QString)));
        connect(client, SIGNAL(disconnected()), this, SLOT(onClientDisconnect()));

        clients.append(client);

        client->sendCommand(QString("INIT"));
    }
}

void WMControlServer::onClientDisconnect()
{
    WMControlClient *client = (WMControlClient *)QObject::sender();;

    log (QString("Control client #%1 disconnected"), WMLogger::Info);
    clients.removeAt(clients.indexOf(client));

    client->deleteLater();
}

void WMControlServer::onClientCommand(QString message)
{
    WMControlClient *client = (WMControlClient *)QObject::sender();

    log ("Control command: "+message);

    QStringList commands = message.split(" ", QString::SkipEmptyParts);

    if (commands[0] == "AUTH")
    {
        // todo
    }
}


void WMControlServer::onServerExit()
{
    log ("Server is exiting", WMLogger::Info);
    server->close();
}

void WMControlServer::log(QString message, WMLogger::LogLevel logLevel, QString component)
{
    WMLogger::instance->log(message, logLevel, component);
}
