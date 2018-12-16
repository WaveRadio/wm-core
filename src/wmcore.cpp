#include "wmcore.h"

WMCore::WMCore(QString configFile, QCoreApplication *app, QObject *parent) :
    QObject(parent), app(app), configFile(configFile)
{
    connect(app, SIGNAL(aboutToQuit()), this, SLOT(onCoreExit()));

    if (!QFile::exists(configFile))
    {
        if (configFile.isEmpty())
            configFile = "<none>";

        printf ("Configuration file %s does not exist. WaveManager can't work without a config file. Exiting.\n",
                configFile.toUtf8().data());
        exit(-1);
    }

    loadConfig(configFile);

    ///
    /// WARNING! WARNING! WARNING!
    /// THIS PART IS EXTREMELY DESTRUCTIVE!
    /// IT PERFORMS A GENOCIDE OF PROCESSES THAT WERE RAN BEFORE
    /// PLEASE DO NOT USE NEOTON WITH OTHER SERVICES THAT USE
    /// THE SAME PROCESS NAME AS SPECIFIED IN `liquidsoap_path` CONFIG VARIABLE!
    /// OTHERWISE IT WILL CAUSE DATA LOSS AND NUCLEAR EXPLOSION!
    ///
    /// YOU WERE WARNED!
    ///

    // (or should we use pidfiles instead?..)

    WMLogger::instance = new WMLogger(logFile, (WMLogger::LogLevel)logLevel);
    log ("This is WaveManager Core Service", WMLogger::Info);
    log (QString("You're using WMCore/%1").arg(WMCORE_VERSION));

    QString processImageName = QFileInfo(liquidsoapAppPath).fileName();

    #ifdef __linux__
        log ("A Linux platform detected, cleaning up old processes if exist...", WMLogger::Warning);
        system(QString("killall -9 %1").arg(processImageName).toUtf8().data());
    #elif _WIN32
        log ("A Windows platform detected, cleaning up old processes if exist...", WMLogger::Warning);
        system(QString("taskkill /IM %1 /F /T").arg(processImageName).toUtf8().data());
    #else
        log ("The platform WaveManager runs in is unknown, process cleaning is your business.", WMLogger::Warning);
    #endif

    server = new WMControlServer(serverPort, this);

    if (loadInstances(WMProcess::Liquidsoap))
        createProcesses(WMProcess::Liquidsoap);
}

bool WMCore::performProcessAction(QString tag, WMProcess::ProcessType type,
                                  WMControlServer::ProcessControlAction action)
{
    QStringList *tags;

    switch (type)
    {
        case WMProcess::Abstract:
            log ("Ambiguous process type to perform an action on, Abstract type isn't supported!", WMLogger::Warning);
            return false;
            break;

        case WMProcess::Liquidsoap:
            tags = &liquidsoapTags;
            break;

        case WMProcess::Icecast:
            tags = &icecastTags;
            break;

        default:
            log ("Bad value in ProcessType! That's a bug!", WMLogger::Warning);
            return false;
    }

    if (tags->indexOf(tag) == -1)
    {
        log ("No instance found for the specified name", WMLogger::Warning);
        return false;
    }


    switch (action)
    {
        case WMControlServer::Restart:
            restartProcessFor(tag, type);
            return true;
            break;

        case WMControlServer::Stop:
            return stopProcessFor(tag, type, true);
            break;

        case WMControlServer::Start:
            return createProcessFor(tag, type);
            break;

        default:
            log ("Bad value in ProcessControlAction! That's a bug!", WMLogger::Warning);
            return false;
    }
}

void WMCore::log(QString message, WMLogger::LogLevel logLevel, QString component)
{
    WMLogger::instance->log(message, logLevel, component);
}

void WMCore::loadConfig(QString configFile)
{
    QSettings settings(configFile, QSettings::IniFormat);

    settings.beginGroup("system");
    logLevel = (WMLogger::LogLevel)settings.value("log_level", WMLogger::Debug).toInt();
    if (logLevel > 4)
        logLevel = WMLogger::Debug;

    logFile = settings.value("log_file", "stdout").toString();

    respawnProcessesOnDeath = settings.value("respawn", false).toBool();
    respawnOnlyOnBadDeath = settings.value("respawn_on_crash", false).toBool();
    settings.endGroup();

    settings.beginGroup("network");
    serverPort = settings.value("server_port", 8903).toInt();
    settings.endGroup();

    settings.beginGroup("paths");
    liquidsoapAppPath = settings.value("liquidsoap_path", "/usr/bin/liquidsoap").toString();
    icecastAppPath = settings.value("server_port", "/usr/bin/icecast2").toString();

    dataDir = settings.value("data_dir", "/etc/wavemanager").toString();
    runtimeDir = settings.value("runtime_dir", "/var/run/wavemanager").toString();
    settings.endGroup();
}

bool WMCore::loadInstances(WMProcess::ProcessType type)
{
    QStringList *tags;
    QString fileName;

    log (QString("Loading instances list for type %1").arg(WMProcess::typeToString(type)));

    switch (type)
    {
        case WMProcess::Abstract:
            log ("Ambiguous process type to load config of, Abstract type isn't supported!", WMLogger::Warning);
            return false;
            break;

        case WMProcess::Liquidsoap:
            fileName = "stations";
            tags = &liquidsoapTags;
            break;

        case WMProcess::Icecast:
            fileName = "icecasts";
            tags = &icecastTags;
            break;

        default:
            log ("Bad value in ProcessType! That's a bug!", WMLogger::Warning);
            return false;
    }

    QFile file(QString("%1/%2.json").arg(dataDir).arg(fileName));

    if (!file.exists())
    {
        log (QString("No data file found in path %1").arg(file.fileName()), WMLogger::Warning);
        return false;
    }

    if (!file.open(QIODevice::ReadOnly))
    {
        log (QString("Could not open the file; error %1").arg(file.errorString()), WMLogger::Warning);
        return false;
    }

    QString data = file.readAll();
    file.close();

    QJsonParseError error;
    QJsonDocument json = QJsonDocument::fromJson(data.toUtf8(), &error);

    if (error.error != QJsonParseError::NoError)
    {
        log (QString("A JSON parsing error occured: #%1; %2; file %3")
             .arg(error.error).arg(error.errorString()).arg(file.fileName()), WMLogger::Warning);
        return false;
    }

    QJsonArray list = json.array();
    if (list.count() == 0)
    {
        log ("Empty JSON array or not an array at all, skipping", WMLogger::Info);
        return true; // ?
    }

    tags->clear();
    for (int i = 0; i < list.count(); i++)
    {
        log (QString("Adding a new instance of type %1 with tag %2")
             .arg(WMProcess::typeToString(type)).arg(list.at(i).toString()));
        tags->append(list.at(i).toString());
    }

    return true;
}

// TODO: implement creating and correcting
void WMCore::createProcesses(WMProcess::ProcessType type)
{
    log (QString("Creating process instances for type %1").arg(WMProcess::typeToString(type)));

    QStringList tags;

    switch (type)
    {
        case WMProcess::Abstract:
            log ("Ambiguous process list to create, Abstract type isn't supported!", WMLogger::Warning);
            return;
            break;

        case WMProcess::Liquidsoap:
            tags = liquidsoapTags;
            break;

        case WMProcess::Icecast:
            tags = icecastTags;
            break;

        default:
            log ("Bad value in ProcessType! That's a bug!", WMLogger::Warning);
            return;
    }

    for (int i = 0; i < tags.count(); i++)
    {
        createProcessFor(tags.at(i), type);
    }
}

// This method is called when the list of processes
// has been reloaded from JSON file.
// It checks the match between existing instances and tag list
// and runs/stops the corresponding processes according to
// tag list.
/// This method is very expensive, please don't call it frequently.
void WMCore::correctProcesses(WMProcess::ProcessType type)
{
    QStringList tags;

    switch (type)
    {
        case WMProcess::Abstract:
            log ("Ambiguous process list to correct, Abstract type isn't supported!", WMLogger::Warning);
            return;
            break;

        case WMProcess::Liquidsoap:
            tags = liquidsoapTags;
            break;

        case WMProcess::Icecast:
            tags = icecastTags;
            break;

        default:
            log ("Bad value in ProcessType! That's a bug!", WMLogger::Warning);
            return;
    }

    for (int i = 0; i < tags.count(); i++)
    {
        if (getProcessFor(tags.at(i), type) == NULL)
           createProcessFor(tags.at(i), type);
    }

    for (int i = 0; i < processPool.count(); i++)
    {
        WMProcess *proc = processPool.at(i);

        if (proc->type() == type)
        {
            if (tags.indexOf(proc->tag()) == -1)
                proc->stop(true);
        }
    }
}


WMProcess *WMCore::getProcessFor(QString tag, WMProcess::ProcessType type)
{
    for (int i = 0; i < processPool.count(); i++)
    {
        WMProcess *proc = processPool.at(i);
        if (proc->tag() == tag && (proc->type() == type || type == WMProcess::Abstract))
            return proc;
    }

    return NULL;
}

bool WMCore::createProcessFor(QString tag, WMProcess::ProcessType type)
{
    if (getProcessFor(tag, type) != NULL)
    {
        log (QString("A process for %1 is already running").arg(tag));
        return false;
    }

    log (QString("Creating a new process instance for %1").arg(tag));

    QString procPath;
    QStringList procArgs;

    switch (type)
    {
        case WMProcess::Abstract:
            log ("Cannot start an abstract process, check your code!", WMLogger::Warning);
            return false;
            break;

        case WMProcess::Liquidsoap:
            procPath = liquidsoapAppPath;
            procArgs << QString("%1/scripts/%2.liq").arg(dataDir).arg(tag);
            break;

        case WMProcess::Icecast:
            procPath = icecastAppPath;
            procArgs << "-c" << QString("%1/conf/%2.xml").arg(dataDir).arg(tag);
            break;

        default:
            log ("Bad value in ProcessType! That's a bug!", WMLogger::Warning);
            return false;
    }

    WMProcess *process = new WMProcess(procPath, runtimeDir, tag, type, procArgs);

    processPool.append(process);

    connect(process, SIGNAL(processDead(int, bool)), this, SLOT(onProcessDeath(int,bool)));
    connect(process, SIGNAL(processStarted()), this, SLOT(onProcessStart()));
    process->start();

    return true;
}

bool WMCore::stopProcessFor(QString tag, WMProcess::ProcessType type, bool forced)
{
    WMProcess *proc = getProcessFor(tag, type);

    if (proc != NULL)
    {
        proc->stop(forced);
        return true;
    }
    else
    {
        log (QString("Trying to stop an already dead process for %1").arg(tag), WMLogger::Warning);
        return false;
    }
}

void WMCore::restartProcessFor(QString tag, WMProcess::ProcessType type)
{
    WMProcess *proc = getProcessFor(tag, type);

    proc->setNeedsRespawn(true);
    proc->stop();
}

void WMCore::killAllProcesses(WMProcess::ProcessType type, bool forRestart)
{
    log (QString("Killing all the running processes of type %1").arg(WMProcess::typeToString(type)),
         WMLogger::Info);

    for (int i = 0; i < processPool.count(); i++)
    {
        WMProcess *proc = processPool.at(i);

        if (proc->type() == type || type == WMProcess::Abstract)
        {
            proc->setNeedsRespawn(forRestart);
            proc->stop(true);
        }
    }
}

void WMCore::onProcessStart()
{
    WMProcess *proc = (WMProcess *)QObject::sender();

    log (QString("A process of type %1 for tag %2 has successfully started with pid %3")
         .arg(proc->type()).arg(proc->tag()).arg(proc->pid()));
}

void WMCore::onProcessDeath(int exitCode, bool needsToRespawn)
{
    WMProcess *proc = (WMProcess *)QObject::sender();

    log (QString("A process of type %1 for tag %2 has just dead with exit code %3")
         .arg(proc->type()).arg(proc->tag()).arg(exitCode));


    processPool.removeOne(proc);

    if (needsToRespawn)
    {
        log ("This process requires to restart itself");
        createProcessFor(proc->tag(), proc->type());
    }
        else
    if (respawnProcessesOnDeath && exitCode != 0xf291) // 0xf291 is a Qt's magic number that indicates internally killed processes
    {
        if (respawnOnlyOnBadDeath)
        {
            if (exitCode == 0)
                log ("We need to respawn only really crashed processes.");
            else
                createProcessFor(proc->tag(), proc->type());
        }
        else
            createProcessFor(proc->tag(), proc->type());
    }
    else
        log ("Good night, sweet process.");

    proc->deleteLater();
}

void WMCore::onCoreExit()
{
    log ("Stopping the Core", WMLogger::Info);
    server->stop();

    killAllProcesses(WMProcess::Abstract);
}
