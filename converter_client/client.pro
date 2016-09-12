QT += qml quick quickwidgets

SOURCES += \
    main.cpp \
    iodata.cpp \
    protobuf/clientData.pb.cc \
    clientworker.cpp

RESOURCES += \
    resources.qrc

HEADERS += \
    iodata.h \
    protobuf/clientData.pb.h \
    clientworker.h


LIBS += -lprotobuf

QMAKE_CXXFLAGS += -std=c++11
