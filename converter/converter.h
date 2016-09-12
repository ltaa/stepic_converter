#ifndef CONVERTER_H
#define CONVERTER_H
#include "protobuf/clientData.pb.h"

extern "C" {
#include <stdio.h>

#include "libavformat/avformat.h"
#include "libavformat/avio.h"

#include "libavcodec/avcodec.h"

#include "libavutil/audio_fifo.h"
#include "libavutil/avassert.h"
#include "libavutil/avstring.h"
#include "libavutil/frame.h"
#include "libavutil/opt.h"

#include "libswresample/swresample.h"

}
#include <unordered_map>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <cstdio>
#include <algorithm>
#include <string>



class Converter
{
public:
    Converter();
    virtual int run(ClientData &inputData, ClientData &outputData) = 0;
    virtual ~Converter() = 0;
};


class AudioConverterCommonImpl : public Converter {
public:
    AudioConverterCommonImpl();
    virtual int run(ClientData &inputData, ClientData &outputData) override;
    virtual ~AudioConverterCommonImpl() override;
private:
    AVFormatContext *inputFormatContext;
    AVFormatContext *outputFormatContext;
    AVCodecContext *inputCodecContext;
    AVCodecContext *outputCodecContext;
    AVAudioFifo *fifo;
    SwrContext *resampler;
    int outputFd;
    int64_t pts;
    const static int OUTPUT_BIT_RATE;
    std::unordered_map<std::string, AVCodecID> code_hasnmap;

    void clean();

    int initInputFormat(unsigned char *data, size_t data_len);

    int openInputData(std::string &filename);

    int openOutputData(std::string &filename, AVCodecID codecID);

    void initPacket(AVPacket &packet);
    int initFrame(AVFrame *&frame);
    int initResampler();

    int initFifo();
    int writeOutputFileHeader();
    int decodeFrame(AVFrame *frame, int &data_presend, int &finished);

    int initConverterSamples(uint8_t ** &convertedInputSample, int frameSize);

    int convertSamples(const uint8_t **inputData,
                       uint8_t **convertedData,
                       const int frameSize);

    int addSamplesToFifo(uint8_t **convertedInputSamples, const int frameSize);

    int inputProcessing(int &finished);

    int encodeAudioFrame(AVFrame *frame, int &dataPresent);



    int initOutputFrame(AVFrame *&frame, int frameSize);


    int outputProcessing();

    int writeOutputTrailer();

    int getOutputString(std::string &outputString);

    std::string getOutputFilename(ClientData &inputData);





};
#endif // CONVERTER_H
