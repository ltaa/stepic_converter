#ifndef CONVERTERWORKER_H
#define CONVERTERWORKER_H
#include <sys/types.h>
#include <ev++.h>
#include "rwhandler.h"
#include "clienthandler.h"
#include <unordered_map>

#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>
#include "converter.h"
#include <cstdio>

class ConverterWorker {
    int listen_sd;
    struct ev_loop *_loop;

    std::unordered_map<int, ClientHandler> client_hash;
public:
    ConverterWorker(int sd);
    void run();

private:
    int recvFd(int sock, char *buf, size_t buflen, int *fd);
    void readDataCallback(ev::io &watcher, int);
    void writeDataCallback(ev::io &watcher, int);
    void addClientCallback(ev::io &watcher, int);
    void timerCallback(ev::timer &watcher, int);
    void closeConnection(int fd);
};

#endif // CONVERTERWORKER_H
