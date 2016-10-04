#include "clienthandler.h"

ClientHandler::ClientHandler() : io_event_(nullptr), timer_event_(nullptr) {}

ClientHandler::ClientHandler(ev::io *io_event, Timer *timer_event) :
    io_event_(io_event),
    timer_event_(timer_event) {}

ClientHandler::ClientHandler(ClientHandler &&src) {
    io_event_ = src.io_event_;
    timer_event_ = src.timer_event_;

    src.io_event_ = nullptr;
    src.timer_event_ = nullptr;

    rwObject = RWHandlerCommonImpl(io_event_->fd);
    rwObject.setMaxReadData(defaultMaxReader);
}

ClientHandler &ClientHandler::operator=(ClientHandler &&src) {
    if(this == &src)
        return *this;

    io_event_ = src.io_event_;
    timer_event_ = src.timer_event_;

    src.fd_ = -1;
    src.io_event_ = nullptr;
    src.timer_event_ = nullptr;

    rwObject = RWHandlerCommonImpl(io_event_->fd);
    rwObject.setMaxReadData(defaultMaxReader);
    return *this;
}

ClientHandler::~ClientHandler() {
    if(io_event_) {
        io_event_->stop();
        shutdown(io_event_->fd, SHUT_RDWR);
        close(io_event_->fd);
    }

    if(timer_event_) {
        timer_event_->stop();
    }

    delete io_event_;
    delete timer_event_;
}

ev::io& ClientHandler::getIOEvent()
{
    return *io_event_;
}

Timer &ClientHandler::getTimerEvent()
{
    return *timer_event_;
}

RWHandlerCommonImpl& ClientHandler::getRwObject()
{
    return rwObject;
}

