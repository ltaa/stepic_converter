#ifndef RWHANDLER_H
#define RWHANDLER_H
#include "protobuf/clientData.pb.h"
#include <sys/socket.h>
#include <cstdio>

class RWHandlerInterface
{
public:
    enum {RD_ERROR, RD_AGAIN, RD_OK, WR_ERROR, WR_AGAIN, WR_OK, TOO_BIG_DATA, CONNECTION_CLOSED};
    RWHandlerInterface();
    virtual int read() = 0;
    virtual int write() = 0;
    virtual ~RWHandlerInterface() = 0;

};



class RWHandlerCommonImpl : public RWHandlerInterface {
public:
    RWHandlerCommonImpl() {}
    RWHandlerCommonImpl(int sd);
    virtual int read() override;
    virtual int write() override;
    ~RWHandlerCommonImpl();

    std::string getData() const;
    void setData(const std::string &value);

    RWHandlerCommonImpl(RWHandlerCommonImpl && _src);
    RWHandlerCommonImpl(const RWHandlerCommonImpl &src) = default;
    RWHandlerCommonImpl& operator =(const RWHandlerCommonImpl &src) = default;

    size_t getMaxReadData() const;
    void setMaxReadData(const size_t &value);

private:
    int readSize();
    int readData();
    int writrSize();
    int writeData();

    int _sd;
    uint64_t data_len;
    size_t readed_data;
    std::string _source_data;
    int (RWHandlerCommonImpl::*func) ();
    size_t maxReadData;
};



#endif // RWHANDLER_H
