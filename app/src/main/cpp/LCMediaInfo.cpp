//
// Created by Administrator on 2021/11/10.
//

#include "LCMediaInfo.h"

LCMediaInfo::LCMediaInfo()
{
    av_register_all();

    resetAllMediaPlayerParameters();
}

LCMediaInfo::~LCMediaInfo()
{

}

void LCMediaInfo::StopPlayVideo()
{
    m_vStreamTimeRational = av_make_q(0,0);
    m_aStreamTimeRational = av_make_q(0,0);

    UnInitVideoCodec();

    resetAllMediaPlayerParameters();

    LOGD("======STOP PLAY VIDEO SUCCESS========");
}

void LCMediaInfo::SetVideoFilePath(std::string& path)
{
    m_videoPathString = path;
}

int LCMediaInfo::InitVideoCodec()
{
    if (m_videoPathString == "") {
        LOGE("file path is Empty...");
        return -1;
    }

    const char * filePath = m_videoPathString.c_str();

    if (avformat_open_input(&m_pFormatCtx, filePath, NULL, NULL) != 0)
    {
        LOGE("open input stream failed .");
        return -1;
    }

    if (avformat_find_stream_info(m_pFormatCtx, nullptr) < 0) {
        LOGE("avformat_find_stream_info failed .");
        return -1;
    }

    m_videoStreamIdx = -1;
    m_audioStreamIdx = -1;

    for (int i = 0; i < (int )m_pFormatCtx->nb_streams; ++i) {
        AVCodecParameters  *codecParameters = m_pFormatCtx->streams[i]->codecpar;

        if (codecParameters->codec_type == AVMEDIA_TYPE_VIDEO) {

            m_videoStreamIdx = i;
            LOGD("video index: %d",m_videoStreamIdx);

            AVCodec *codec = avcodec_find_decoder(codecParameters->codec_id);
            if (codec == nullptr)
            {
                LOGF("Video AVCodec is NULL");
                return -1;
            }

            m_pVideoCodecCtx = avcodec_alloc_context3(codec);
            avcodec_parameters_to_context(m_pVideoCodecCtx, codecParameters);

            if (avcodec_open2(m_pVideoCodecCtx, codec, NULL) < 0)
            {
                LOGF("Could not open codec.");
                return -1;
            }

        } else if (codecParameters->codec_type == AVMEDIA_TYPE_AUDIO) {

            m_audioStreamIdx = i;
            LOGD("Audio index: %d",m_audioStreamIdx);

            AVCodec *codec = avcodec_find_decoder(codecParameters->codec_id);
            if (codec == nullptr)
            {
                LOGF("Audo AVCodec is NULL");
                return -1;
            }

            m_pAudioCodecCtx = avcodec_alloc_context3(codec);
            avcodec_parameters_to_context(m_pAudioCodecCtx, codecParameters);

            if (avcodec_open2(m_pAudioCodecCtx, codec, NULL) < 0)
            {
                LOGF("audio decoder not found.");
                return -1;
            }

            m_sampleRate = m_pAudioCodecCtx->sample_rate;
            m_channel = m_pAudioCodecCtx->channels;
            switch (m_pAudioCodecCtx->sample_fmt)
            {
                case AV_SAMPLE_FMT_U8:
                    m_sampleSize = 8;
                case AV_SAMPLE_FMT_S16:
                    m_sampleSize = 16;
                    break;
                case AV_SAMPLE_FMT_S32:
                    m_sampleSize = 32;
                    break;
                default:
                    break;
            }
        }
    }

    if (m_pVideoCodecCtx != NULL) {
        m_videoWidth = m_pVideoCodecCtx->width;
        m_videoHeight = m_pVideoCodecCtx->height;

        LOGD("Init VideoCodec sucess. Width: %d Height: %d",m_videoWidth,m_videoHeight);
    }

    if (m_videoStreamIdx != -1) {
        AVStream *videoStream = m_pFormatCtx->streams[m_videoStreamIdx];
        m_RotateAngle = getRotateAngle(videoStream);
        m_vStreamTimeRational = videoStream->time_base;
        m_videoFPS            = videoStream->avg_frame_rate.num / videoStream->avg_frame_rate.den;
        LOGD("Init VideoCodec sucess.V Time Base: %d  Den :%d",videoStream->time_base.num,videoStream->time_base.den);
        LOGD("Init VideoCodec sucess.rotate :%d", m_RotateAngle);
    }

    if(m_audioStreamIdx != -1){
        AVStream *audioStream=m_pFormatCtx->streams[m_audioStreamIdx];
        m_aStreamTimeRational = audioStream->time_base;
        LOGD("Init VideoCodec sucess.A Time Base: %d Den  %d",audioStream->time_base.num,audioStream->time_base.den);
    }

    return 0;

}

void LCMediaInfo::UnInitVideoCodec()
{
    LOGD("UnInitVideoCodec...");
    if(m_pAudioCodecCtx != NULL)
    {
        avcodec_close(m_pAudioCodecCtx);
    }

    if(m_pVideoCodecCtx != NULL)
    {
        avcodec_close(m_pVideoCodecCtx);
    }

    if(m_pFormatCtx != NULL)
    {
        avformat_close_input(&m_pFormatCtx);
    }
}

int LCMediaInfo::GetVideoWidth()
{
    return m_videoWidth;
}

int LCMediaInfo::GetVideoHeight()
{
    return m_videoHeight;
}

int LCMediaInfo::GetRotateAngle()
{
    return m_RotateAngle;
}

int LCMediaInfo::GetVideoFps()
{
    return m_videoFPS;
}

void LCMediaInfo::resetAllMediaPlayerParameters()
{
    m_pFormatCtx       = NULL;
    m_pVideoCodecCtx   = NULL;
    m_pAudioCodecCtx   = NULL;

    m_videoWidth   = 0;
    m_videoHeight  = 0;
    m_RotateAngle = 0;
    m_videoFPS = 0;

    m_videoPathString = "";

    m_videoStreamIdx = -1;
    m_audioStreamIdx = -1;

    m_sampleRate = 48000;
    m_sampleSize = 16;
    m_channel    = 2;

    m_vStreamTimeRational = av_make_q(0,0);
    m_aStreamTimeRational = av_make_q(0,0);
}

std::string LCMediaInfo::getFileSuffix(const char* path)
{
    const char* pos = strrchr(path,'.');
    if(pos)
    {
        std::string str(pos+1);
        std::transform(str.begin(),str.end(),str.begin(),::tolower);
        return str;
    }
    return std::string();
}