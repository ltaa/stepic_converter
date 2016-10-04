#include "converterworker.h"
#include "timer.h"

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

    ClientHandler &client = client_hash[watcher.fd];
    RWHandlerCommonImpl &rwData = client.getRwObject();
    ev::timer &timer_event = client.getTimerEvent();
    timer_event.stop();

    if ((ret = rwData.read()) == RWHandlerCommonImpl::RD_AGAIN) {
        timer_event.again();
        return;
    }
    else if(ret == RWHandlerCommonImpl::RD_OK) {

        ClientData inputData;
        if(!inputData.ParseFromString(rwData.getData())) {
            std::fprintf(stderr, "Could not parse input data\n");
            closeConnection(watcher.fd);
            return;
        }

        ClientData outputData;

        AudioConverterCommonImpl converter;
        if(converter.run(inputData, outputData)) {
            std::fprintf(stderr, "Could not converted input data\n");
            closeConnection(watcher.fd);
            return;
        }

        std::string outputString = outputData.SerializeAsString();

        std::fprintf(stderr, "output string size %lu\n", outputString.size());
        std::fprintf(stderr, "output source_data size %lu\n", outputData.source_data().size());

        rwData.setData(outputString);

        watcher.stop();
        watcher.set<ConverterWorker, &ConverterWorker::writeDataCallback> (this);
        watcher.set(watcher.fd, ev::WRITE);
        watcher.start();
        timer_event.again();

    } else {
        if (ret == RWHandlerCommonImpl::CONNECTION_CLOSED)
            std::fprintf(stderr, "closing %i connection\n", watcher.fd);
        else if(ret == RWHandlerCommonImpl::TOO_BIG_DATA)
            std::fprintf(stderr, "closing, too big data size\n");
        else
            std::fprintf(stderr, "IO error, closing %i connection\n", watcher.fd);

        closeConnection(watcher.fd);
    }
}


void ConverterWorker::writeDataCallback(ev::io &watcher, int) {

    int ret;

    ClientHandler &client = client_hash[watcher.fd];
    RWHandlerCommonImpl &rwData = client.getRwObject();
    ev::timer &timer_event = client.getTimerEvent();
    timer_event.stop();

    if ((ret = rwData.write()) == RWHandlerCommonImpl::WR_AGAIN) {
        timer_event.again();

        return;
    } else {

        if(ret == RWHandlerCommonImpl::WR_OK)
            std::fprintf(stderr, "writing ok, close %i connection\n", watcher.fd);
        else {
            std::fprintf(stderr, "Writing error, close %i connection\n", watcher.fd);
            errno = 0;
        }

        closeConnection(watcher.fd);
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

    ev::io *io_event = new ev::io(_loop);
    io_event->set<ConverterWorker, &ConverterWorker::readDataCallback> (this);
    io_event->set(fd, ev::READ);


    Timer *timer_event = new Timer(_loop);
    timer_event->setFd(fd);
    timer_event->set<ConverterWorker, &ConverterWorker::timerCallback> (this);
    timer_event->set(30., 0.);

    ClientHandler client(io_event, timer_event);
    client_hash[fd] = std::move(client);

    timer_event->start();
    io_event->start();

    std::fprintf(stderr, "adding connection fd %i\n", fd);

}

void ConverterWorker::timerCallback(ev::timer &watcher, int) {

    Timer &cur_watcher = static_cast<Timer&> (watcher);
    std::fprintf(stderr, "Timeout, closing fd %i\n", cur_watcher.getFd());

    client_hash.erase(cur_watcher.getFd());

}

void ConverterWorker::closeConnection(int fd) {
    client_hash.erase(fd);
}
