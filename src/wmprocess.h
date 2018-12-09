#ifndef WMPROCESS_H
#define WMPROCESS_H

#include <QObject>

#include <QProcess>
#include <QString>
#include <QStringList>

#include "wmlogger.h"

class WMProcess : public QObject
{
    Q_OBJECT
public:

    enum ProcessType {
        Abstract,
        Liquidsoap,
        Icecast
    };

    explicit WMProcess(QString appPath, QString runtimeDir,
                       QString processTag, ProcessType processType,
                       QStringList args, QObject *parent = 0);

    void start();
    void stop(bool forced = false);

    void setNeedsRespawn(bool need);
    bool isNeedToRespawn();

    int pid();
    QString tag();
    QString typeAsString();
    ProcessType type();

   static QString typeToString(ProcessType type);

private:

    bool isRunning;
    bool iNeedToRespawn;

    QString appPath;
    QString processTag;
    QString runtimeDir;
    QStringList args;
    ProcessType processType;

    QProcess *process;

    void log (QString message, WMLogger::LogLevel level = WMLogger::Debug);

private slots:
    void onProcessStart();
    void onProcessFinish(int exitCode);

signals:
    void processDead(int, bool);
    void processStarted();

public slots:
};

#endif // WMPROCESS_H
