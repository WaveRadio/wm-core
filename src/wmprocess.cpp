#include "wmprocess.h"

WMProcess::WMProcess(QString appPath, QString runtimeDir,
                     QString processTag, ProcessType processType,
                     QStringList args, QObject *parent) :

                     QObject(parent), appPath(appPath), runtimeDir(runtimeDir),
                     processTag(processTag), processType(processType), args(args)
{
    pidFilePath = QString("%1/pid/%2_%3.pid").arg(runtimeDir).arg(typeToString(processType)).arg(processTag);

    isRunning = false;
    iNeedToRespawn = false;

    processId = readPid();

    if (processId == -1)
    {
        log (QString("Could not read pidfile %1, further work is meaningless!"), WMLogger::Error);
        emit processDead(-1, false);
        return;
    }

    if (processId != 0 && isProcessRunning(processId))
    {
        log (QString("Process of %1/%2 is already running and has PID %3; just attaching to it")
             .arg(typeToString(processType)).arg(processTag).arg(processId), WMLogger::Info);

        isAttached = true;
    }
        else
    {
        log (QString("No process for %1/%2 is running, a new instance will be created on start()")
             .arg(typeToString(processType)).arg(processTag));
        isAttached = false;

        process = new QProcess(this);
        process->setProgram(appPath);
        process->setArguments(args);

        connect(process, SIGNAL(started()), this, SLOT(onProcessStart()));
        connect(process, SIGNAL(finished(int)), this, SLOT(onProcessFinish(int)));
    }

    log (QString("Created a new instance of a process handler, process image %1").arg(appPath), WMLogger::Info);
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

bool WMProcess::attached()
{
    return isAttached;
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

bool WMProcess::isProcessRunning(int pid)
{
#ifdef __linux__
    return 0 == kill(pid, 0);
#elif _WIN32
    HANDLE pss = CreateToolhelp32Snapshot(TH32CS_SNAPALL, 0);

    PROCESSENTRY32 pe = { 0 };
    pe.dwSize = sizeof(pe);

    if (Process32First(pss, &pe))
    {
        do
        {
            if (pe.th32ProcessID == pid)
                return true;
        }
        while(Process32Next(pss, &pe));
    }

    CloseHandle(pss);

    return false;
#else
    log("Unknown OS, process detection feature does not work!", WMLogger::Warning);
    return false;
#endif
}

void WMProcess::winOnProcessExit(PVOID lpParameter, BOOLEAN TimerOrWaitFired)
{
// TODO: handle this
}

int WMProcess::readPid()
{
    QFile file(pidFilePath);

    if (!file.exists())
    {
        log (QString("No pidfile found in %1; that's okay, we'll try to create our own").arg(file.fileName()));
        return 0;
    }

    if (!file.open(QIODevice::ReadOnly))
    {
        log (QString("Could not open the file %1; error %2").arg(file.fileName()).arg(file.errorString()), WMLogger::Warning);
        return -1;
    }

    QString data = file.readAll();
    file.close();

    bool ok = false;
    int pidToReturn = data.toInt(&ok);

    if (!ok)
    {
        log (QString("Pidfile %1 contains wrong data: %2").arg(file.fileName()).arg(data), WMLogger::Warning);
        return 0; // -1?
    }
        else
        return pidToReturn;
}

bool WMProcess::writePid(int pid)
{
    QFile file(pidFilePath);

    if (!file.open(QIODevice::WriteOnly))
    {
        log (QString("Could not open pidfile %1 for write!").arg(pidFilePath), WMLogger::Warning);
        return false;
    }

    QTextStream stream(&file);
    stream << pid;

    file.close();
    return true;

}

void WMProcess::start()
{
    if (isRunning)
    {
        log ("Process is already running!", WMLogger::Warning);
        return;
    }

    if (isAttached)
    {
        log ("Process is already running, WMProcess is attaching to it...");

#ifdef __linux__
        log ("Unfortunately, Linux process callbacks aren't supported for now ._.");
#elif _WIN32
        processHandle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, processId);

        RegisterWaitForSingleObject(&eventHandle, processHandle, winOnProcessExit, NULL, INFINITE, WT_EXECUTEONLYONCE);
#else
        log("WMProcess: unknown target OS, process crash detection won't work!", WMLogger::Warning);
#endif

        onProcessStart();
    }
    else
    {
        log ("Creating a new process...");
        process->start();
    }
}

void WMProcess::log(QString message, WMLogger::LogLevel level)
{
    WMLogger::instance->log(message, level, "sproc");
}

void WMProcess::onProcessStart()
{
    if (!isAttached)
    {
        processId = process->processId();

        if (!writePid(processId))
        {
            log(QString("Could not write pidfile!"), WMLogger::Error);
            return;
        }

        log (QString("Process spawning succeeded, PID is %1").arg(processId));
    }

    isRunning = true;
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
            log (QString("Process exited normally: code 0, PID %1").arg(processId));
            break;

        case 0xf291 : // Thanks Qt for this freaking magic number
            log (QString("Process %1 is killed internally").arg(processId));
            break;

        default :
            log (QString("WARNING: Process %1 finished abnormally, exit code is %2")
                 .arg(processId).arg(exitCode),
                 WMLogger::Warning);
            break;
    }

    emit processDead(exitCode, iNeedToRespawn);
}
