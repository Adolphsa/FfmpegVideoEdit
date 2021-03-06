//
// Created by Administrator on 2021/10/26.
//

#ifndef FFMPEGVIDEOEDIT_MERGE_H
#define FFMPEGVIDEOEDIT_MERGE_H

#include <stdio.h>
#include <string>
#include <vector>
#include <limits.h>
#include <sys/time.h>
#include <fstream>

using namespace std;

extern "C"
{
#include "common/CLog.h"
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>
#include <libavutil/timestamp.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
#include <libavfilter/avfilter.h>
#include <libavfilter/buffersrc.h>
#include <libavfilter/buffersink.h>
}

class Merge {
public:
    Merge();

    ~Merge();

    /** 媒体格式类型：
     *  媒体格式分为流式(FLV,MPEG)和非流式(MP4)。主要区别在于两种文件格式如何嵌入元信息，非流式的元信息通常存储在文件中开头，有时在结尾；而流式的元信息跟
     *  具体音视频数据同步存放的。所以多个流式文件简单串联在一起形成新的文件也能正常播放。而多个非流式文件的合并则可能要重新编解码才可以
    */

    /** 非流式媒体容器(MP4)的合并
     *  实现合并任意多个相同容器类型容器文件的功能；合并后的文件分辨率取最低的文件分辨率，像素格式及颜色范围取第一文件的。编码方式则都取第一个文件的
     *  目的：文件合并后文件能够按照合并的顺序正常播放
     */
    void mergeFiles(vector<string> srcVector, string dPath1);

    int getProcessStatus();

    /**
     * 设置视频编码参数
     * @param width
     * @param height
     * @param videoBitrate
     * @param fps
     */
    void setEncodeParam(int width, int height, int videoBitrate, int fps);

    /** 实现给任意一个无声的视频文件添加背景音乐的功能；为了简化处理，假设要加入的音频编码方式被视频容器格式支持。要求：
     *  1、声音可以在视频时间轴上的任意位置开始
     *  2、以视频时间轴为参考，若音频时间最终超过则截断
     *  目的：合并后的文件能正确播放视频以及音频
     */
    void addMusic(const string& videoPath, const string& audioPath, const string& dstPath, const string& startTime);

    /**
     * 删除视频原声
     * @param videoSrcPath
     * @param videoDstPath
     */
    void deleteAudio(string videoSrcPath, string videoDstPath);

private:

    int processStatus;
    vector<string> imgTmpPaths;

    //encoder param
    int encoderWidth;
    int encoderHeight;
    int encoderBitrate;
    int encoderFps;

//    fstream  preFile;
//    fstream outFile;

    uint8_t *yuvBuffer;
    uint8_t *scaleBuffer;

    string dstpath;
    AVFormatContext *in_fmt1;
    AVFormatContext *in_fmt2;
    AVFormatContext *ou_fmt;
    int video1_in_index, audio1_in_index;
    int video2_in_index, audio2_in_index;
    int video_ou_index, audio_ou_index;
    int64_t next_video_pts, next_audio_pts;

    // 所有的输入源
    vector<string> srcPaths;
    // 所有的输入文件解封装器
    vector<AVFormatContext *> in_fmts;
    typedef struct DeCtx {
        AVCodecContext *video_de_ctx;
        AVCodecContext *audio_de_ctx;
    } DeCtx;

    // 所有输入文件对应的音视频解码器;如果某个文件没有视频则对应的为NULL
    vector<DeCtx> de_ctxs;
    AVCodecContext *video_en_ctx;
    AVCodecContext *audio_en_ctx;
    // 用于视频转换
    SwsContext *swsctx;
    // 用于音频转换
    SwrContext *swrctx;

#define MediaIndexCmd(index, fi, vi, ai, wi, hi, sr, pf, sf, cl, vc, ac, vb, ab, fp, rt) \
index.file_index = fi;  \
index.video_index = vi; \
index.audio_index = ai; \
index.width = wi;       \
index.height = hi;      \
index.sample_rate = sr; \
index.pix_fmt = pf;  \
index.smp_fmt = sf; \
index.ch_layout = cl;   \
index.video_codecId = vc;   \
index.audio_codecId = ac;   \
index.video_bit_rate = vb;   \
index.audio_bit_rate = ab;   \
index.fps = fp;                                                                      \
index.rotate = rt;

    // 所有的输入源音视频索引
    typedef struct MediaIndex {
        int file_index;
        int video_index;
        int audio_index;
        int width, height;
        int sample_rate;
        enum AVPixelFormat pix_fmt;
        enum AVSampleFormat smp_fmt;
        int64_t ch_layout;
        AVCodecID video_codecId;
        AVCodecID audio_codecId;
        int64_t video_bit_rate, audio_bit_rate;
        int fps;
        int rotate;
    } MediaIndex;

    vector<MediaIndex> in_indexes;
    AVFrame *video_de_frame;
    AVFrame *audio_de_frame;
    AVFrame *video_en_frame;
    AVFrame *yuv_frame;
    AVFrame *audio_en_frame;
    MediaIndex curIndex;
    MediaIndex preIndex;
    // 最终编码参考的源文件
    MediaIndex wantIndex;
    uint8_t **audio_buffer;
    int left_size;
    bool audio_init;

    // 用于添加背景音乐
    int64_t start_pos;

    bool openInputFile();

    bool addStream();

    void doDecode(AVPacket *pkt, bool isVideo);

    void updateAVFrame();

    void doConvert(AVFrame **dst, AVFrame *src, bool isVideo);

    void doEncode(AVFrame *frame, bool isVideo);

    void resetAllMergeParameters();

    void releaseSources();
};


#endif //FFMPEGVIDEOEDIT_MERGE_H
