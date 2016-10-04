#ifndef TIMER_H
#define TIMER_H
#include <ev++.h>

class Timer : public ev::timer
{
public:
    Timer(ev::loop_ref loop = ev::get_default_loop());
    int getFd() const;
    void setFd(int fd);
private:
    int fd_;
};

#endif // TIMER_H
