#ifndef CLIENTHANDLER_H
#define CLIENTHANDLER_H

#include "rwhandler.h"
#include <ev++.h>
#include <unistd.h>
#include "timer.h"

class ClientHandler
{
public:
    ClientHandler();
    ClientHandler(ev::io* io_event, Timer *timer_event);

    ClientHandler(ClientHandler &&src);
    ClientHandler& operator= (ClientHandler && src);
    ~ClientHandler();

    ev::io& getIOEvent();
    Timer& getTimerEvent();

    RWHandlerCommonImpl &getRwObject();

private:
    const size_t defaultMaxReader = 500 * 1024 * 1024;
    RWHandlerCommonImpl rwObject;
    ev::io *io_event_;
    Timer *timer_event_;
};

#endif // CLIENTHANDLER_H
