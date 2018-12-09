#include "wmprocess.h"

WMProcess::WMProcess(QString appPath, QString runtimeDir,
                     QString processTag, ProcessType processType,
                     QStringList args, QObject *parent) :

                     QObject(parent), appPath(appPath), runtimeDir(runtimeDir),
                     processTag(processTag), processType(processType), args(args)
{
    log (QString("Created a new instance of a process handler, process image %1").arg(appPath), WMLogger::Info);

    isRunning = false;
    iNeedToRespawn = false;

    process = new QProcess(this);
    process->setProgram(appPath);
    process->setArguments(args);

    connect(process, SIGNAL(started()), this, SLOT(onProcessStart()));
    connect(process, SIGNAL(finished(int)), this, SLOT(onProcessFinish(int)));
}

void WMProcess::stop(bool forced)
{
    if (!isRunning)
    {
        log ("Nothing to stop, process is already dead.", WMLogger::Warning);
        return;
    }

    if (forced)
    {
        log ("Killing process");
        log (QString("Forcing to kill process %1").arg(process->processId()), WMLogger::Warning);
        process->kill();
    }
        else
    {
        log (QString("Trying to stop process %1").arg(process->processId()), WMLogger::Info);
        process->terminate();
    }
}

void WMProcess::setNeedsRespawn(bool need)
{
    iNeedToRespawn = need;
}

bool WMProcess::isNeedToRespawn()
{
    return iNeedToRespawn;
}

int WMProcess::pid()
{
    return process->processId();
}

QString WMProcess::tag()
{
    return processTag;
}

QString WMProcess::typeAsString()
{
    return typeToString(processType);
}

WMProcess::ProcessType WMProcess::type()
{
    return processType;
}

QString WMProcess::typeToString(ProcessType type)
{
    switch (type)
    {
        case WMProcess::Abstract:
            return "abstract";

        case WMProcess::Icecast:
            return "icecast";

        case WMProcess::Liquidsoap:
            return "liquidsoap";

        default:
            return "unknown";
    }
}

void WMProcess::start()
{
    if (isRunning)
    {
        log ("Process is already running!", WMLogger::Warning);
        return;
    }

    log ("Starting the process...");
    process->start();
}

void WMProcess::log(QString message, WMLogger::LogLevel level)
{
    WMLogger::instance->log(message, level, "sproc");
}

void WMProcess::onProcessStart()
{
    isRunning = true;
    log (QString("Process spawning succeeded, PID is %1").arg(process->processId()));

    emit processStarted();
}

void WMProcess::onProcessFinish(int exitCode)
{
    if (!isRunning)
    {
        log ("WARNING: onProcessFinish() is emitted on a dead process!", WMLogger::Warning);
        return;
    }

    isRunning = false;
    switch (exitCode)
    {
        case 0 :
            log (QString("Process exited normally: code 0, PID %1").arg(process->processId()));
            break;

        case 0xf291 : // Thanks Qt for this freaking magic number
            log (QString("Process %1 is killed internally").arg(process->processId()));
            break;

        default :
            log (QString("WARNING: Process %1 finished abnormally, exit code is %2")
                 .arg(process->processId()).arg(exitCode),
                 WMLogger::Warning);
            break;
    }

    emit processDead(exitCode, iNeedToRespawn);
}
