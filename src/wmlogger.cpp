#include "wmlogger.h"

WMLogger *WMLogger::instance = 0;

void WMLogger::log(QString message, WMLogger::LogLevel logLevel, QString component)
{
    if (logLevel > verbosity)
        return;

    QString logLevelCode;

    switch (logLevel)
    {
        case WMLogger::Debug   : logLevelCode = "DBG"; break;
        case WMLogger::Info    : logLevelCode = "INF"; break;
        case WMLogger::Warning : logLevelCode = "WRN"; break;
        case WMLogger::Error   : logLevelCode = "ERR"; break;
        case WMLogger::None    : return;
    }

    if (writeStdout)
    {
        printf ("[%s] <%s> %s: %s\n",
                QDateTime::currentDateTime().toString("dd.MM.yy@hh:mm:ss:zzz").toUtf8().data(),
                logLevelCode.toUtf8().data(),
                component.toUtf8().data(),
                message.toUtf8().data());
    }
        else
    {
       QTextStream(&logFile) << QString("[%1] <%2> %3: %4\n")
                                .arg(QDateTime::currentDateTime().toString("dd.MM.yy@hh:mm:ss:zzz"))
                                .arg(logLevelCode)
                                .arg(component)
                                .arg(message);

    }

}
