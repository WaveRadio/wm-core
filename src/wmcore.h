#ifndef WMCORE_H
#define WMCORE_H

#include <QCoreApplication>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QSettings>
#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>
#include <QJsonParseError>

#include <QFileInfo>

#include "wmlogger.h"
#include "wmprocess.h"
#include "wmcontrolserver.h"

class WMControlServer;

class WMCore : public QObject
{
    Q_OBJECT
public:
    explicit WMCore(QString configFile, QCoreApplication *app = 0, QObject *parent = 0);

    /// Public API to be used by WMControlServer
    // Broadcasting processes
    bool performProcessAction(QString tag, WMProcess::ProcessType type, WMControlServer::ProcessControlAction action);
    QString getCurrentSecret();
    QStringList getInstancesList();

private:

    /// Config variables
    // System
    QString logFile;
    QString configFile;
    WMLogger::LogLevel logLevel;

    // Broadcasting processes
    QString liquidsoapAppPath;
    QString icecastAppPath;
    QString runtimeDir;
    QString dataDir;
    QStringList liquidsoapTags;
    QStringList icecastTags;
    bool respawnProcessesOnDeath;
    bool respawnOnlyOnBadDeath;

    // Control server
    uint serverPort;

    /// Objects & Pointers
    QCoreApplication *app;
    WMControlServer *server;
    QList<WMProcess *> processPool;

    /// Methods
    // System
    void log(QString message, WMLogger::LogLevel logLevel = WMLogger::Debug, QString component = "wcore");
    void loadConfig (QString configFile);
    bool loadInstances(WMProcess::ProcessType type);

    // Broadcasting processes    
    void createProcesses(WMProcess::ProcessType type);
    void correctProcesses(WMProcess::ProcessType type);

    WMProcess *getProcessFor(QString tag, WMProcess::ProcessType type);
    bool createProcessFor(QString tag, WMProcess::ProcessType type);
    bool stopProcessFor(QString tag, WMProcess::ProcessType type, bool forced = false);
    void restartProcessFor(QString tag, WMProcess::ProcessType type);
    void killAllProcesses(WMProcess::ProcessType type = WMProcess::Abstract, bool forRestart = false);

signals:

private slots:
    void onProcessStart();
    void onProcessDeath(int exitCode, bool needsToRestart);

public slots:
    void onCoreExit();
};

#endif // WMCORE_H
