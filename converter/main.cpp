#include "converterserver.h"
#include <stdio.h>

int main() {

    ConverterServer server;

    if(server.addAddress("127.0.0.1", 12345)) {
        return -1;
    }

    if(server.run()) {
        return -1;
    }

    return 0;
}
