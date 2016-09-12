#ifndef IODATA_H
#define IODATA_H

#include <QObject>
#include <QString>
#include <QUrl>
#include <QtDebug>


#include <random>
#include <ctime>
#include <unordered_map>
#include "clientworker.h"

class IOData : public QObject
{
    Q_OBJECT
public:
    explicit IOData(QObject *parent = 0);
    Q_INVOKABLE void sendData(const QString &q_filepath, const QString &oformat);

signals:

public slots:
private:




};

#endif // IODATA_H
