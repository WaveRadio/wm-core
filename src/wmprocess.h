#ifndef WMPROCESS_H
#define WMPROCESS_H

#include <QObject>

#include <QProcess>
#include <QString>
#include <QStringList>
#include <QFile>
#include <QTextStream>
#include <QTimer>

#ifdef __linux__
#include <sys/types.h>
#include <signal.h>
#elif _WIN32
#include <windows.h>
#include <tlhelp32.h>
#else
#error "WMProcess: unknown target OS, process presence detection won't work!"
#endif

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

    bool attached();

    int pid();
    QString tag();
    QString typeAsString();
    ProcessType type();

    static QString typeToString(ProcessType type);
    static bool isProcessRunning(int pid);

    static const int RC_KILLEDBYCONTROL = 0xf291; // this is Qt's internal return code

private:

    bool isRunning;
    bool iNeedToRespawn;
    bool isAttached;
    bool isStopRequested;

    QString appPath;
    QString processTag;
    QString runtimeDir;
    QString pidFilePath;
    QStringList args;
    ProcessType processType;

    QProcess *process;
    int processId;

// Windows-specific vars to receive callbacks when process we attached to is dead
#ifdef _WIN32
    HANDLE processHandle;
    HANDLE eventHandle;

    static void CALLBACK winOnProcessExit (PVOID lpParameter, BOOLEAN TimerOrWaitFired);
#endif

// Linux-specific process management
#ifdef __linux__
    QTimer *processWatchTimer;
    int processPollInterval;
#endif

    int readPid();
    bool writePid(int pid);

    void log (QString message, WMLogger::LogLevel level = WMLogger::Debug);

private slots:
    void onProcessStart();
    void onProcessFinish(int exitCode);

#ifdef __linux__
    void onProcessTimerCheck();
#endif

signals:
    void processDead(int, bool);
    void processStarted();

public slots:
};

#endif // WMPROCESS_H
