#ifndef WMSTATIONPROCESS_H
#define WMSTATIONPROCESS_H

#include <QObject>

#include "wmprocess.h"

class WMStationProcess : public WMProcess
{
public:
    WMStationProcess(QString appPath, QString stationTag, QString runtimeDir);


private:
    QString appPath;
    QString stationTag;
    QString runtimeDir;
};

#endif // WMSTATIONPROCESS_H
