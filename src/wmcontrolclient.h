#ifndef WMCONTROLCLIENT_H
#define WMCONTROLCLIENT_H

#include <QObject>
#include <QString>
#include <QWebSocket>

#include "wmlogger.h"

class WMControlClient : public QObject
{
    Q_OBJECT
public:

    explicit WMControlClient(QWebSocket *sock, QObject *parent = 0);

    void sendCommand (QString command);
    void setAuthorized (bool auth);
    void setChallengeNonce (QString nonce);

    bool authorized();
    QString challengeNonce();

    void close();

private:


protected:
    QWebSocket *sock;

    bool isAuthorized;
    QString chNonce;
    void log (QString message, WMLogger::LogLevel level = WMLogger::Debug);

signals:

private slots:
    void onSocketMessage (QString message);
    void onSocketDisconnect();

public slots:

signals:
    void newCommandReceived(QString);
    void disconnected();
};

#endif // WMCONTROLCLIENT_H
