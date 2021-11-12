//
// Created by Administrator on 2021/11/8.
//

#include "LCVideoWriter.h"
#include "libyuv.h"
#include <sys/time.h>

LCVideoWriter *LCVideoWriter::m_pInstance = NULL;
pthread_mutex_t LCVideoWriter::m_mutex;
LCVideoWriter::Garbage LCVideoWriter::m_garbage;

LCVideoWriter *LCVideoWriter::GetInstance() {
    if (m_pInstance == nullptr) {
        pthread_mutex_init(&m_mutex, 0);

        pthread_mutex_lock(&m_mutex);
        m_pInstance = new LCVideoWriter();
        pthread_mutex_unlock(&m_mutex);
    }

    return m_pInstance;
}

LCVideoWriter::LCVideoWriter()
{
    g_aacEncodeConfig = NULL;

    av_register_all();
    avcodec_register_all();

    pthread_mutex_init(&m_videoWriteMutex, 0);
    pthread_mutex_init(&aacWriterMutex, 0);

    g_aacEncodeConfig = initAudioEncodeConfiguration();

    m_startTimeStamp = 0;
}

LCVideoWriter::~LCVideoWriter() 
{
    releaseAccConfiguration();
}

void LCVideoWriter::StopRecordReleaseAllResources()
{
    pthread_mutex_lock(&m_videoWriteMutex);

    m_bRecording=false;

    writeMp4FileTrail();

    releaseAllRecordResources();

    pthread_mutex_unlock(&m_videoWriteMutex);
}

void LCVideoWriter::releaseAllRecordResources() 
{
    if (m_pAudioCodecCtx)
    {
        avcodec_free_context(&m_pAudioCodecCtx);
    }

    if (m_pVideoCodecCtx)
    {
        avcodec_free_context(&m_pVideoCodecCtx);
    }

    if (m_pFormatCtx)
    {
        avformat_free_context(m_pFormatCtx);//内部会将ic置为NULL
    }

    if (m_pYUVFrame)
    {
        av_frame_free(&m_pYUVFrame);
    }
}

bool LCVideoWriter::StartRecordWithFilePath(const char *file)
{
    if (file == NULL) {
        return false;
    }

    m_pYUVFrame      = NULL;
    m_pAudioCodecCtx = NULL;
    m_pVideoCodecCtx = NULL;
    m_pFormatCtx     = NULL;

    m_filePath = file;

    //封装文件输出上下文
    avformat_alloc_output_context2(&m_pFormatCtx, NULL, NULL, file);
    if (m_pFormatCtx == NULL) {
        LOGD("avformat_alloc_output_context2 failed...");
        return false;
    }

    m_videoFramePts = 0;
    m_audioFramePts = 0;
    
    addVideoStream();
    addAudioStream();

    if (!writeVideoHeader()) {
        writeMp4FileTrail();
        releaseAllRecordResources();
        LOGD("write video header failed...");
        return false;
    }

    m_bRecording = true;

    m_startTimeStamp = GetTickCount();

    return true;
}

bool LCVideoWriter::addVideoStream()
{
    if (m_pFormatCtx == NULL) {
        return false;
    }
    
    //1.视频编码器创建
    AVCodec* codec = avcodec_find_encoder(AV_CODEC_ID_H264);
    if (!codec) {
        LOGD("avcodec_find_encoder AV_CODEC_ID_H264 failed!");
        return false;
    }

    m_pVideoCodecCtx = avcodec_alloc_context3(codec);
    if (!m_pVideoCodecCtx) {
        LOGD("video avcodec_alloc_context3 failed!");
        return false;
    }

    //设置编码相关参数
    m_pVideoCodecCtx->width = m_videoOutWidth;
    m_pVideoCodecCtx->height = m_videoOutHeight;
    
    //时间基
    m_pVideoCodecCtx->time_base = (AVRational){1, 1000};

    //画面组大小，多少帧一个关键帧
    m_pVideoCodecCtx->gop_size = 20;

    m_pVideoCodecCtx->max_b_frames = 0;
    m_pVideoCodecCtx->pix_fmt = AV_PIX_FMT_YUV420P;
    m_pVideoCodecCtx->codec_id = AV_CODEC_ID_H264;
    av_opt_set(m_pVideoCodecCtx->priv_data,"preset","superfast",0);

    m_pVideoCodecCtx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
    
    //打开编码器
    int ret = avcodec_open2(m_pVideoCodecCtx, codec, NULL);
    if (ret != 0) {
        avcodec_free_context(&m_pVideoCodecCtx);
        LOGD("video avcodec_open2 failed!");
        return false;
    }

    m_pVideoStream = avformat_new_stream(m_pFormatCtx,NULL);
    if(!m_pVideoStream){
        LOGD("avformat_new_stream failed!");
        return false;
    }

    //流内部不包含编码器信息
    m_pVideoStream->codecpar->codec_tag = 0;
    avcodec_parameters_from_context(m_pVideoStream->codecpar,m_pVideoCodecCtx);

    if (!m_pYUVFrame) {
        m_pYUVFrame = av_frame_alloc();
        m_pYUVFrame->format = AV_PIX_FMT_YUV420P;
        m_pYUVFrame->width = m_videoOutWidth;
        m_pYUVFrame->height = m_videoOutHeight;
        m_pYUVFrame->pts =0;
        int ret = av_frame_get_buffer(m_pYUVFrame,0);
        if(ret != 0){
            LOGD("aav_frame_get_buffer failed!");
            return false;
        }
    }

    return true;

}

bool LCVideoWriter::WriteVideoFrameWithRgbData(const unsigned char* rgb)
{
    if (!m_bRecording) {
        return false;
    }

    if (!m_pFormatCtx || !m_pYUVFrame || !rgb) {
        return false;
    }

    int t_width = m_videoInWidth;
    int t_height = m_videoInHeight;

    int rgbStride = m_videoInWidth * 4;
    int whSize = t_width * t_height;
    int uv_stride = t_width / 2;
    int uv_length = uv_stride * (t_height /2);

    uint8_t* yuvBuffer = (uint8_t*)malloc(whSize*3/2);

    uint8_t* Y_data_Ptr = yuvBuffer;
    uint8_t* U_data_ptr = yuvBuffer + whSize;
    uint8_t* V_data_ptr =  yuvBuffer + whSize+ uv_length;

    libyuv::BGRAToI420((uint8_t*)rgb, rgbStride,
                       Y_data_Ptr, t_width,
                       U_data_ptr, uv_stride,
                       V_data_ptr, uv_stride,
                       t_width, t_height);

    m_pYUVFrame->data[0] = Y_data_Ptr;
    m_pYUVFrame->data[1] = U_data_ptr;
    m_pYUVFrame->data[2] = V_data_ptr;

    m_pYUVFrame->linesize[0] = t_width;
    m_pYUVFrame->linesize[1] = uv_stride;
    m_pYUVFrame->linesize[2] = uv_stride;

    unsigned long currentPts = GetTickCount() - m_startTimeStamp;
    if (currentPts <= 0) {
        currentPts += 1;
    }

    m_pYUVFrame->pts = currentPts;

    int ret = avcodec_send_frame(m_pVideoCodecCtx, m_pYUVFrame);
    if (ret != 0) {
        if (yuvBuffer != NULL) {
            free(yuvBuffer);
            yuvBuffer = NULL;
        }
        return false;
    }
    if(!m_bRecording || !m_pFormatCtx || !m_pYUVFrame){
        return false;
    }

    AVPacket packet;
    av_init_packet(&packet);

    if(!m_bRecording || !m_pFormatCtx || !m_pYUVFrame){
        return false;
    }

    ret = avcodec_receive_packet(m_pVideoCodecCtx, &packet);
    if (ret != 0 || packet.size <= 0) {
        if (yuvBuffer != NULL) {
            free(yuvBuffer);
            yuvBuffer = NULL;
        }
        return false;
    }

    if (yuvBuffer != NULL) {
        free(yuvBuffer);
        yuvBuffer = NULL;
    }

    //用于将 AVPacket 中各种时间值从一种时间基转换为另一种时间基
    av_packet_rescale_ts(&packet, m_pVideoCodecCtx->time_base, m_pVideoStream->time_base);
    packet.stream_index = m_pVideoStream->index;

    writeFrame(&packet);

    return true;
}

bool LCVideoWriter::writeVideoHeader() 
{
    if (!m_pFormatCtx) {
        return false;
    }
    
    //打开io
    int ret = avio_open(&m_pFormatCtx->pb, m_filePath.c_str(), AVIO_FLAG_WRITE);
    if (ret != 0) {
        LOGD("avio_open failed! %s" ,m_filePath.c_str());
        return false;
    }

    //写入封装头
    ret = avformat_write_header(m_pFormatCtx, NULL);
    if (ret != 0)
    {
        LOGD("avformat_write_header failed!" );
        return false;
    }
    LOGD("Write %s   success!" ,m_filePath.c_str() );

    return true;
}

bool LCVideoWriter::writeFrame(AVPacket *pkt) 
{
    if(!m_bRecording){
        return false;
    }

    if (!m_pFormatCtx || !pkt || pkt->size <= 0){
        return false;
    }

    if(pkt == NULL){
        return false;
    }

    if(pkt->data ==NULL){
        return false;
    }
    
    pthread_mutex_lock(&m_videoWriteMutex);
    int retValue=av_interleaved_write_frame(m_pFormatCtx, pkt);
    pthread_mutex_unlock(&m_videoWriteMutex);

    //av_write_frame
    if (retValue != 0) {
        return false;
    }

    return true;
}

bool LCVideoWriter::writeMp4FileTrail() 
{
    if (!m_pFormatCtx || !m_pFormatCtx->pb) return false;

    //写入尾部信息索引
    if (av_write_trailer(m_pFormatCtx) != 0)
    {
        LOGD("av_write_trailer failed!");
        return false;
    }

    //关闭IO
    if (avio_closep(&m_pFormatCtx->pb) != 0)
    {
        LOGD("avio_close failed!" );
        return false;
    }

    LOGD("WriteEnd success!" );

    return true;
}

bool LCVideoWriter::addAudioStream()
{
    if (!m_pFormatCtx) return false;
    
    //1.找到音频编码
    AVCodec *codec = avcodec_find_encoder(AV_CODEC_ID_AAC);
    if (!codec)
    {
        LOGD("avcodec_find_encoder AV_CODEC_ID_AAC failed!");
        return false;
    }

    //2 创建并打开音频编码器
    m_pAudioCodecCtx = avcodec_alloc_context3(codec);
    if (!m_pAudioCodecCtx)
    {
        LOGD("avcodec_alloc_context3 failed!" );
        return false;
    }

    m_pAudioCodecCtx->sample_rate = m_audioOutSamplerate;
    m_pAudioCodecCtx->sample_fmt = AV_SAMPLE_FMT_FLTP;
    m_pAudioCodecCtx->channels = m_audioOutChannels;
    m_pAudioCodecCtx->channel_layout = av_get_default_channel_layout(m_audioOutChannels);
    m_pAudioCodecCtx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

    int ret = avcodec_open2(m_pAudioCodecCtx, codec, NULL);
    if (ret != 0)
    {
        avcodec_free_context(&m_pAudioCodecCtx);
        LOGD("audio avcodec_open2 failed!" );
        return false;
    }

    //3 新增音频流
    audioStream = avformat_new_stream(m_pFormatCtx, NULL);
    if (!audioStream)
    {
        LOGD("avformat_new_stream failed" );
        return false;
    }

    audioStream->codecpar->codec_tag = 0;
    avcodec_parameters_from_context(audioStream->codecpar, m_pAudioCodecCtx);

//    av_dump_format(m_pFormatCtx, 0, m_filePath.c_str(), 1);

    return true;
}

void LCVideoWriter::releaseAccConfiguration()
{
    if(g_aacEncodeConfig == NULL)
    {
        return;
    }

    pthread_mutex_lock(&aacWriterMutex);

    if(g_aacEncodeConfig->hEncoder!= NULL)
    {
        faacEncClose(g_aacEncodeConfig->hEncoder);
        g_aacEncodeConfig->hEncoder = NULL;
    }

    if(g_aacEncodeConfig->pcmBuffer!= NULL)
    {
        free(g_aacEncodeConfig->pcmBuffer);
        g_aacEncodeConfig->pcmBuffer = NULL;
    }

    if(g_aacEncodeConfig->aacBuffer!= NULL)
    {
        free(g_aacEncodeConfig->aacBuffer);
        g_aacEncodeConfig->aacBuffer = NULL;
    }
    if(g_aacEncodeConfig == NULL)
    {
        free(g_aacEncodeConfig);
        g_aacEncodeConfig = NULL;
    }
    pthread_mutex_unlock(&aacWriterMutex);
}

bool LCVideoWriter::WriteAudioFrameWithPcmData(unsigned char *data, int size)
{
    if(!m_bRecording){
        return false;
    }

    linearPCM2AAC(data,size);

    return true;
}

AACEncodeConfig * LCVideoWriter::initAudioEncodeConfiguration()
{
    AACEncodeConfig* aacConfig = NULL;

    faacEncConfigurationPtr pConfiguration;

    int nRet = 0;
    m_pcmBufferSize = 0;

    aacConfig = (AACEncodeConfig*)malloc(sizeof(AACEncodeConfig));
    aacConfig->nSampleRate = m_audioOutSamplerate;
    aacConfig->nChannels = 1;
    aacConfig->nPCMBitSize = 16;
    aacConfig->nInputSamples = 0;
    aacConfig->nMaxOutputBytes = 0;

    aacConfig->hEncoder = faacEncOpen(aacConfig->nSampleRate, aacConfig->nChannels,  (unsigned long *)&aacConfig->nInputSamples, (unsigned long *)&aacConfig->nMaxOutputBytes);
    if(aacConfig->hEncoder == NULL)
    {
        LOGD("failed to call faacEncOpen()");
        return NULL;
    }

    m_pcmBufferSize = (int)(aacConfig->nInputSamples*(aacConfig->nPCMBitSize/8));
    m_pcmBufferRemainSize=m_pcmBufferSize;

    aacConfig->pcmBuffer=(unsigned char*)malloc(m_pcmBufferSize*sizeof(unsigned char));
    memset(aacConfig->pcmBuffer, 0, m_pcmBufferSize);

    aacConfig->aacBuffer=(unsigned char*)malloc(aacConfig->nMaxOutputBytes*sizeof(unsigned char));
    memset(aacConfig->aacBuffer, 0, aacConfig->nMaxOutputBytes);


    pConfiguration = faacEncGetCurrentConfiguration(aacConfig->hEncoder);

    pConfiguration->inputFormat = FAAC_INPUT_16BIT;
    pConfiguration->outputFormat = 0;
    pConfiguration->aacObjectType = LOW;

    nRet = faacEncSetConfiguration(aacConfig->hEncoder, pConfiguration);

    return aacConfig;
}

int LCVideoWriter::linearPCM2AAC(unsigned char *pData, int captureSize)
{
    if(!m_bRecording){
        return -1;
    }

    if(pData==NULL){
        return -1;
    }

    if((captureSize>m_pcmBufferSize)||(captureSize<=0)){
        return -1;
    }

    int nRet = 0;
    int copyLength = 0;

    if(m_pcmBufferRemainSize > captureSize){
        copyLength = captureSize;
    }
    else{
        copyLength = m_pcmBufferRemainSize;
    }

    memcpy((&g_aacEncodeConfig->pcmBuffer[0]) + m_pcmWriteRemainSize, pData, copyLength);
    m_pcmBufferRemainSize -= copyLength;
    m_pcmWriteRemainSize += copyLength;

    if(m_pcmBufferRemainSize > 0){
        return 0;
    }

    pthread_mutex_lock(&aacWriterMutex);
    nRet = faacEncEncode(g_aacEncodeConfig->hEncoder,(int*)(g_aacEncodeConfig->pcmBuffer),g_aacEncodeConfig->nInputSamples,g_aacEncodeConfig->aacBuffer,g_aacEncodeConfig->nMaxOutputBytes);
    pthread_mutex_unlock(&aacWriterMutex);

    memset(g_aacEncodeConfig->pcmBuffer, 0, m_pcmBufferSize);
    m_pcmWriteRemainSize = 0;
    m_pcmBufferRemainSize = m_pcmBufferSize;

    AVPacket pkt;
    av_init_packet(&pkt);

    pkt.stream_index = audioStream->index;//音频流的索引
    pkt.data=g_aacEncodeConfig->aacBuffer;
    pkt.size=nRet;
    pkt.pts = m_audioFramePts;
    pkt.dts = pkt.pts;
    AVRational rat=(AVRational){1,m_pAudioCodecCtx->sample_rate};

    m_audioFramePts += av_rescale_q(m_samples, rat, m_pAudioCodecCtx->time_base);

    writeFrame(&pkt);

    memset(g_aacEncodeConfig->pcmBuffer, 0, m_pcmBufferSize);
    if((captureSize - copyLength) > 0 ){
        memcpy((&g_aacEncodeConfig->pcmBuffer[0]), pData+copyLength, captureSize - copyLength);
        m_pcmWriteRemainSize = captureSize - copyLength;
        m_pcmBufferRemainSize = m_pcmBufferSize - (captureSize - copyLength);
    }

    return nRet;
}

unsigned long LCVideoWriter::GetTickCount()
{

    struct timeval tv;

    if (gettimeofday(&tv, NULL) != 0)
        return 0;

    return (tv.tv_sec*1000  + tv.tv_usec / 1000);
}



