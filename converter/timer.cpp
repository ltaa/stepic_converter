#include "timer.h"

Timer::Timer(ev::loop_ref loop) : ev::timer(loop){}

int Timer::getFd() const {
    return fd_;
}

void Timer::setFd(int fd) {
    fd_ = fd;
}
