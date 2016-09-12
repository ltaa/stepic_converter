#include "iodata.h"


IOData::IOData(QObject *parent) : QObject(parent) {
}

void IOData::sendData(const QString &q_filepath, const QString &oformat) {

    ClientWorker *clientWorker = new ClientWorker(this);
    clientWorker->sendData(QUrl(q_filepath).path(), oformat);
}
