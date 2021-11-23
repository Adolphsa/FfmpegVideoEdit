//
// Created by Administrator on 2021/11/10.
//

#ifndef FFMPEGVIDEOEDIT_LCMEDIAINFO_H
#define FFMPEGVIDEOEDIT_LCMEDIAINFO_H

#include <string>

#include "CommonUtils.h"

extern "C" {
#include "common/CLog.h"
#include<libavcodec/avcodec.h>
#include<libavformat/avformat.h>

}

class LCMediaInfo {
public:
    explicit LCMediaInfo();

    virtual ~LCMediaInfo();

    void StopPlayVideo();
    void SetVideoFilePath(std::string &path);

    int InitVideoCodec();
    void UnInitVideoCodec();

    int GetVideoWidth();
    int GetVideoHeight();
    int GetRotateAngle();
    int GetVideoFps();

    float    GetMediaTotalSeconds();

private:
    void resetAllMediaPlayerParameters();

    std::string getFileSuffix(const char *path);

private:
    int m_videoWidth = 0;
    int m_videoHeight = 0;
    int m_RotateAngle = 0;
    int m_videoFPS = 0;

    std::string m_videoPathString = "";

    int m_videoStreamIdx = -1;
    int m_audioStreamIdx = -1;

    AVFormatContext *m_pFormatCtx = NULL;

    AVCodecContext *m_pVideoCodecCtx = NULL;
    AVCodecContext *m_pAudioCodecCtx = NULL;

    AVRational m_vStreamTimeRational;
    AVRational m_aStreamTimeRational;

    int m_sampleRate = 48000;
    int m_sampleSize = 16;
    int m_channel = 2;

};


#endif //FFMPEGVIDEOEDIT_LCMEDIAINFO_H
