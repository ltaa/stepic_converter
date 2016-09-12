#ifndef CLIENTWORKER_H
#define CLIENTWORKER_H

#include <QObject>
#include <QUrl>
#include <QTcpSocket>
#include <QString>
#include <string>
#include <QFile>
#include <QHostAddress>
#include <QDataStream>
#include <QDebug>
#include "protobuf/clientData.pb.h"

class ClientWorker : public QObject
{
    Q_OBJECT
public:
    explicit ClientWorker(QObject *parent = 0);
    void sendData(const QString &q_filepath, const QString &oformat);
    int setHostAddress(QString address, int _port);
signals:

public slots:
    void onReadyRead();

private slots:

private:
    QTcpSocket *sock;
    QByteArray databuf;
    int64_t len;
    int readed_data;
    QHostAddress host;
    int port;
};


#endif // CLIENTWORKER_H
