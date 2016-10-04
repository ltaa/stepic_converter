TEMPLATE = app
CONFIG += console c++11
CONFIG -= app_bundle
CONFIG -= qt

SOURCES += main.cpp \
    converterserver.cpp \
    converterworker.cpp \
    rwhandler.cpp \
    converter.cpp \
    protobuf/clientData.pb.cc \
    clienthandler.cpp \
    timer.cpp

HEADERS += \
    converterserver.h \
    converterworker.h \
    rwhandler.h \
    converter.h \
    protobuf/clientData.pb.h \
    clienthandler.h \
    timer.h


LIBS += -lev -lavcodec-ffmpeg -lavutil-ffmpeg -lavformat-ffmpeg -lavresample-ffmpeg -lswresample-ffmpeg -lprotobuf

QMAKE_CXXFLAGS += -Wall
