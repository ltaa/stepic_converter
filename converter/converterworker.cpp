#include "converterworker.h"


ConverterWorker::ConverterWorker(int sd)
{
    listen_sd = sd;
}

void ConverterWorker::run()
{

    _loop = ev_default_loop(0);
    ev::io event(_loop);


    event.set<ConverterWorker, &ConverterWorker::addClientCallback> (this);
    event.set(listen_sd, ev::READ);
    event.start();


    while(true)
        ev_loop(_loop, 0);

}

int ConverterWorker::recvFd(int sock, char *buf, size_t buflen, int *sd) {
    struct iovec iov;
    struct msghdr msg;
    struct cmsghdr *cmsg;

    union {
        struct cmsghdr cmsghdr;
        char control[CMSG_SPACE(sizeof(int))];
    } cmsgu;

    iov.iov_base = &buf;
    iov.iov_len = buflen;

    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;
    msg.msg_name = NULL;
    msg.msg_namelen = 0;

    msg.msg_control = cmsgu.control;
    msg.msg_controllen = CMSG_LEN(sizeof(cmsgu.control));

    ssize_t sz = recvmsg(sock, &msg, 0);

    if(sz < 0) {
        fprintf(stderr, "fd not passing\n");
        return -1;
    }


    cmsg = CMSG_FIRSTHDR(&msg);

    if(cmsg->cmsg_len == CMSG_LEN(sizeof(int))) {
        if(cmsg->cmsg_level != SOL_SOCKET) {
            fprintf(stderr, "cmsg_level != SOL_SOCKET\n");
            return -1;

        }

        if(cmsg->cmsg_type != SCM_RIGHTS) {
            fprintf(stderr, "cmsg_type != SCM_RIGHTS\n");
            return -1;
        }

        *sd = (*(int*) CMSG_DATA(cmsg));

    }

    return sz;


}

void ConverterWorker::readDataCallback(ev::io &watcher, int) {


    int ret;

    RWHandlerCommonImpl &rwData = client_hash[watcher.fd];
    if ((ret = rwData.read()) == RWHandlerCommonImpl::RD_AGAIN) {
        return;
    }
    else if(ret == RWHandlerCommonImpl::RD_OK) {
        RWHandlerCommonImpl &rwObject =  client_hash[watcher.fd];

        ClientData inputData;
        if(!inputData.ParseFromString(rwObject.getData())) {
            std::fprintf(stderr, "Could not parse input data\n");
            closeConnection(watcher);
            return;
        }

        ClientData outputData;

        AudioConverterCommonImpl converter;
        if(converter.run(inputData, outputData)) {
            std::fprintf(stderr, "Could not converted input data\n");
            closeConnection(watcher);
            return;
        }

        std::string outputString = outputData.SerializeAsString();

        std::fprintf(stderr, "output string size %lu\n", outputString.size());
        std::fprintf(stderr, "output source_data size %lu\n", outputData.source_data().size());

        rwObject.setData(outputString);

        watcher.stop();
        watcher.set<ConverterWorker, &ConverterWorker::writeDataCallback> (this);
        watcher.set(watcher.fd, ev::WRITE);
        watcher.start();

    } else {
        if (RWHandlerCommonImpl::CONNECTION_CLOSED)
            std::fprintf(stderr, "closing %i connection\n", watcher.fd);
        else
            std::fprintf(stderr, "IO error, closing %i connection\n", watcher.fd);

        closeConnection(watcher);
    }
}


void ConverterWorker::writeDataCallback(ev::io &watcher, int) {

    int ret;

    RWHandlerCommonImpl &rwData = client_hash[watcher.fd];
    if ((ret = rwData.write()) == RWHandlerCommonImpl::WR_AGAIN) {

        return;
    } else {

        if(ret == RWHandlerCommonImpl::WR_OK)
            std::fprintf(stderr, "writing ok, close %i connection\n", watcher.fd);
        else {
            std::fprintf(stderr, "Writing error, close %i connection\n", watcher.fd);
            errno = 0;
        }

        shutdown(watcher.fd, SHUT_RDWR);
        close(watcher.fd);
        client_hash.erase(watcher.fd);
        watcher.stop();
        delete &watcher;

    }

}

void ConverterWorker::addClientCallback(ev::io &watcher, int) {

    char buf[1] = {0};
    int fd = -1;

    int sz = recvFd(watcher.fd, buf, sizeof(buf),  &fd);

    if(sz < 0 || fd < 0) {
        return;
    }

    int f_flag = fcntl(fd, F_GETFL);
    f_flag = f_flag | O_NONBLOCK;

    if(fcntl(fd, F_SETFL, f_flag) == -1) {
        std::fprintf(stderr, "Invalid set O_NONBLOCK on %i fd\n", fd);
    }


    ev::io *event = new ev::io(_loop);
    event->set<ConverterWorker, &ConverterWorker::readDataCallback> (this);
    event->set(fd, ev::READ);
    event->start();

    RWHandlerCommonImpl rwData(fd);
    client_hash[fd] = std::move(rwData);


    std::fprintf(stderr, "adding connection fd %i\n", fd);

}

void ConverterWorker::closeConnection(ev::io &watcher) {
    watcher.stop();
    shutdown(watcher.fd, SHUT_RDWR);
    close(watcher.fd);
    client_hash.erase(watcher.fd);
    delete &watcher;
}
