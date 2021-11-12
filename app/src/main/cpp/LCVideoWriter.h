//
// Created by Administrator on 2021/11/8.
//

#ifndef FFMPEGVIDEOEDIT_LCVIDEOWRITER_H
#define FFMPEGVIDEOEDIT_LCVIDEOWRITER_H

extern "C"
{
#include "common/CLog.h"
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>
#include <libswresample/swresample.h>
#include <libavutil/opt.h>
}

#include <pthread.h>
#include <iostream>
#include "faac.h"

using namespace std;

typedef struct MP4_AAC_CONFIGURE {
    faacEncHandle hEncoder;            //音频文件描述符
    unsigned int nSampleRate;          //音频采样数
    unsigned int nChannels;          //音频声道数
    unsigned int nPCMBitSize;         //音频采样精度
    unsigned int nInputSamples;       //每次调用编码时所应接收的原始数据长度
    unsigned int nMaxOutputBytes;     //每次调用编码时生成的AAC数据的最大长度
    unsigned char *pcmBuffer;         //pcm数据
    unsigned char *aacBuffer;         //aac数据

} AACEncodeConfig;

class LCVideoWriter {

public:
    static LCVideoWriter* GetInstance();

private:
    static LCVideoWriter*   m_pInstance;
    static pthread_mutex_t  m_mutex;

    //视频输入参数
    int m_videoInWidth      = 640;
    int m_videoInHeight     = 360;

    int  m_videoInPixFmt    = AV_PIX_FMT_ARGB;

    //音频输入参数
    int m_audioInSamplerate = 22050;
    int m_audioInChannels   = 1;


    //视频输出参数
    int m_videoOutBitrate   = 512000;
    int m_videoOutWidth     = 640;
    int m_videoOutHeight    = 360;
    int m_videoOutFps       = 15;

    //音频输出参数
    int m_audioOutBitrate       = 64000;

    int m_audioOutSamplerate    = 22050;
    int m_audioOutChannels      = 1;


    int m_samples =960;//输入输出的每帧数据每通道的样本数

    AVFormatContext *m_pFormatCtx = NULL;//封装mp4输出上下文

    AVCodecContext *m_pVideoCodecCtx = NULL;//视频编码器上下文
    AVCodecContext *m_pAudioCodecCtx = NULL;//音频编码器上下文

    AVStream* m_pVideoStream = NULL;//视频流
    AVStream* audioStream = NULL;//音频流


    AVFrame* m_pYUVFrame = NULL;//输出yuv


    int m_audioFramePts = 0;//音频的pts;
    int m_videoFramePts=0;

    std::string m_filePath;

    bool    m_bRecording=false;

    pthread_mutex_t     m_videoWriteMutex;
    pthread_mutex_t     aacWriterMutex;

    unsigned long                                    lastAudioTimeStamp;
    unsigned long                                    currentAudioTimeStamp;

    int                         m_pcmBufferSize=0;
    int                         m_pcmBufferRemainSize=0;
    int                         m_pcmWriteRemainSize=0;
    AACEncodeConfig*            g_aacEncodeConfig;

    unsigned long                m_startTimeStamp;

public:

    bool StartRecordWithFilePath(const char* file);
    void StopRecordReleaseAllResources();

    void SetupInputResolution(int w,int h){ m_videoInWidth=w; m_videoInHeight=h;}
    void SetupOutputResolution(int w,int h){ m_videoOutWidth=w,m_videoOutHeight=h;}
    void SetupVideoOutParams(int fps,int bit ){m_videoOutFps=fps; m_videoOutBitrate=bit;}

    bool WriteVideoFrameWithRgbData(const unsigned char* rgb);
    bool WriteAudioFrameWithPcmData(unsigned char* data,int captureSize);

private:

    LCVideoWriter();
    ~LCVideoWriter();

    bool addVideoStream();
    bool addAudioStream();

    bool writeFrame(AVPacket* pkt);

    bool writeVideoHeader() ;
    bool writeMp4FileTrail() ;

    void releaseAllRecordResources();

    AACEncodeConfig* initAudioEncodeConfiguration();
    void releaseAccConfiguration();

    int linearPCM2AAC(unsigned char * pData,int captureSize);

    unsigned long GetTickCount();

    class Garbage{
    public:
        ~Garbage(){
            if(LCVideoWriter::m_pInstance != nullptr){
                std::cout<<"Recycle CCVideoWriter::m_pInstance"<<endl;
                delete LCVideoWriter::m_pInstance;
                LCVideoWriter::m_pInstance= NULL;
            }
        }
    };

    static Garbage  m_garbage;
};


#endif //FFMPEGVIDEOEDIT_LCVIDEOWRITER_H
