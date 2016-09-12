#ifndef CONVERTERSERVER_H
#define CONVERTERSERVER_H

#include <list>
#include <ev++.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string>
#include <algorithm>
#include "converterworker.h"
#include <cstdio>



class ConverterServer {
    enum {sockpair_read = 0, sockpair_write};
    int cnt = 0;
    long avail_cpu;
    std::list<int>::iterator cur_worker;
public:
    ConverterServer();
    int addAddress(const std::string &ipaddr, int port);
    int run();

protected:
    void acceptCallback(ev::io &watcher, int);
    void sendFd(int sock, char *buf, int buflen, int fd);
    int initWorkers();

private:
    std::list<int> listening_list;
    std::list<int> workers_list;
};

#endif // CONVERTERSERVER_H
