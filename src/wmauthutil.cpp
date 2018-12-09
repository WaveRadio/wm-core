#include "wmauthutil.h"

WMAuthUtil::WMAuthUtil(QObject *parent) : QObject(parent)
{

}

int WMAuthUtil::rangedRand(int min, int max)
{
    srand (time(NULL));
    return min + (rand() % (max - min + 1));
}

QString WMAuthUtil::sha256(QString text)
{
    return QString(QCryptographicHash::hash(QByteArray(text.toUtf8()), QCryptographicHash::Sha256).toHex());
}

QString WMAuthUtil::randomString(int length)
{
    QString alphabet = "qwertyuiopasdfghjklzxcvbnmQWERTYUIOPASDFGHJKLZXCVBNM1234567890-_=+";
    int alphabetLen  = alphabet.length() - 1;

    QString res;

    for (int i = 0; i < length; i++)
    {
        res += alphabet.at(rangedRand(0, alphabetLen));
    }

    return res;
}
