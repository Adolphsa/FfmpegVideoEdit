//
// Created by Administrator on 2021/11/1.
//

#ifndef FFMPEGVIDEOEDIT_COMMONUTILS_H
#define FFMPEGVIDEOEDIT_COMMONUTILS_H



extern "C"
{
#include "common/CLog.h"
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>
#include <libavutil/imgutils.h>
}


/**
 * 获取旋转角度
 * @param avStream
 * @return
 */
static int getRotateAngle(AVStream *avStream) {

    AVDictionaryEntry *tag = NULL;

    int m_Rotate = -1;
    tag = av_dict_get(avStream->metadata, "rotate", tag, 0);

    if (tag == NULL) {
        m_Rotate = 0;
    } else {
        int angle = atoi(tag->value);
        angle %= 360;
        if (angle == 90) {
            m_Rotate = 90;
        } else if (angle == 180) {
            m_Rotate = 180;
        } else if (angle == 270) {
            m_Rotate = 270;
        } else {
            m_Rotate = 0;
        }
    }

    return m_Rotate;
}

/**
 * @return 返回以毫秒为单位的当前时间
 */
static unsigned long getTickCount() {
    struct timeval tv;
    if (gettimeofday(&tv, NULL) != 0)
        return 0;
    return (tv.tv_sec*1000 + tv.tv_usec / 1000);
}

#endif //FFMPEGVIDEOEDIT_COMMONUTILS_H
