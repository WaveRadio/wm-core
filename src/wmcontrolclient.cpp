#include "wmcontrolclient.h"

WMControlClient::WMControlClient(QWebSocket *sock, QObject *parent) : QObject(parent), sock(sock)
{
    connect (sock, SIGNAL(textMessageReceived(QString)), this, SLOT(onSocketMessage(QString)));
    connect (sock, SIGNAL(disconnected()), this, SLOT(onSocketDisconnect()));
}

void WMControlClient::sendCommand(QString command)
{
    if (sock->isValid())
        sock->sendTextMessage(command);
}

void WMControlClient::setAuthorized(bool auth)
{
    isAuthorized = auth;
}

void WMControlClient::setChallengeNonce(QString nonce)
{
    chNonce = nonce;
}

bool WMControlClient::authorized()
{
    return isAuthorized;
}

QString WMControlClient::challengeNonce()
{
    return chNonce;
}

void WMControlClient::close()
{
    sock->close();
}

void WMControlClient::log(QString message, WMLogger::LogLevel level)
{
    WMLogger::instance->log(message, level, "wclnt");
}

void WMControlClient::onSocketMessage(QString message)
{
    emit newCommandReceived(message);
}

void WMControlClient::onSocketDisconnect()
{
    log ("Socket disconnected.");
    emit disconnected();
}

