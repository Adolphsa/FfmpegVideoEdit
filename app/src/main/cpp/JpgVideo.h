//
// Created by Administrator on 2021/10/21.
//

#ifndef FFMPEGVIDEOEDIT_JPGVIDEO_H
#define FFMPEGVIDEOEDIT_JPGVIDEO_H

#include <stdio.h>
#include <string>
using namespace std;

extern "C" {
#include "common/CLog.h"
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libswscale/swscale.h>
#include <libavutil/timestamp.h>
#include <libavutil/opt.h>
}

class JpgVideo {
public:
    JpgVideo();
    ~JpgVideo();

    /**
     * 将多张JPG照片合并成一段视频
     */
    void doJpgToVideo(string srcPath,string dstPath);

private:
    AVFrame *de_frame;
    AVFrame *en_frame;
    // 用于视频像素转换
    SwsContext *sws_ctx;
    // 用于读取视频
    AVFormatContext *in_fmt;
    // 用于解码
    AVCodecContext *de_ctx;
    // 用于编码
    AVCodecContext *en_ctx;
    // 用于封装jpg
    AVFormatContext *ou_fmt;
    int video_ou_index;

    void releaseSources();
    void doDecode(AVPacket *in_pkt);
    void doEncode(AVFrame *en_frame);
};


#endif //FFMPEGVIDEOEDIT_JPGVIDEO_H
