#include "converterserver.h"


ConverterServer::ConverterServer() : avail_cpu(1) {
}

int ConverterServer::addAddress(const std::string &ipaddr, int port) {

    int sock;
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == -1 ) {
        perror("init socket");
        return -1;
    }

    int flag = 1;
    if(setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag)) == -1) {
        perror("setsockopt: ");
        return -1;
    }


    sockaddr_in sin;
    sin.sin_family = AF_INET;
    sin.sin_port = htons(port);

    if(inet_pton(AF_INET, ipaddr.c_str(), &sin.sin_addr) != 1) {
        perror("inet_pton error");
        return -1;
    }

    if(bind(sock, (const sockaddr *)&sin, sizeof(sin)) == -1) {
        perror("bind");
        return -1;
    }

    if(listen(sock, SOMAXCONN) == -1) {
        perror("listening");
        return -1;
    }

    listening_list.push_back(sock);

    return 0;

}

int ConverterServer::run() {
    if(initWorkers() != 0) {
        return -1;
    }

    if(listening_list.empty()) {
        if(addAddress("127.0.0.1", 12345)) {
            perror("Could not init default listening socket\n");
            return -1;
        }
    }


    ev::default_loop loop;

    ev::io event(loop);
    for(auto &elem : listening_list) {
        event.set<ConverterServer, &ConverterServer::acceptCallback> (this);
        event.set(elem, ev::READ);
        event.start();
    }

    while(true)
        loop.run(0);


    return -1;

}

inline void ConverterServer::acceptCallback(struct ev::io &watcher, int) {

    int cur_fd = accept(watcher.fd, NULL, NULL);

    char buf[1] = {0};

    int worker_fd = *cur_worker;

    sendFd(worker_fd, buf, sizeof(buf), cur_fd);

    ///need addead creating new workers if worker is down
    ++cur_worker;

    if(cur_worker == workers_list.end())
        cur_worker = workers_list.begin();

    close(cur_fd);

}

void ConverterServer::sendFd(int sock, char *buf, int buflen, int sd) {
    struct msghdr msg;
    struct iovec iov;
    struct cmsghdr *cmsg;

    union {
        struct cmsghdr cmsghdr;
        char control[CMSG_SPACE(sizeof(int))];
    } cmsgu;

    iov.iov_base = buf;
    iov.iov_len = buflen;

    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;
    msg.msg_name = NULL;
    msg.msg_namelen = 0;

    if(sd) {

        msg.msg_control = cmsgu.control;
        msg.msg_controllen = sizeof(cmsgu.control);
        cmsg = CMSG_FIRSTHDR(&msg);
        cmsg->cmsg_len = CMSG_LEN(sizeof(int));
        cmsg->cmsg_level = SOL_SOCKET;
        cmsg->cmsg_type = SCM_RIGHTS;

        (*(int*) CMSG_DATA(cmsg)) = sd;

    }

    ssize_t size = sendmsg(sock, &msg, 0);

    if(size < 0)
        fprintf(stderr, "ConverterServer fd %i not passing\n", sd);

}

int ConverterServer::initWorkers() {
    avail_cpu = sysconf(_SC_NPROCESSORS_CONF);
    if(avail_cpu == -1) {
        avail_cpu = 1;
    }

    for(int i = 0; i < avail_cpu; ++i) {
        int sv[2];
        if(( socketpair(AF_UNIX, SOCK_STREAM, 0, sv)) == -1) {
            perror("workers create socket");
            return -1;
        }

        if(fork() == 0) {
            close(sv[sockpair_write]);
            ConverterWorker cur_worker(sv[sockpair_read]);
            cur_worker.run();
            exit(EXIT_FAILURE);
        }

        close(sv[sockpair_read]);

        workers_list.push_back(sv[sockpair_write]);
    }
    cur_worker = workers_list.begin();

    return 0;
}
