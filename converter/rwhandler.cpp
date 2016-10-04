#include "rwhandler.h"


RWHandlerInterface::RWHandlerInterface() {

}

RWHandlerInterface::~RWHandlerInterface()
{

}

RWHandlerCommonImpl::RWHandlerCommonImpl(int sd)
{
    _sd = sd;
    readed_data = 0;
    data_len = 0;
    func = &RWHandlerCommonImpl::readSize;
    maxReadData = 0;
}

int RWHandlerCommonImpl::read() {

    int ret;

    if(!func)
        func = &RWHandlerCommonImpl::readSize;

    while(func != nullptr) {
        ret = (this->*func)();
        if(ret != RD_OK)
            return ret;
    }

    return ret;
}

int RWHandlerCommonImpl::write() {

    int ret;

    if(!func)
        func = &RWHandlerCommonImpl::writrSize;

    while(func != nullptr) {
        ret = (this->*func)();
        if(ret != WR_OK)
            return ret;
    }

    return ret;


}

RWHandlerCommonImpl::~RWHandlerCommonImpl() {

}

std::string RWHandlerCommonImpl::getData() const {
    return _source_data;
}

void RWHandlerCommonImpl::setData(const std::string &value) {
    _source_data = value;
    data_len = _source_data.length() * sizeof(std::string::value_type);
}

RWHandlerCommonImpl::RWHandlerCommonImpl(RWHandlerCommonImpl &&_src)
{
    _sd = _src._sd;
    data_len = _src.data_len;
    readed_data = _src.readed_data;
    _source_data = std::move(_src._source_data);
    func = _src.func;

    _src._sd = -1;
    _src.data_len = 0;
    _src.readed_data = 0;
    func = nullptr;

}



inline int RWHandlerCommonImpl::readSize() {

    while(true) {
        ssize_t len = recv(_sd, &data_len + readed_data, sizeof(uint64_t) - readed_data, MSG_NOSIGNAL);

        if(len + readed_data == sizeof(uint64_t)) {
            if(data_len > 0 && data_len > maxReadData) {
                func = nullptr;
                return TOO_BIG_DATA;
            }

            readed_data = 0;
            _source_data.resize(data_len);
            func = &RWHandlerCommonImpl::readData;
            return RD_OK;
        } else if(len == -1) {
            if(errno == EAGAIN) {
                errno = 0;
                return RD_AGAIN;
            }

            return RD_ERROR;
        } else if( len == 0) {
            return CONNECTION_CLOSED;
        } else {
            readed_data += len;
        }
    }


}



int RWHandlerCommonImpl::readData() {

    while(true) {
        ssize_t len = recv(_sd, &_source_data[readed_data], data_len - readed_data, MSG_NOSIGNAL);

        if(len + readed_data == data_len) {

            std::fprintf(stderr, "string size %lu\n", _source_data.size());

            readed_data = 0;
            func = nullptr;
            return RD_OK;
        } else if(len == -1) {
            if(errno == EAGAIN) {
                errno = 0;
                return RD_AGAIN;
            }
            return RD_ERROR;
        } else if(len == 0) {

            return CONNECTION_CLOSED;

        } else {
            readed_data += len;
        }

    }

}

int RWHandlerCommonImpl::writrSize() {

    while(true) {
        ssize_t len = send(_sd, &data_len + readed_data, sizeof(uint64_t) - readed_data, MSG_NOSIGNAL);

        if(len + readed_data == sizeof(uint64_t)) {
            readed_data = 0;
            func = &RWHandlerCommonImpl::writeData;
            return WR_OK;
        } else if(len == -1) {
            if(errno == EAGAIN) {
                errno = 0;
                return WR_AGAIN;
            }
            return WR_ERROR;
        } else {
            readed_data += len;
        }
    }

}

int RWHandlerCommonImpl::writeData() {


    while(true) {
        ssize_t len = send(_sd, &_source_data[0] + readed_data, data_len - readed_data, MSG_NOSIGNAL);

        if(len + readed_data == data_len) {
            std::fprintf(stderr,"all data writed ok\n");
            readed_data = 0;
            func = nullptr;
            return WR_OK;
        } else if(len == -1) {
            if(errno == EAGAIN) {
                errno = 0;
                return WR_AGAIN;
            }
            return WR_ERROR;
        } else {
            readed_data += len;
        }
    }

}

size_t RWHandlerCommonImpl::getMaxReadData() const
{
    return maxReadData;
}

void RWHandlerCommonImpl::setMaxReadData(const size_t &value)
{
    maxReadData = value;
}
