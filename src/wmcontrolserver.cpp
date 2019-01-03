#include "wmcontrolserver.h"
#include "wmcore.h"

WMControlServer::WMControlServer(int serverPort, WMCore *core) : core(core), serverPort(serverPort)
{
    // 9xx - system errors
    errorCodes.insert(999, "Syntax error");

    // 1xx - user & auth codes
    errorCodes.insert(100, "Not authorized");
    errorCodes.insert(101, "Bad auth data");

    // 2xx - user-initiated errors
    errorCodes.insert(200, "No such service");

    // 3xx - eventual errors
    errorCodes.insert(300, "Service %1 has crashed");

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

void WMControlServer::sendClientCommand(WMControlClient *client, QString command)
{
    if (clients.indexOf(client) > -1)
    {
        log (QString("Sending to control client: %1").arg(command));
        client->sendCommand(command);
    }
    else
        log ("Trying to send a command to a non-existent client!", WMLogger::Warning);
}

void WMControlServer::broadcastCommand(QString command)
{
    for (int i = 0; i < clients.count(); i++)
    {
        WMControlClient *client = clients.at(i);
        if (client->authorized())
        {
            client->sendCommand(command);
        }
    }
}

void WMControlServer::sendErrorMessage(WMControlClient *client, int code, QStringList args)
{
    QString comment = errorCodes.value(code);

    if (comment.isEmpty())
    {
        log (QString("Trying to send a wrong error code #%1, that's a bug!").arg(code), WMLogger::Warning);
        return;
    }

    if (args.count() > 0)
    {
        for (int i = 0; i < args.count(); i++)
            comment = comment.arg(args.at(i));
    }

    client->sendCommand(QString("ERROR %1 #%2").arg(code).arg(comment));
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

        client->setChallengeNonce(WMAuthUtil::randomString());

        connect(client, SIGNAL(newCommandReceived(QString)), this, SLOT(onClientCommand(QString)));
        connect(client, SIGNAL(disconnected()), this, SLOT(onClientDisconnect()));

        clients.append(client);

        client->sendCommand(QString("INIT %1 #WMCore/%2").arg(client->challengeNonce()).arg(WMCORE_VERSION));
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

    if (commands.count() == 0)
        return;

    if (commands[0] == "AUTH")
    {
        if (commands.count() < 2)
        {
            sendErrorMessage(client, 999);
            return;
        }

        QString secret = core->getCurrentSecret();

        if (secret.isEmpty())
        {
            log ("WARNING: No Secret received, all auth attempts are declined!", WMLogger::Warning);
            sendErrorMessage(client, 101);
            return;
        }

        if (commands[1] == WMAuthUtil::authHash(secret, client->challengeNonce()))
        {
            client->setAuthorized(true);
            client->sendCommand("AUTH OK #Welcome here :3");
            log ("Client auth OK");
            return;
        }
            else
        {
            log ("Client auth failed", WMLogger::Warning);
            sendErrorMessage(client, 101);
            return;
        }
    }

    if (!client->authorized())
    {
        log ("Client tries to send commands while unauthorized!", WMLogger::Warning);
        sendErrorMessage(client, 100);
        return;
    }

    if (commands[0] == "SERVICE")
    {
        if (commands.count() < 4)
        {
            client->sendCommand("ERROR 199 #Bad command syntax");
            return;
        }

        WMProcess::ProcessType procType;
        ProcessControlAction action;

        if (commands[1] == "ICECAST")
            procType = WMProcess::Icecast;
        else if (commands[1] == "LIQUIDSOAP")
            procType = WMProcess::Liquidsoap;
        else
            {
                sendErrorMessage(client, 999);
                return;
            }

        if (commands[2] == "RESTART")
            action = Restart;
        else if (commands[2] == "STOP")
            action = Stop;
        else if (commands[2] == "START")
            action = Start;
        else
        {
            sendErrorMessage(client, 999);
            return;
        }

        if (!core->performProcessAction(commands[3], procType, action))
            sendErrorMessage(client, 200);

        return;
    }
}


void WMControlServer::onServerExit()
{
    log ("Server is exiting", WMLogger::Info);
    server->close();
}

void WMControlServer::onProcessChangeState(QString tag, WMProcess::ProcessType type, WMControlServer::ProcessControlAction action)
{
    QString stringType;

    switch (type)
    {
        case WMProcess::Liquidsoap:
            stringType = "LIQUIDSOAP";
            break;

        case WMProcess::Icecast:
            stringType = "ICECAST";
            break;

        default:
            log ("Bad ProcessType, won't broadcast this event", WMLogger::Warning);
            return;
    }

    QString stringAction;
    switch (action)
    {
        case Restart:
            stringAction = "RESTART";
            break;

        case Stop:
            stringAction = "STOP";
            break;

        case Start:
            stringAction = "START";
            break;

        case Crash:
            stringAction = "CRASH";
            break;

        default:
            log ("Bad ProcessControlAction, won't broadcast this event", WMLogger::Warning);
            return;
    }

    broadcastCommand(QString("SERVICE %1 %2 %3").arg(stringType).arg(stringAction).arg(tag));
}

void WMControlServer::log(QString message, WMLogger::LogLevel logLevel, QString component)
{
    WMLogger::instance->log(message, logLevel, component);
}
