#include "converter.h"


Converter::Converter() {
}

Converter::~Converter() {

}

AudioConverterCommonImpl::AudioConverterCommonImpl() :
    inputFormatContext(nullptr),
    outputFormatContext(nullptr),
    inputCodecContext(nullptr),
    outputCodecContext(nullptr),
    fifo(nullptr),
    resampler(nullptr),
    outputFd(-1),
    pts(0)
{

    av_register_all();

    code_hasnmap.insert(std::make_pair("flac", AV_CODEC_ID_FLAC));
    code_hasnmap.insert(std::make_pair("ogg", AV_CODEC_ID_VORBIS));
    code_hasnmap.insert(std::make_pair("mp3", AV_CODEC_ID_MP3));

}


const int AudioConverterCommonImpl::OUTPUT_BIT_RATE = 96000;

int AudioConverterCommonImpl::run(ClientData &inputData, ClientData &outputData) {
    pts = 0;

    std::string inputFilename = inputData.filename();
    std::string outputFilename = getOutputFilename(inputData);


    std::string *source_data = inputData.mutable_source_data();

    if(initInputFormat((unsigned char *) &(*source_data)[0], source_data->size())) {
        clean();
        return -1;
    }

    AVCodecID codecId =  code_hasnmap[inputData.output_format()];

    if(openInputData(inputFilename)) {
        clean();
        return -1;
    }


    if(openOutputData(outputFilename, codecId)) {
        clean();
        return -1;
    }

    if(initResampler()) {
        clean();
        return -1;
    }

    if(initFifo()) {
        clean();
        return -1;
    }

    if(writeOutputFileHeader()) {
        clean();
        return -1;
    }


    while(true) {

        int finished = 0;
        int outputFrameSize = outputCodecContext->frame_size;

        while(av_audio_fifo_size(fifo) < outputFrameSize) {

            if(inputProcessing(finished)) {
                clean();
                return -1;
            }

            if(finished)
                break;
        }


        while(av_audio_fifo_size(fifo) >= outputFrameSize
              || (finished && (av_audio_fifo_size(fifo) > 0))) {

            if(outputProcessing()) {
                clean();
                return -1;
            }

        }

        if(finished) {
            int data_writen = 0;
            do {
                if(encodeAudioFrame(NULL, data_writen)) {
                    clean();
                    return -1;
                }
            } while (data_writen);
            break;
        }
    }

    if (writeOutputTrailer()) {
        clean();
        return -1;
    }


    std::string outputString;
    if(getOutputString(outputString) == -1) {
        clean();
        return -1;
    }

    clean();


    outputData.set_filename(outputFilename);
    outputData.set_output_format(inputData.output_format());
    outputData.set_source_data(outputString);
    outputData.set_result_code(0);


    return 0;

}

void AudioConverterCommonImpl::clean() {
    if(fifo) {
        av_audio_fifo_free(fifo);
    }
    swr_free(&resampler);

    if(inputFormatContext) {
        av_free((void*) inputFormatContext->pb);
        avformat_close_input(&inputFormatContext);

    }
    if(inputCodecContext) {
        avcodec_close(inputCodecContext);
    }

    if(outputFormatContext) {
        avio_closep(&outputFormatContext->pb);
        avformat_free_context(outputFormatContext);
    }

    if(outputCodecContext) {
        avcodec_close(outputCodecContext);

    }

    if(outputFd != -1) {
        close(outputFd);
        outputFd = -1;
    }

}



int AudioConverterCommonImpl::initInputFormat(unsigned char *data, size_t data_len) {

    AVIOContext *ioContext = NULL;

    if((ioContext = avio_alloc_context((unsigned char *)data, data_len, 0, NULL, NULL, NULL, NULL)) == NULL) {
            fprintf(stderr, "Could not io context allocated\n");
            return -1;
    }

    if(!(inputFormatContext = avformat_alloc_context())) {
        std::fprintf(stderr, "Could not input format context allocate\n");
        av_free((void*) ioContext);
        return -1;
    }

    inputFormatContext->pb = ioContext;

    AVProbeData probeData;
    probeData.buf = data;
    probeData.buf_size = data_len;
    probeData.filename = "";


    if(!(inputFormatContext->iformat = av_probe_input_format(&probeData, 1))) {
        std::fprintf(stderr, "Unknown input format\n");

        av_free((void*) inputFormatContext->pb);

        avformat_free_context(inputFormatContext);
        inputFormatContext = NULL;
        return -1;
    }

    inputFormatContext->flags |= AVFMT_FLAG_CUSTOM_IO;

    return 0;

}

int AudioConverterCommonImpl::openInputData(std::string &filename) {
    AVCodec *codec;
    int error = 0;



    if((error = avformat_open_input(&inputFormatContext, "", NULL, NULL)) < 0) {
        std::fprintf(stderr, "Could not open input file %s\n", filename.c_str());
        return -1;
    }

    if((error = avformat_find_stream_info(inputFormatContext, NULL)) < 0) {
        std::fprintf(stderr, "Could not find stream in %s\n", filename.c_str());
        avformat_close_input(&inputFormatContext);
        return -1;
    }


    if(inputFormatContext->nb_streams != 1) {
        std::fprintf(stderr, "To many streams in input file\n");
        avformat_close_input(&inputFormatContext);
        return -1;
    }

    if(!(codec = avcodec_find_decoder(inputFormatContext->streams[0]->codec->codec_id))) {
        std::fprintf(stderr, "Coult not find input codec\n");
        avformat_close_input(&inputFormatContext);
        return -1;
    }


    if((error = avcodec_open2(inputFormatContext->streams[0]->codec, codec, NULL)) < 0) {
        std::fprintf(stderr, "Could not open input codec\n");
        avformat_close_input(&inputFormatContext);
        return -1;

    }

    inputCodecContext = inputFormatContext->streams[0]->codec;
    return 0;
}


int AudioConverterCommonImpl::openOutputData(std::string &filename, AVCodecID codecID) {
    AVIOContext *io_output = NULL;
    AVCodec *codec = NULL;
    AVStream *stream = NULL;
    int error;

    char buf[] = {"conv_XXXXXX"};
    outputFd = mkstemp(buf);

    if((error = avio_open(&io_output, buf,  AVIO_FLAG_WRITE)) < 0) {
        std::fprintf(stderr, "Could not open output file\n");
        return -1;
    }

    unlink(buf);


    if(!(outputFormatContext = avformat_alloc_context())) {
        std::fprintf(stderr,"Could not alloc output context\n");
        avio_close(io_output);
        return -1;
    }

    outputFormatContext->pb = io_output;

    if(!(outputFormatContext->oformat = av_guess_format(NULL, filename.c_str(), NULL)))  {
        std::fprintf(stderr, "Undefined output format\n");
        avio_closep(&outputFormatContext->pb);
        avformat_free_context(outputFormatContext);
        outputFormatContext = NULL;
        return -1;
    }

    av_strlcpy(outputFormatContext->filename, filename.c_str(), sizeof(outputFormatContext->filename));


    if(!(codec = avcodec_find_encoder(codecID))) {
        std::fprintf(stderr, "Could not find encoder\n");
        avio_closep(&outputFormatContext->pb);
        avformat_free_context(outputFormatContext);
        outputFormatContext = NULL;
        return -1;
    }


    if(!(stream = avformat_new_stream(outputFormatContext, codec))) {
        std::fprintf(stderr, "Could not find encoder\n");
        avio_closep(&outputFormatContext->pb);
        avformat_free_context(outputFormatContext);
        outputFormatContext = NULL;
        return -1;
    }

    outputCodecContext = stream->codec;

    outputCodecContext->channels = inputCodecContext->channels;
    outputCodecContext->channel_layout = av_get_default_channel_layout(inputCodecContext->channels);
    outputCodecContext->sample_rate = inputCodecContext->sample_rate;
    outputCodecContext->sample_fmt = codec->sample_fmts[0];
    outputCodecContext->bit_rate = OUTPUT_BIT_RATE;

    stream->time_base.den = inputCodecContext->sample_rate;
    stream->time_base.num = 1;

    if(outputFormatContext->oformat->flags & AVFMT_GLOBALHEADER)
        outputFormatContext->oformat->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

    if((error = avcodec_open2(outputCodecContext, codec, NULL)) < 0) {
        avio_closep(&outputFormatContext->pb);
        avformat_free_context(outputFormatContext);
        outputFormatContext = NULL;
        return -1;
    }

    return 0;

}

inline void AudioConverterCommonImpl::initPacket(AVPacket &packet) {
    av_init_packet(&packet);
    packet.data = NULL;
    packet.size = 0;
}

inline int AudioConverterCommonImpl::initFrame(AVFrame* &frame) {
    if(!(frame = av_frame_alloc())) {
        std::fprintf(stderr, "Could not allocate frame\n");
        return -1;
    }

    return 0;
}

int AudioConverterCommonImpl::initResampler()
{
    resampler = swr_alloc_set_opts(NULL,
                       av_get_default_channel_layout(outputCodecContext->channels),
                       outputCodecContext->sample_fmt,
                       outputCodecContext->sample_rate,
                       av_get_default_channel_layout(inputCodecContext->channels),
                       inputCodecContext->sample_fmt,
                       inputCodecContext->sample_rate,
                       0, NULL);

    if(!resampler) {
        std::fprintf(stderr, "Could not resampler allocate\n");
        return -1;
    }


    if(swr_init(resampler) < 0) {
        std::fprintf(stderr, "Could not init resampler\n");
        swr_free(&resampler);
        return -1;
    }

    return 0;
}

int AudioConverterCommonImpl::initFifo() {
    if(!(fifo = av_audio_fifo_alloc(outputCodecContext->sample_fmt, outputCodecContext->channels, 1))) {
        std::fprintf(stderr, "Could not fifo allocated\n");
        return -1;
    }
    return 0;
}

int AudioConverterCommonImpl::writeOutputFileHeader() {
    if(avformat_write_header(outputFormatContext, NULL) < 0) {
        std::fprintf(stderr, "Could not write header to output format context\n");
        return -1;
    }

    return 0;

}

int AudioConverterCommonImpl::decodeFrame(AVFrame *frame, int &data_present, int &finished) {
    AVPacket packet;
    initPacket(packet);
    int error = 0;

    if((error = av_read_frame(inputFormatContext, &packet)) < 0) {

        if(error == AVERROR_EOF)
            finished = 1;
        else {
            std::fprintf(stderr, "Could not read frame\n");
            av_free_packet(&packet);
            return -1;
        }
    }

    if ((avcodec_decode_audio4(inputCodecContext, frame, &data_present, &packet)) < 0) {
        std::fprintf(stderr, "Could not decoded frame\n");
        av_free_packet(&packet);
        return -1;
    }


    if(finished && data_present)
        finished = 0;

    av_free_packet(&packet);
    return 0;

}

int AudioConverterCommonImpl::initConverterSamples(uint8_t ** &convertedInputSample, int frameSize) {
    int error = 0;

    if(!(convertedInputSample = (uint8_t **)calloc(outputCodecContext->channels, sizeof(**convertedInputSample)))) {
        std::fprintf(stderr, "Could not allocate input sample pointers\n");
        return -1;
    }

    if((error = av_samples_alloc(convertedInputSample, NULL, outputCodecContext->channels, frameSize, outputCodecContext->sample_fmt, 0)) < 0) {
        std::fprintf(stderr, "Could not sample allocated\n");
        av_freep(&(convertedInputSample)[0]);
        free(convertedInputSample);
        convertedInputSample = NULL;
        return -1;
    }

    return 0;

}

int AudioConverterCommonImpl::convertSamples(const uint8_t **inputData, uint8_t **convertedData, const int frameSize) {
    int error = 0;

    if((error = swr_convert(resampler, convertedData, frameSize, inputData, frameSize )) < 0 ) {
        std::fprintf(stderr, "Could not convert input samples\n");
        return -1;
    }

    return 0;
}

int AudioConverterCommonImpl::addSamplesToFifo(uint8_t **convertedInputSamples, const int frameSize) {
    int error = 0;
    if((error = av_audio_fifo_realloc(fifo, av_audio_fifo_size(fifo) + frameSize)) < 0) {
        std::fprintf(stderr, "Could not fifo realloc\n");
        return -1;
    }

    if((error = av_audio_fifo_write(fifo, (void **)convertedInputSamples, frameSize)) < frameSize) {
        std::fprintf(stderr, "Could not write converted data to fifo\n");
        return -1;
    }

    return 0;
}

int AudioConverterCommonImpl::inputProcessing(int &finished) {

    AVFrame *frame;
    int data_present = 0;

    uint8_t **convertedInputSample = NULL;

    if(initFrame(frame)) {
        av_frame_free(&frame);
        return -1;
    }

    if(decodeFrame(frame, data_present, finished)) {
        av_frame_free(&frame);
        return -1;
    }

    if(finished && !data_present) {
        av_frame_free(&frame);
        return 0;
    }

    if(data_present) {
        if(initConverterSamples(convertedInputSample, frame->nb_samples)) {
            av_frame_free(&frame);
            return -1;
        }

        if(convertSamples((const uint8_t **)frame->extended_data, convertedInputSample, frame->nb_samples)) {
            av_frame_free(&frame);
            av_freep(&convertedInputSample[0]);
            free(convertedInputSample);
            return -1;
        }

        if(addSamplesToFifo(convertedInputSample, frame->nb_samples)) {
            av_frame_free(&frame);
            av_freep(&convertedInputSample[0]);
            free(convertedInputSample);
            return -1;
        }
    }

    av_frame_free(&frame);
    av_freep(&convertedInputSample[0]);
    free(convertedInputSample);

    return 0;

}


int AudioConverterCommonImpl::encodeAudioFrame(AVFrame *frame, int &dataPresent) {
    AVPacket packet;
    int error = 0;

    initPacket(packet);


    if(frame) {
        frame->pts = pts;
        pts += frame->nb_samples;
    }


    if((error = avcodec_encode_audio2(outputCodecContext, &packet, frame, &dataPresent)) < 0) {
        std::fprintf(stderr, "Could not encoded frame\n");
        av_free_packet(&packet);
        return -1;
    }

    if(dataPresent) {
        if((error = av_write_frame(outputFormatContext, &packet)) < 0) {
            std::fprintf(stderr, "Could not write frame in output context\n");
            av_free_packet(&packet);
            return -1;
        }
        av_free_packet(&packet);
    }

    return 0;


}

int AudioConverterCommonImpl::initOutputFrame(AVFrame *&frame, int frameSize) {
    int error = 0;

    if(!(frame = av_frame_alloc())) {
       std::fprintf(stderr, "Could not allocated output frame\n");
       return -1;
   }


   frame->nb_samples = frameSize;
   frame->channel_layout = outputCodecContext->channel_layout;
   frame->format = outputCodecContext->sample_fmt;
   frame->sample_rate = outputCodecContext->sample_rate;


    if((error = av_frame_get_buffer(frame, 0)) < 0) {
        std::fprintf(stderr,"Could not allocate frame samples\n");
        av_frame_free(&frame);
        return -1;
    }

    return 0;
}

int AudioConverterCommonImpl::outputProcessing() {
    AVFrame *frame;
    int data_present = 0;
    const int frameSize = FFMIN(av_audio_fifo_size(fifo), outputCodecContext->frame_size);

    if(initOutputFrame(frame, frameSize)) {
        return -1;
    }

    if(av_audio_fifo_read(fifo, (void **)frame->data, frameSize) < frameSize) {
        std::fprintf(stderr, "Could not read frame from fifo\n");
        av_frame_free(&frame);
        return -1;
    }

    if(encodeAudioFrame(frame, data_present)) {
        av_frame_free(&frame);
        return -1;
    }

    av_frame_free(&frame);

    return 0;

}

int AudioConverterCommonImpl::writeOutputTrailer() {
    int error = 0;

    if((error = av_write_trailer(outputFormatContext)) < 0) {
        std::fprintf(stderr, "Could not write output trailer\n");
        return -1;
    }

    return 0;

}

#include <fcntl.h>
int AudioConverterCommonImpl::getOutputString(std::string &outputString){


    if(lseek(outputFd, 0, SEEK_CUR) == -1) {
        std::fprintf(stderr, "Could not lseek output data\n");
        return -1;
    }

    struct stat st;
    fstat(outputFd, &st);
    int blockSize = st.st_blksize;
    char obuf[blockSize];



    int len = -1;
    while( len != 0) {
        len = read(outputFd, obuf, blockSize);
        if(len > 0)
            outputString.append(obuf, len);
        else if(len < 0) {
            std::fprintf(stderr,"Could not read converted data\n");
            return -1;
        }
    }

    return 0;
}

std::string AudioConverterCommonImpl::getOutputFilename(ClientData &inputData) {
    const std::string &inputFilename = inputData.filename();

    auto last_point = inputFilename.find_last_of('.');

    if(last_point != inputFilename.length())
        last_point += 1;


    std::string outputFilename = inputFilename.substr(0, last_point);
    outputFilename.append(inputData.output_format());
    return outputFilename;
}

AudioConverterCommonImpl::~AudioConverterCommonImpl() {

}
