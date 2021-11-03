//
// Created by Administrator on 2021/11/2.
//

#ifndef FFMPEGVIDEOEDIT_YUVUTILS_H
#define FFMPEGVIDEOEDIT_YUVUTILS_H

#include <libyuv.h>

static void
rotateI420(uint8 *src_i420_data, int width, int height, uint8 *dst_i420_data, int degree) {

    int src_i420_y_size = width * height;
    int src_i420_u_size = (width >> 1) * (height >> 1);

    uint8 *src_i420_y_data = src_i420_data;
    uint8 *src_i420_u_data = src_i420_data + src_i420_y_size;
    uint8 *src_i420_v_data = src_i420_data + src_i420_y_size + src_i420_u_size;

    uint8 *dst_i420_y_data = dst_i420_data;
    uint8 *dst_i420_u_data = dst_i420_data + src_i420_y_size;
    uint8 *dst_i420_v_data = dst_i420_data + src_i420_y_size + src_i420_u_size;

    //要注意这里的width和height在旋转之后是相反的
    if (degree == libyuv::kRotate90 || degree == libyuv::kRotate270) {
        libyuv::I420Rotate((const uint8 *) src_i420_y_data, width,
                           (const uint8 *) src_i420_u_data, width >> 1,
                           (const uint8 *) src_i420_v_data, width >> 1,
                           (uint8 *) dst_i420_y_data, height,
                           (uint8 *) dst_i420_u_data, height >> 1,
                           (uint8 *) dst_i420_v_data, height >> 1,
                           width, height,
                           (libyuv::RotationMode) degree);
    } else {
        libyuv::I420Rotate((const uint8 *) src_i420_y_data, width,
                           (const uint8 *) src_i420_u_data, width >> 1,
                           (const uint8 *) src_i420_v_data, width >> 1,
                           (uint8 *) dst_i420_y_data, width,
                           (uint8 *) dst_i420_u_data, width >> 1,
                           (uint8 *) dst_i420_v_data, width >> 1,
                           width, height,
                           (libyuv::RotationMode) degree);
    }
}

static void
scaleI420(uint8 *src_i420_data, int width, int height,
          uint8 *dst_i420_data, int dst_width, int dst_height,
          int mode) {

    int src_i420_y_size = width * height;
    int src_i420_u_size = (width >> 1) * (height >> 1);
    uint8 *src_i420_y_data = src_i420_data;
    uint8 *src_i420_u_data = src_i420_data + src_i420_y_size;
    uint8 *src_i420_v_data = src_i420_data + src_i420_y_size + src_i420_u_size;

    int dst_i420_y_size = dst_width * dst_height;
    int dst_i420_u_size = (dst_width >> 1) * (dst_height >> 1);
    uint8 *dst_i420_y_data = dst_i420_data;
    uint8 *dst_i420_u_data = dst_i420_data + dst_i420_y_size;
    uint8 *dst_i420_v_data = dst_i420_data + dst_i420_y_size + dst_i420_u_size;

    libyuv::I420Scale((const uint8 *) src_i420_y_data, width,
                      (const uint8 *) src_i420_u_data, width >> 1,
                      (const uint8 *) src_i420_v_data, width >> 1,
                      width, height,
                      (uint8 *) dst_i420_y_data, dst_width,
                      (uint8 *) dst_i420_u_data, dst_width >> 1,
                      (uint8 *) dst_i420_v_data, dst_width >> 1,
                      dst_width, dst_height,
                      (libyuv::FilterMode) mode);

}

class YuvUtils {

};


#endif //FFMPEGVIDEOEDIT_YUVUTILS_H
