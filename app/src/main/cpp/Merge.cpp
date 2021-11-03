//
// Created by Administrator on 2021/10/26.
//

#include "Merge.h"
#include "CommonUtils.h"
#include "libyuv.h"
#include "FilterVideoScale.h"


#include <utility>

Merge::Merge() {
    av_register_all();
    avcodec_register_all();

    encoderWidth = 720;
    encoderHeight = 1280;
    encoderBitrate = 512000;
    encoderFps = 25;

    in_fmt1 = NULL;
    in_fmt2 = NULL;
    ou_fmt = NULL;
    video1_in_index = -1;
    audio1_in_index = -1;
    video2_in_index = -1;
    audio2_in_index = -1;
    video_ou_index = -1;
    audio_ou_index = -1;
    next_video_pts = 0;
    next_audio_pts = 0;

    curIndex = {0, -1, -1};
    preIndex = {0, -1, -1};
    ou_fmt = NULL;
    video_de_frame = NULL;
    audio_de_frame = NULL;
    video_en_frame = NULL;
    yuv_frame = NULL;
    audio_en_frame = NULL;
    swsctx = NULL;
    swrctx = NULL;
    srcPaths = vector<string>();
    in_fmts = vector<AVFormatContext *>();
    de_ctxs = vector<DeCtx>();
    in_indexes = vector<MediaIndex>();
    video_en_ctx = NULL;
    audio_en_ctx = NULL;
    audio_buffer = NULL;
}

Merge::~Merge() {

}

void Merge::setEncodeParam(int width, int height, int videoBitrate, int fps) {
    //外部设置输出文件的视频编码参数
    if (width != 0) {
        encoderWidth = width;
    }

    if (height != 0) {
        encoderHeight = height;
    }

    if (encoderBitrate != 0) {
        encoderBitrate = videoBitrate;
    }

    if (encoderFps != 0) {
        encoderFps = fps;
    }
}

//合并非流式容器
void Merge::mergeFiles(vector<string> srcVector, string dPath1) {

    outFile.open("/storage/emulated/0/Android/data/com.lc.fve/files/Movies/aa_result_test.yuv",
                 ios::out);
    preFile.open("/storage/emulated/0/Android/data/com.lc.fve/files/Movies/aa_pre_test.yuv",
                 ios::out);

    dstpath = std::move(dPath1);

    srcPaths = std::move(srcVector);

    unsigned long startTime = getTickCount();

//    for (int i = 0; i < srcVector.size(); ++i) {
//        FilterVideoScale filterVideoScale;
//        string temp = srcVector.at(i);
//        string str1 = string(temp, 0, temp.size()-4);
//        string str2 = string(temp, temp.size()-4, 4);
//        LOGD("str1 = %s, str2 = %s", str1.c_str(), str2.c_str());
//
//        str1.append("_pre");
//        str1.append(to_string(i));
//        str1.append(str2);
//        LOGD("result str1 = %s", str1.c_str());
//
//        filterVideoScale.doVideoScale(temp, str1);
//        srcPaths.push_back(str1);
//    }

    if (!openInputFile()) {
        return;
    }

    if (!addStream()) {
        return;
    }



    // 开始解码和重新编码
    for (int i = 0; i < in_fmts.size(); i++) {
        AVFormatContext *in_fmt = in_fmts[i];
        curIndex = in_indexes.at(i);

        // 创建AVFrame
        updateAVFrame();

        // 读取数据
        while (true) {
            AVPacket *pkt = av_packet_alloc();
            if (av_read_frame(in_fmt, pkt) < 0) {
                LOGD("没有数据了 %d", i);
                av_packet_unref(pkt);
                break;
            }

            // 解码
            doDecode(pkt, pkt->stream_index == curIndex.video_index);
            // 使用完毕后释放AVPacket
            av_packet_unref(pkt);
        }

        // 刷新解码缓冲区;同时刷新两个
        doDecode(NULL, true);
        doDecode(NULL, false);

        LOGD("finish file %d", i);
        unsigned long endTime = getTickCount();
        LOGD("耗时 = %ld", (endTime - startTime) / 1000);
    }

    // 写入文件尾部
    if (av_write_trailer(ou_fmt) < 0) {
        LOGD("av_write_trailer fail");
    }
    LOGD("结束写入");

    // 是否资源
    releaseSources();
}


bool Merge::openInputFile() {
    MediaIndexCmd(wantIndex, 0, -1, -1, INT_MAX, INT_MAX, 0, AV_PIX_FMT_NONE, AV_SAMPLE_FMT_NONE, 0,
                  AV_CODEC_ID_NONE, AV_CODEC_ID_NONE, 0, 0, 0, 0)
    int ret = 0;
    for (int i = 0; i < srcPaths.size(); i++) {
        string srcpath = srcPaths[i];
        LOGD("srcpath = %s", srcpath.c_str());
        AVFormatContext *in_fmt = NULL;
        if ((ret = avformat_open_input(&in_fmt, srcpath.c_str(), NULL, NULL)) < 0) {
            LOGD("%d avformat_open_input fail %d %s", i, ret, av_err2str(ret));
            releaseSources();
            return false;
        }

        if ((ret = avformat_find_stream_info(in_fmt, NULL)) < 0) {
            LOGD("%d avformat_find_stream_info fail %d %s", i, ret, av_err2str(ret));
            releaseSources();
            return false;
        }

        in_fmts.push_back(in_fmt);

        //遍历出源文件的音视频信息
        MediaIndex index;
        MediaIndexCmd(index, i, -1, -1, INT_MAX, INT_MAX, 0, AV_PIX_FMT_NONE, AV_SAMPLE_FMT_NONE, 0,
                      AV_CODEC_ID_NONE, AV_CODEC_ID_NONE, 0, 0, 0, 0)
        DeCtx dectx = {NULL, NULL};
        for (int j = 0; j < in_fmt->nb_streams; j++) {

            AVStream *stream = in_fmt->streams[j];

            //创建解码器
            AVCodec *codec = avcodec_find_decoder(stream->codecpar->codec_id);
            LOGD("stream->codecpar->codec_id = %d ,name = %s", stream->codecpar->codec_id,
                 avcodec_get_name(stream->codecpar->codec_id));

            AVCodecContext *ctx = avcodec_alloc_context3(codec);
            if (ctx == NULL) {
                LOGD("avcodec_alloc_context3 fail");
                releaseSources();
                return false;
            }
            //设置解码参数
            if ((ret = avcodec_parameters_to_context(ctx, stream->codecpar)) < 0) {
                LOGD("avcodec_parameters_to_context fail %s", av_err2str(ret));
                releaseSources();
                return false;
            }
            //初始化解码器
            if ((ret = avcodec_open2(ctx, codec, NULL) < 0)) {
                LOGD("avcodec_open2 fail %d %s", ret, av_err2str(ret));
                releaseSources();
                return false;
            }

            if (stream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
                // 视频
                index.video_index = j;
                index.width = stream->codecpar->width;
                index.height = stream->codecpar->height;
                index.video_codecId = stream->codecpar->codec_id;
                index.pix_fmt = (enum AVPixelFormat) stream->codecpar->format;
                index.video_bit_rate = stream->codecpar->bit_rate;
                index.fps = stream->r_frame_rate.num;
                index.rotate = getRotateAngle(stream);
                LOGD("angle = %d", index.rotate);

                // 输出文件的视频编码参数
                if (stream->codecpar->width < wantIndex.width) {  // 取出最小值
                    wantIndex.width = stream->codecpar->width;
                    wantIndex.height = stream->codecpar->height;
                    wantIndex.video_codecId = AV_CODEC_ID_H264;
                    wantIndex.pix_fmt = AV_PIX_FMT_YUV420P;
//                    wantIndex.video_bit_rate = stream->codecpar->bit_rate;
                    wantIndex.video_bit_rate = 512000;
                    wantIndex.fps = 25;

                    LOGD("wantIndex.width = %d, wantIndex.height = %d, wantIndex.video_codecId = %d, wantIndex.pix_fmt = %d, wantIndex.video_bit_rate = %ld, wantIndex.fps = %d",
                         wantIndex.width, wantIndex.height, wantIndex.video_codecId,
                         wantIndex.pix_fmt,
                         wantIndex.video_bit_rate, wantIndex.fps);
                }
                dectx.video_de_ctx = ctx;
            }

            if (stream->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
                index.audio_index = j;
                index.sample_rate = stream->codecpar->sample_rate;
                index.ch_layout = stream->codecpar->channel_layout;
                index.audio_codecId = stream->codecpar->codec_id;
                index.smp_fmt = (enum AVSampleFormat) stream->codecpar->format;
                index.audio_bit_rate = stream->codecpar->bit_rate;

                // 输出文件的音频编码标准
                if (wantIndex.audio_codecId == AV_CODEC_ID_NONE) {  // 取第一个出现的值
                    wantIndex.audio_index = j;
//                    wantIndex.sample_rate = 48000;
//                    wantIndex.ch_layout = 2;
//                    wantIndex.audio_codecId = stream->codecpar->codec_id;
//                    wantIndex.smp_fmt = AV_SAMPLE_FMT_FLTP;
//                    wantIndex.audio_bit_rate = 103445;

                    wantIndex.sample_rate = stream->codecpar->sample_rate;
                    wantIndex.ch_layout = stream->codecpar->channel_layout;
                    wantIndex.audio_codecId = stream->codecpar->codec_id;
                    wantIndex.smp_fmt = (enum AVSampleFormat) stream->codecpar->format;
                    wantIndex.audio_bit_rate = stream->codecpar->bit_rate;

                    LOGD("wantIndex.sample_rate = %d, wantIndex.ch_layout = %ld, wantIndex.audio_codecId = %d, wantIndex.smp_fmt = %d, wantIndex.audio_bit_rate = %ld",
                         wantIndex.sample_rate, wantIndex.ch_layout, wantIndex.audio_codecId,
                         wantIndex.smp_fmt,
                         wantIndex.audio_bit_rate);
                }

                dectx.audio_de_ctx = ctx;
            }
        }
        in_indexes.push_back(index);
        de_ctxs.push_back(dectx);
    }
    return true;
}

static int select_sample_rate(AVCodec *codec, int rate) {
    int best_rate = 0;
    int deft_rate = 44100;
    bool surport = false;
    const int *p = codec->supported_samplerates;
    if (!p) {
        return deft_rate;
    }
    while (*p) {
        best_rate = *p;
        if (*p == rate) {
            surport = true;
            break;
        }
        p++;
    }

    if (best_rate != rate && best_rate != 0 && best_rate != deft_rate) {
        return deft_rate;
    }

    return best_rate;
}

bool Merge::addStream() {
    int ret = 0;
    if ((ret = avformat_alloc_output_context2(&ou_fmt, NULL, NULL, dstpath.c_str())) < 0) {
        LOGD("avformat_alloc_out_context2 fail %s", av_err2str(ret));
        releaseSources();
        return false;
    }

    if (wantIndex.video_codecId != AV_CODEC_ID_NONE) {

        // 添加视频流
        AVStream *stream = avformat_new_stream(ou_fmt, NULL);
        video_ou_index = stream->index;

        AVCodec *codec = avcodec_find_encoder(wantIndex.video_codecId);
        AVCodecContext *ctx = avcodec_alloc_context3(codec);
        if (ctx == NULL) {
            LOGD("find ctx fail");
            releaseSources();
            return false;
        }
        // 设置编码参数
        ctx->pix_fmt = wantIndex.pix_fmt;
        ctx->width = encoderWidth;
        ctx->height = encoderHeight;
        ctx->bit_rate = encoderBitrate;
        ctx->time_base = (AVRational) {1, wantIndex.fps * 100};
        ctx->framerate = (AVRational) {wantIndex.fps, 1};
        stream->time_base = ctx->time_base;

        if (ou_fmt->oformat->flags & AVFMT_GLOBALHEADER) {
            ctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
        }
        if (codec->id == AV_CODEC_ID_H264) {
            ctx->flags2 |= AV_CODEC_FLAG2_LOCAL_HEADER;
        }

        // 初始化上下文
        if ((ret = avcodec_open2(ctx, codec, NULL)) < 0) {
            LOGD("avcodec_open2 fail %s", av_err2str(ret));
            releaseSources();
            return false;
        }

        if ((ret = avcodec_parameters_from_context(stream->codecpar, ctx)) < 0) {
            LOGD("avcodec_parameters_from_context fail %s", av_err2str(ret));
            releaseSources();
            return false;
        }

        LOGD("ctx->width = %d, ctx->height = %d", ctx->width, ctx->height);
        video_en_ctx = ctx;
    }

    if (wantIndex.audio_codecId != AV_CODEC_ID_NONE) {
        AVStream *stream = avformat_new_stream(ou_fmt, NULL);
        audio_ou_index = stream->index;
        AVCodec *codec = avcodec_find_encoder(wantIndex.audio_codecId);
        AVCodecContext *ctx = avcodec_alloc_context3(codec);
        if (ctx == NULL) {
            LOGD("avcodec alloc fail");
            releaseSources();
            return false;
        }
        // 设置音频编码参数
        ctx->sample_fmt = wantIndex.smp_fmt;
        ctx->channel_layout = wantIndex.ch_layout;
        int relt_sample_rate = select_sample_rate(codec, wantIndex.sample_rate);
        if (relt_sample_rate == 0) {
            LOGD("cannot surpot sample_rate");
            releaseSources();
            return false;
        }
        LOGD("relt_sample_rate = %d", relt_sample_rate);
        ctx->sample_rate = relt_sample_rate;
        ctx->bit_rate = wantIndex.audio_bit_rate;
        ctx->time_base = (AVRational) {1, ctx->sample_rate};
        stream->time_base = ctx->time_base;
        if (ou_fmt->oformat->flags & AVFMT_GLOBALHEADER) {
            ctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
        }
        // 初始化编码器
        if ((ret = avcodec_open2(ctx, codec, NULL)) < 0) {
            LOGD("aovcec_open2() fail %s", av_err2str(ret));
            releaseSources();
            return false;
        }
        if (avcodec_parameters_from_context(stream->codecpar, ctx) < 0) {
            LOGD("avcodec_parameters_from_context fail");
            releaseSources();
            return false;
        }

        audio_en_ctx = ctx;
    }

    // 打开输入上下文及写入头信息
    if (!(ou_fmt->oformat->flags & AVFMT_NOFILE)) {
        if (avio_open2(&ou_fmt->pb, dstpath.c_str(), AVIO_FLAG_WRITE, NULL, NULL) < 0) {
            LOGD("avio_open2 fail");
            releaseSources();
            return false;
        }
    }
    if (avformat_write_header(ou_fmt, NULL) < 0) {
        LOGD("avformat_write_header fail");
        releaseSources();
        return false;
    }

    return true;
}

void Merge::updateAVFrame() {
    if (video_en_frame) {
        av_frame_unref(video_en_frame);
    }
    if (yuv_frame) {
        av_frame_unref(yuv_frame);
    }
    if (audio_en_frame) {
        av_frame_unref(audio_en_frame);
    }
    if (video_en_ctx) {
        video_en_frame = av_frame_alloc();
        yuv_frame = av_frame_alloc();
        video_de_frame = av_frame_alloc();
        video_en_frame->format = video_en_ctx->pix_fmt;
        video_en_frame->width = curIndex.width;
        video_en_frame->height = curIndex.height;
        av_frame_get_buffer(video_en_frame, 0);
        av_frame_make_writable(video_en_frame);

        yuv_frame->format = video_en_ctx->pix_fmt;
        if (curIndex.rotate != 0) {
            yuv_frame->width = curIndex.height;
            yuv_frame->height = curIndex.width;
        } else {
            yuv_frame->width = curIndex.width;
            yuv_frame->height = curIndex.height;
        }

        av_frame_get_buffer(yuv_frame, 0);
        av_frame_make_writable(yuv_frame);
    }

    if (audio_en_ctx) {
        audio_en_frame = av_frame_alloc();
        audio_de_frame = av_frame_alloc();
        audio_en_frame->format = audio_en_ctx->sample_fmt;
        audio_en_frame->channel_layout = audio_en_ctx->channel_layout;
        audio_en_frame->sample_rate = audio_en_ctx->sample_rate;
        audio_en_frame->nb_samples = audio_en_ctx->frame_size;
        av_frame_get_buffer(audio_en_frame, 0);
        av_frame_make_writable(audio_en_frame);
    }
}

void Merge::doDecode(AVPacket *pkt, bool isVideo) {
    DeCtx dectx = de_ctxs[curIndex.file_index];
    AVCodecContext *ctx = NULL;
    AVFrame *de_frame = NULL;
    AVFrame *en_frame = NULL;
    if (isVideo) {   // 说明是视频
        ctx = dectx.video_de_ctx;
        de_frame = video_de_frame;
        en_frame = video_en_frame;
    } else {    // 说明是音频
        ctx = dectx.audio_de_ctx;
        de_frame = audio_de_frame;
        en_frame = audio_en_frame;
        isVideo = false;
    }
    if (ctx == NULL) {
        LOGD("%d error avcodecxtx", curIndex);
        releaseSources();
        return;
    }

    // 开始解码
    int ret = 0;
    avcodec_send_packet(ctx, pkt);
    while (true) {
        ret = avcodec_receive_frame(ctx, de_frame);
        if (ret == AVERROR_EOF && curIndex.file_index + 1 == in_fmts.size()) {   // 说明解码完毕
            doEncode(NULL, ctx->width > 0);
            LOGD("decode finish");
            break;
        } else if (ret < 0) {
            break;
        }

        // 解码成功;开始编码
        doConvert(&en_frame, de_frame, de_frame->width > 0);
    }
}

void Merge::doConvert(AVFrame **dst, AVFrame *src, bool isVideo) {
    int ret = 0;
    LOGD("(*dst)->width1 = %d, src->width1 = %d", (*dst)->width, src->width);
    LOGD("(*dst)->format = %d, src->format = %d", (*dst)->format, src->format);
    //handle video
    if (isVideo && (src->width != (*dst)->width || src->format != (*dst)->format)) {
        MediaIndex index = in_indexes[curIndex.file_index];

        LOGD("index.width = %d, index.height = %d, index.pix_fmt = %d, index.rotate = %d",
             index.width, index.height,
             index.pix_fmt, index.rotate);
        LOGD("video_en_ctx.width = %d, video_en_ctx height = %d, video_en_ctx pix_fmt = %d",
             video_en_ctx->width,
             video_en_ctx->height, video_en_ctx->pix_fmt);

        if (!swsctx) {
            swsctx = sws_getContext(index.width, index.height, index.pix_fmt,
                                    index.width, index.height,
                                    video_en_ctx->pix_fmt,
                                    SWS_FAST_BILINEAR, NULL, NULL, NULL);
            if (swsctx == NULL) {
                LOGD("sws_getContext fail");
                releaseSources();
                return;
            }
        }

        if ((ret = sws_scale(swsctx, src->data, src->linesize, 0, src->height, (*dst)->data,
                             (*dst)->linesize)) < 0) {
            LOGD("sws_scale fail %s", av_err2str(ret));
            releaseSources();
            return;
        }

        LOGD("(*dst)->width = %d, (*dst)->height = %d", (*dst)->width, (*dst)->height);


//        uint8 *i420_scale_data = (uint8 *)malloc(sizeof(uint8) * (*dst)->width * (*dst)->height * 3 / 2);
//        rotateI420((*dst)->data[0], (*dst)->width, (*dst)->height, i420_scale_data, curIndex.rotate);
//        scaleI420((*dst)->data[0], (*dst)->width, (*dst)->height, i420_scale_data, 1280, 720, 0);
//        (*dst)->data[0] = i420_scale_data;

//        preFile << (*dst)->data[0];
//        preFile << (*dst)->data[1];
//        preFile << (*dst)->data[2];

        if (curIndex.rotate == 90 || curIndex.rotate == 270) {
            int whSize = (*dst)->width * (*dst)->height;
            int uv_stride = (*dst)->width / 2;
            int uv_length = uv_stride * (*dst)->height / 2;
            uint8_t *yuvBuffer = (uint8_t *) malloc(whSize * 3 / 2);
            LOGD("sizeof(yuvBuffer) = %lu", sizeof(yuvBuffer) * whSize * 3 / 2);
            memset(yuvBuffer, 0, whSize * 3 / 2);
            uint8_t *Y_data_Dst = yuvBuffer;
            uint8_t *U_data_Dst = yuvBuffer + whSize;
            uint8_t *V_data_Dst = U_data_Dst + uv_length;

            libyuv::I420Rotate((*dst)->data[0], (*dst)->linesize[0],
                               (*dst)->data[1], (*dst)->linesize[1],
                               (*dst)->data[2], (*dst)->linesize[2],
                               Y_data_Dst, (*dst)->height,
                               U_data_Dst, (*dst)->height >> 1,
                               V_data_Dst, (*dst)->height >> 1,
                               (*dst)->width, (*dst)->height,
                               (libyuv::RotationMode) curIndex.rotate);

            yuv_frame->data[0] = Y_data_Dst;
            yuv_frame->data[1] = U_data_Dst;
            yuv_frame->data[2] = V_data_Dst;

            //写文件
//        outFile<< yuvBuffer;

            LOGD("(*dst)->width2 = %d, (*dst)->height2 = %d", (*dst)->width, (*dst)->height);
            LOGD("(*dst)->linesize[0] = %d, (*dst)->linesize[1] = %d, (*dst)->linesize[2] = %d",
                 (*dst)->linesize[0], (*dst)->linesize[1], (*dst)->linesize[2]);
            yuv_frame->linesize[0] = (*dst)->height;
            yuv_frame->linesize[1] = (*dst)->height / 2;
            yuv_frame->linesize[2] = (*dst)->height / 2;

        } else {
            yuv_frame->data[0] = (*dst)->data[0];
            yuv_frame->data[1] = (*dst)->data[1];
            yuv_frame->data[2] = (*dst)->data[2];

            yuv_frame->linesize[0] = (*dst)->width;
            yuv_frame->linesize[1] = (*dst)->width / 2;
            yuv_frame->linesize[2] = (*dst)->width / 2;
        }

        int scaleW = encoderWidth;
        int scaleH = encoderHeight;
        int y_size = scaleW * scaleH;
        int u_size = (scaleW >> 1) * (scaleH >> 1);
        uint8_t *scaleBuffer = (uint8_t *) malloc(y_size * 3 / 2);
        uint8_t *Y_scale_Dst = scaleBuffer;
        uint8_t *U_scale_Dst = scaleBuffer + y_size;
        uint8_t *V_scale_Dst = U_scale_Dst + u_size;

        libyuv::I420Scale(yuv_frame->data[0], yuv_frame->linesize[0],
                          yuv_frame->data[1], yuv_frame->linesize[1],
                          yuv_frame->data[2], yuv_frame->linesize[2],
                          (*dst)->height, (*dst)->width,
                          Y_scale_Dst, scaleW,
                          U_scale_Dst, scaleW >> 1,
                          V_scale_Dst, scaleW >> 1,
                          scaleW, scaleH, (libyuv::FilterMode) 0);

        yuv_frame->data[0] = Y_scale_Dst;
        yuv_frame->data[1] = U_scale_Dst;
        yuv_frame->data[2] = V_scale_Dst;

        LOGD("yuv_frame->linesize[0] = %d, yuv_frame->linesize[1] = %d, yuv_frame->linesize[2] = %d",
             yuv_frame->linesize[0], yuv_frame->linesize[1], yuv_frame->linesize[2]);

        yuv_frame->linesize[0] = scaleW;
        yuv_frame->linesize[1] = scaleW >> 1;
        yuv_frame->linesize[2] = scaleW >> 1;

        LOGD("yuv_frame->linesize[0] = %d, yuv_frame->linesize[1] = %d, yuv_frame->->linesize[2] = %d",
             yuv_frame->linesize[0], yuv_frame->linesize[1], yuv_frame->linesize[2]);
        LOGD("(*dst)->width3 = %d, (*dst)->height3 = %d", (*dst)->width, (*dst)->height);


        /** 遇到问题：合成后的视频时长不是各个视频文件视频时长之和
         *  分析原因：因为每个视频文件的fps不一样，合并时pts没有和每个视频文件的fps对应
         *  解决方案：合并时pts要和每个视频文件的fps对应
         */
        yuv_frame->pts = next_video_pts + video_en_ctx->time_base.den / curIndex.fps;
        next_video_pts += video_en_ctx->time_base.den / curIndex.fps;
        doEncode(yuv_frame, yuv_frame->width > 0);

    } else if (isVideo && (src->width == (*dst)->width && src->format == (*dst)->format)) {
        LOGD("直接复制视频流");
        av_frame_copy((*dst), src);

        int scaleW = encoderWidth;
        int scaleH = encoderHeight;
        int y_size = scaleW * scaleH;
        int u_size = (scaleW >> 1) * (scaleH >> 1);
        uint8_t *scaleBuffer = (uint8_t *) malloc(y_size * 3 / 2);
        uint8_t *Y_scale_Dst = scaleBuffer;
        uint8_t *U_scale_Dst = scaleBuffer + y_size;
        uint8_t *V_scale_Dst = U_scale_Dst + u_size;

        libyuv::I420Scale((*dst)->data[0], (*dst)->linesize[0],
                          (*dst)->data[1], (*dst)->linesize[1],
                          (*dst)->data[2], (*dst)->linesize[2],
                          (*dst)->width, (*dst)->height,
                          Y_scale_Dst, scaleW,
                          U_scale_Dst, scaleW >> 1,
                          V_scale_Dst, scaleW >> 1,
                          scaleW, scaleH, (libyuv::FilterMode) 0);

        yuv_frame->data[0] = Y_scale_Dst;
        yuv_frame->data[1] = U_scale_Dst;
        yuv_frame->data[2] = V_scale_Dst;

        LOGD("yuv_frame->linesize[0] = %d, yuv_frame->linesize[1] = %d, yuv_frame->linesize[2] = %d",
             yuv_frame->linesize[0], yuv_frame->linesize[1], yuv_frame->linesize[2]);

        yuv_frame->linesize[0] = scaleW;
        yuv_frame->linesize[1] = scaleW >> 1;
        yuv_frame->linesize[2] = scaleW >> 1;


        yuv_frame->pts = next_video_pts + video_en_ctx->time_base.den / curIndex.fps;
        next_video_pts += video_en_ctx->time_base.den / curIndex.fps;
        doEncode(yuv_frame, yuv_frame->width > 0);
    }

    //handle audio
    if (!isVideo && (src->sample_rate != (*dst)->sample_rate || src->format != (*dst)->format ||
                     src->channel_layout != (*dst)->channel_layout)) {

        if (!swrctx) {
            swrctx = swr_alloc_set_opts(NULL, audio_en_ctx->channel_layout,
                                        audio_en_ctx->sample_fmt, audio_en_ctx->sample_rate,
                                        src->channel_layout, (enum AVSampleFormat) src->format,
                                        src->sample_rate, 0, NULL);
            if ((ret = swr_init(swrctx)) < 0) {
                LOGD("swr_alloc_set_opts() fail %d", ret);
                releaseSources();
                return;
            }
        }

        int dst_nb_samples = (int) av_rescale_rnd(
                swr_get_delay(swrctx, src->sample_rate) + src->nb_samples, (*dst)->sample_rate,
                src->sample_rate, AV_ROUND_UP);
        if (dst_nb_samples != (*dst)->nb_samples) {

            // 进行转化
            /** 遇到问题：音频合并后时长并不是两个文件之和
             *  分析原因：当进行音频采样率重采样时，如采样率升采样，那么swr_convert()内部会进行插值运算，这样相对于原先的nb_samples会多出一些，所以swr_convert()可能需要多次
             *  调用才可以将所有数据取完，那么这就需要建立一个缓冲区来自己组装新采样率下的AVFrame中的音频数据
             *  解决方案：建立音频缓冲区，依次组装所有的音频数据并且和pts对应上
             */
            if (!audio_init) {
                ret = av_samples_alloc_array_and_samples(&audio_buffer, (*dst)->linesize,
                                                         (*dst)->channels, (*dst)->nb_samples,
                                                         (enum AVSampleFormat) (*dst)->format, 0);
                audio_init = true;
                left_size = 0;
            }
            bool first = true;
            while (true) {
                // 进行转换
                if (first) {
                    ret = swr_convert(swrctx, audio_buffer, (*dst)->nb_samples,
                                      (const uint8_t **) audio_de_frame->data,
                                      audio_de_frame->nb_samples);
                    if (ret < 0) {
                        LOGD("swr_convert() fail %d", ret);
                        releaseSources();
                        return;
                    }
                    first = false;

                    int use = ret - left_size >= 0 ? ret - left_size : ret;
                    int size = av_get_bytes_per_sample(
                            (enum AVSampleFormat) audio_en_frame->format);
                    for (int ch = 0; ch < audio_en_frame->channels; ch++) {
                        for (int i = 0; i < use; i++) {
                            (*dst)->data[ch][(i + left_size) * size] = audio_buffer[ch][i * size];
                            (*dst)->data[ch][(i + left_size) * size + 1] = audio_buffer[ch][
                                    i * size + 1];
                            (*dst)->data[ch][(i + left_size) * size + 2] = audio_buffer[ch][
                                    i * size + 2];
                            (*dst)->data[ch][(i + left_size) * size + 3] = audio_buffer[ch][
                                    i * size + 3];
                        }
                    }
                    // 编码
                    left_size += ret;
                    if (left_size >= (*dst)->nb_samples) {
                        left_size -= (*dst)->nb_samples;
                        // 编码
                        (*dst)->pts = av_rescale_q(next_audio_pts,
                                                   (AVRational) {1, (*dst)->sample_rate},
                                                   audio_en_ctx->time_base);
                        next_audio_pts += (*dst)->nb_samples;
                        doEncode((*dst), (*dst)->width > 0);

                        if (left_size > 0) {
                            int size = av_get_bytes_per_sample(
                                    (enum AVSampleFormat) audio_en_frame->format);
                            for (int ch = 0; ch < (*dst)->channels; ch++) {
                                for (int i = 0; i < left_size; i++) {
                                    (*dst)->data[ch][i * size] = audio_buffer[ch][(use + i) * size];
                                    (*dst)->data[ch][i * size + 1] = audio_buffer[ch][
                                            (use + i) * size + 1];
                                    (*dst)->data[ch][i * size + 2] = audio_buffer[ch][
                                            (use + i) * size + 2];
                                    (*dst)->data[ch][i * size + 3] = audio_buffer[ch][
                                            (use + i) * size + 3];
                                }
                            }
                        }
                    }

                } else {
                    ret = swr_convert(swrctx, audio_buffer, (*dst)->nb_samples, NULL, 0);
                    if (ret < 0) {
                        LOGD("1 swr_convert() fail %d", ret);
                        releaseSources();
                        return;
                    }
                    int size = av_get_bytes_per_sample(
                            (enum AVSampleFormat) audio_en_frame->format);
                    for (int ch = 0; ch < audio_en_frame->channels; ch++) {
                        for (int i = 0;
                             i < ret && i + left_size < audio_en_frame->nb_samples; i++) {
                            (*dst)->data[ch][(left_size + i) * size] = audio_buffer[ch][i * size];
                            (*dst)->data[ch][(left_size + i) * size + 1] = audio_buffer[ch][
                                    i * size + 1];
                            (*dst)->data[ch][(left_size + i) * size + 2] = audio_buffer[ch][
                                    i * size + 2];
                            (*dst)->data[ch][(left_size + i) * size + 3] = audio_buffer[ch][
                                    i * size + 3];
                        }
                    }
                    left_size += ret;
                    if (left_size >= (*dst)->nb_samples) {
                        left_size -= (*dst)->nb_samples;
                        LOGD("多了一个编码");
                        // 编码
                        (*dst)->pts = av_rescale_q(next_audio_pts,
                                                   (AVRational) {1, (*dst)->sample_rate},
                                                   audio_en_ctx->time_base);
                        next_audio_pts += audio_en_frame->nb_samples;
                        doEncode((*dst), (*dst)->width > 0);
                    } else {
                        break;
                    }
                }
            }
        }

    } else if (!isVideo &&
               (src->nb_samples == (*dst)->nb_samples || src->sample_rate == (*dst)->sample_rate ||
                src->format == (*dst)->format || src->channel_layout == (*dst)->channel_layout)) {
        av_frame_copy((*dst), src);
        /** 遇到问题：合并后音频和视频不同步
         *  分析原因：因为每个音频文件的采样率，而合并是按照第一个文件的采样率作为time_base的，没有转换过来
         *  解决方案：合并时pts要和每个合并前音频的采样率对应上
         */
        src->pts = next_audio_pts;
        next_audio_pts =
                src->pts + audio_en_ctx->time_base.den / curIndex.sample_rate * src->nb_samples;
        LOGD("next_audio_pts %d", next_audio_pts);
        doEncode(src, src->width > 0);
    }
}

void Merge::doEncode(AVFrame *frame, bool isVideo) {
    AVCodecContext *ctx = NULL;
    if (isVideo) {  // 说明是视频
        ctx = video_en_ctx;
    } else {
        ctx = audio_en_ctx;
    }

    int ret = avcodec_send_frame(ctx, frame);
    while (true) {
        AVPacket *pkt = av_packet_alloc();
        ret = avcodec_receive_packet(ctx, pkt);
        if (ret < 0) {
//            LOGD("ret %s",av_err2str(ret));
            av_packet_unref(pkt);
            break;
        }

        // 编码成功;写入文件,写入之前改变一下pts,dts和duration(因为此时pkt的pts是基于AVCodecContext的，要转换成基于AVFormatContext的)
        int index = ctx->width > 0 ? video_ou_index : audio_ou_index;
        AVStream *stream = ou_fmt->streams[index];
        av_packet_rescale_ts(pkt, ctx->time_base, stream->time_base);
        pkt->stream_index = index;
        LOGD("%s pts %ld(%s) dts %d(%s)", pkt->stream_index == video_ou_index ? "video" : "audio",
             pkt->pts, av_ts2timestr(pkt->pts, &stream->time_base), pkt->pts,
             av_ts2timestr(pkt->dts, &stream->time_base));
        if ((ret = av_write_frame(ou_fmt, pkt)) < 0) {
            LOGD("av_write_frame fail %d", ret);
            releaseSources();
            return;
        }
        av_packet_unref(pkt);
    }
}

void Merge::addMusic(const string& video_path, const string& audio_path, const string& dst_path, const string& start_time) {

    //MP4文件标准封装为h264和aac格式 目前暂不支持mp3音频文件
    string start = start_time;
    if (start_time.length() <= 8) {
        start = "00:00:00";
    }

    //stoi() 把数字字符串转换成int输出
    start_pos = 0;
    start_pos += stoi(start.substr(0, 2)) * 3600;
    start_pos += stoi(start.substr(3, 2)) * 60;
    start_pos += stoi(start.substr(6, 2));

    in_fmt1 = NULL;// 用于解封装视频
    in_fmt2 = NULL;// 用于解封装音频
    ou_fmt = NULL;  //用于封装音视频

    int ret = 0;
    // 打开视频文件
    if ((ret = avformat_open_input(&in_fmt1, video_path.c_str(), NULL, NULL)) < 0) {
        LOGD("1 avformat_open_input() fail %s", av_err2str(ret));
        return;
    }
    if ((ret = avformat_find_stream_info(in_fmt1, NULL)) < 0) {
        LOGD("1 avformat_find_stream_info %s", av_err2str(ret));
        releaseSources();
        return;
    }

    // 打开音频文件
    if ((ret = avformat_open_input(&in_fmt2, audio_path.c_str(), NULL, NULL)) < 0) {
        LOGD("2 avformat_open_input() fail %s", av_err2str(ret));
        return;
    }
    if ((ret = avformat_find_stream_info(in_fmt2, NULL)) < 0) {
        LOGD("2 avformat_find_stream_info %s", av_err2str(ret));
        releaseSources();
        return;
    }

    for (int i = 0; i < in_fmt1->nb_streams; i++) {
        AVStream *stream = in_fmt1->streams[i];
        if (stream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            video1_in_index = i;
            break;
        }
    }
    for (int i = 0; i < in_fmt2->nb_streams; i++) {
        AVStream *stream = in_fmt2->streams[i];
        if (stream->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            audio1_in_index = i;
            break;
        }
    }

    // 打开输出文件的封装器
    if (avformat_alloc_output_context2(&ou_fmt, NULL, NULL, dst_path.c_str()) < 0) {
        LOGD("avformat_alloc_out_context2 ()");
        releaseSources();
        return;
    }

    // 添加视频流并从输入源拷贝视频流编码参数
    AVStream *stream = avformat_new_stream(ou_fmt, NULL);
    video_ou_index = stream->index;
    if (avcodec_parameters_copy(stream->codecpar, in_fmt1->streams[video1_in_index]->codecpar) <
        0) {
        releaseSources();
        return;
    }
    // 如果源和目标文件的码流格式不一致，则将目标文件的code_tag赋值为0
    if (av_codec_get_id(ou_fmt->oformat->codec_tag,
                        in_fmt1->streams[video1_in_index]->codecpar->codec_tag) !=
        stream->codecpar->codec_id) {
        stream->codecpar->codec_tag = 0;
    }

    // 添加音频流并从输入源拷贝编码参数
    AVStream *a_stream = avformat_new_stream(ou_fmt, NULL);
    audio_ou_index = a_stream->index;
    if (avcodec_parameters_copy(a_stream->codecpar, in_fmt2->streams[audio1_in_index]->codecpar) <
        0) {
        LOGD("avcodec_parameters_copy fail");
        releaseSources();
        return;
    }
    if (av_codec_get_id(ou_fmt->oformat->codec_tag,
                        in_fmt2->streams[audio1_in_index]->codecpar->codec_tag) !=
        a_stream->codecpar->codec_id) {
        a_stream->codecpar->codec_tag = 0;
    }

    // 打开输出上下文
    if (!(ou_fmt->oformat->flags & AVFMT_NOFILE)) {
        if (avio_open2(&ou_fmt->pb, dst_path.c_str(), AVIO_FLAG_WRITE, NULL, NULL) < 0) {
            LOGD("avio_open2() fail");
            releaseSources();
            return;
        }
    }

    // 写入头文件
    if (avformat_write_header(ou_fmt, NULL) < 0) {
        LOGD("avformat_write_header()");
        releaseSources();
        return;
    }

    AVPacket *v_pkt = av_packet_alloc();
    AVPacket *a_pkt = av_packet_alloc();
    // 写入视频
    int64_t video_max_pts = 0;
    while (av_read_frame(in_fmt1, v_pkt) >= 0) {
        if (v_pkt->stream_index == video1_in_index) {   // 说明是视频
            // 因为源文件和目的文件时间基可能不一致，所以这里要进行转换
            av_packet_rescale_ts(v_pkt, in_fmt1->streams[video1_in_index]->time_base,
                                 ou_fmt->streams[video_ou_index]->time_base);
            v_pkt->stream_index = video_ou_index;
            video_max_pts = max(v_pkt->pts, video_max_pts);
            LOGD("video pts %d(%s)", v_pkt->pts, av_ts2timestr(v_pkt->pts, &stream->time_base));
            if (av_write_frame(ou_fmt, v_pkt) < 0) {
                LOGD("1 av_write_frame < 0");
                releaseSources();
                return;
            }
        }
    }

    int64_t start_pts = start_pos * a_stream->time_base.den;
    video_max_pts = av_rescale_q(video_max_pts, stream->time_base, a_stream->time_base);
    // 写入音频
    while (av_read_frame(in_fmt2, a_pkt) >= 0) {
        if (a_pkt->stream_index == audio1_in_index) {   // 音频

            // 源文件和目标文件的时间基可能不一致，需要转化
            av_packet_rescale_ts(a_pkt, in_fmt2->streams[audio1_in_index]->time_base,
                                 ou_fmt->streams[audio_ou_index]->time_base);
            // 保证以视频时间轴的指定时间进行添加，那么实际上就是改变pts及dts的值即可
            a_pkt->pts += start_pts;
            a_pkt->dts += start_pts;
            a_pkt->stream_index = audio_ou_index;
            LOGD("audio pts %d(%s)", a_pkt->pts, av_ts2timestr(a_pkt->pts, &a_stream->time_base));
            // 加入音频的时长不能超过视频的总时长
            if (a_pkt->pts >= video_max_pts) {
                break;
            }

            if (av_write_frame(ou_fmt, a_pkt) < 0) {
                LOGD("2 av_write_frame < 0");
                releaseSources();
                return;
            }
        }
    }

    av_write_trailer(ou_fmt);
    LOGD("写入完毕");

    // 释放资源
    releaseSources();
}

void Merge::deleteAudio(string videoSrcPath, string videoDstPath) {

    in_fmt1 = NULL;// 用于解封装视频
    ou_fmt = NULL;  //用于封装音视频

    int ret = 0;
    // 打开视频文件
    if ((ret = avformat_open_input(&in_fmt1, videoSrcPath.c_str(), NULL, NULL)) < 0) {
        LOGD("deleteAudio avformat_open_input() fail %s", av_err2str(ret));
        return;
    }
    if ((ret = avformat_find_stream_info(in_fmt1, NULL)) < 0) {
        LOGD("deleteAudio avformat_find_stream_info %s", av_err2str(ret));
        releaseSources();
        return;
    }

    for (int i = 0; i < in_fmt1->nb_streams; i++) {
        AVStream *stream = in_fmt1->streams[i];
        if (stream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            video1_in_index = i;
            break;
        }
    }

    // 打开输出文件的封装器
    if (avformat_alloc_output_context2(&ou_fmt, NULL, NULL, videoDstPath.c_str()) < 0) {
        LOGD("deleteAudio avformat_alloc_out_context2 ()");
        releaseSources();
        return;
    }

    // 添加视频流并从输入源拷贝视频流编码参数
    AVStream *stream = avformat_new_stream(ou_fmt, NULL);
    video_ou_index = stream->index;
    if (avcodec_parameters_copy(stream->codecpar, in_fmt1->streams[video1_in_index]->codecpar) <
        0) {
        releaseSources();
        return;
    }
    // 如果源和目标文件的码流格式不一致，则将目标文件的code_tag赋值为0
    if (av_codec_get_id(ou_fmt->oformat->codec_tag,
                        in_fmt1->streams[video1_in_index]->codecpar->codec_tag) !=
        stream->codecpar->codec_id) {
        stream->codecpar->codec_tag = 0;
    }

    // 打开输出上下文
    if (!(ou_fmt->oformat->flags & AVFMT_NOFILE)) {
        if (avio_open2(&ou_fmt->pb, videoDstPath.c_str(), AVIO_FLAG_WRITE, NULL, NULL) < 0) {
            LOGD("deleteAudio avio_open2() fail");
            releaseSources();
            return;
        }
    }

    // 写入头文件
    if (avformat_write_header(ou_fmt, NULL) < 0) {
        LOGD("deleteAudio avformat_write_header()");
        releaseSources();
        return;
    }

    AVPacket *v_pkt = av_packet_alloc();
    // 写入视频
    int64_t video_max_pts = 0;
    while (av_read_frame(in_fmt1, v_pkt) >= 0) {
        if (v_pkt->stream_index == video1_in_index) {   // 说明是视频
            // 因为源文件和目的文件时间基可能不一致，所以这里要进行转换
            av_packet_rescale_ts(v_pkt, in_fmt1->streams[video1_in_index]->time_base,
                                 ou_fmt->streams[video_ou_index]->time_base);
            v_pkt->stream_index = video_ou_index;
            video_max_pts = max(v_pkt->pts, video_max_pts);
            LOGD("deleteAudio video pts %d(%s)", v_pkt->pts, av_ts2timestr(v_pkt->pts, &stream->time_base));
            if (av_write_frame(ou_fmt, v_pkt) < 0) {
                LOGD("1 av_write_frame < 0");
                releaseSources();
                return;
            }
        }
    }

    av_write_trailer(ou_fmt);
    LOGD("写入完毕");

    // 释放资源
    releaseSources();
}

void Merge::releaseSources() {
    if (in_fmt1) {
        avformat_close_input(&in_fmt1);
        in_fmt1 = NULL;
    }

    if (in_fmt2) {
        avformat_close_input(&in_fmt2);
        in_fmt2 = NULL;
    }

    if (ou_fmt) {
        avformat_free_context(ou_fmt);
        ou_fmt = NULL;
    }

    for (int i = 0; i < in_fmts.size(); i++) {
        AVFormatContext *fmt = in_fmts[i];
        avformat_close_input(&fmt);
    }

    for (int i = 0; i < de_ctxs.size(); i++) {
        DeCtx ctx = de_ctxs[i];
        if (ctx.video_de_ctx) {
            avcodec_free_context(&ctx.video_de_ctx);
            ctx.video_de_ctx = NULL;
        }
        if (ctx.audio_de_ctx) {
            avcodec_free_context(&ctx.audio_de_ctx);
            ctx.audio_de_ctx = NULL;
        }
    }

    if (video_en_ctx) {
        avcodec_free_context(&video_en_ctx);
        video_en_ctx = NULL;
    }

    if (audio_en_ctx) {
        avcodec_free_context(&audio_en_ctx);
        audio_en_ctx = NULL;
    }

    if (swsctx) {
        sws_freeContext(swsctx);
        swsctx = NULL;
    }

    if (swrctx) {
        swr_free(&swrctx);
        swrctx = NULL;
    }

    if (video_de_frame) {
        av_frame_free(&video_de_frame);
    }

    if (audio_de_frame) {
        av_frame_free(&audio_de_frame);
    }
    if (video_en_frame) {
        av_frame_free(&video_en_frame);
    }
    if (outFile) {
        outFile.close();
    }
    if (preFile) {
        preFile.close();
    }
    if (yuv_frame) {
        av_frame_free(&yuv_frame);
    }
    if (audio_en_frame) {
        av_frame_free(&audio_en_frame);
    }
    if (audio_buffer) {
        av_freep(&audio_buffer[0]);
    }
}