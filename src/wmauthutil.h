#ifndef WMAUTHUTIL_H
#define WMAUTHUTIL_H

// This file has been borrowed from Neoton project
// with only slight modifications

#include <QObject>
#include <QString>
#include <QByteArray>
#include <QCryptographicHash>
#include <cstdlib>
#include <ctime>

class WMAuthUtil : public QObject
{
    Q_OBJECT
public:
    explicit WMAuthUtil(QObject *parent = 0);

    static int rangedRand (int min, int max);
    static QString sha256 (QString text);
    static QString randomString (int length = 64);

signals:

public slots:
};

#endif // WMAUTHUTIL_H
