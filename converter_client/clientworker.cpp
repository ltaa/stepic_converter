#include "clientworker.h"



ClientWorker::ClientWorker(QObject *parent) : QObject(parent), host("127.0.0.1"), port(12345) {

    sock = new QTcpSocket(this);
    connect(sock, SIGNAL(readyRead()), this, SLOT(onReadyRead()));
    connect(sock, SIGNAL(disconnected()), this, SLOT(deleteLater()));
    connect(sock, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(deleteLater()));
    len = 0;
    readed_data = 0;
}


void ClientWorker::sendData(const QString &filepath, const QString &oformat) {

    QFile file(filepath);
    file.open(QIODevice::ReadOnly);

    QByteArray byteArray = file.readAll();

    sock->connectToHost(host, port);



    ClientData clienData;
    clienData.set_output_format(oformat.toLower().toStdString());
    clienData.set_filename(QUrl(filepath).fileName().toStdString());
    clienData.set_source_data(byteArray.toStdString());
    clienData.set_result_code(0);
    std::string serialized_data = clienData.SerializeAsString();


    //need little/big endiang converting
    uint64_t size_data = serialized_data.size();


    sock->write((char *)&size_data, sizeof(size_data));
    sock->write(serialized_data.c_str(), serialized_data.size());

}

int ClientWorker::setHostAddress(QString _address, int _port) {

    int flag = 0;
    if(!host.setAddress(_address)) {
        host.setAddress("127.0.0.1");
        flag = -1;
    }

    if(_port > 0 && _port < 65536)
        port = _port;
    else
        flag = -1;

    return flag;
}


void ClientWorker::onReadyRead() {

    if(len == 0)
        sock->read((char*)&len, sizeof(uint64_t));


    QByteArray byteArray = sock->read(len - readed_data);

    databuf.append(byteArray);
    readed_data += byteArray.size();

    if(databuf.size() != len)
        return;

    disconnect(sock, SIGNAL(readyRead()), this, SLOT(onReadyRead()));

    ClientData clientData;

    if(!clientData.ParseFromString(databuf.toStdString())) {
        std::fprintf(stderr,"invalid data parsing\n");
        sock->disconnectFromHost();
        return;
    }

    const std::string &filename = clientData.filename();
    const std::string &source_data = clientData.source_data();

    QFile file(filename.c_str());
    file.open(QIODevice::WriteOnly);

    QDataStream out(&file);

    out.writeRawData(source_data.c_str(), source_data.size());
    file.close();

    len = 0;
    databuf.clear();
    readed_data = 0;

    sock->disconnectFromHost();

}
