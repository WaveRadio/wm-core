#include <QObject>
#include <QCoreApplication>
#include <QCommandLineParser>
#include <QCommandLineOption>

#include "wmcore.h"

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    QCommandLineParser parser;
    parser.addOption(QCommandLineOption(QStringList() << "c" << "config", "Configuration file", "config"));
    parser.process(a);

    WMCore core(parser.value("config"), &a);

    int ret = a.exec();

    core.onCoreExit();

    return ret;
}
