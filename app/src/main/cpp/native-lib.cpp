#include <jni.h>
#include <string>
#include <vector>

using namespace std;

extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libswscale/swscale.h>
#include "common/CLog.h"
}

#include "JpgVideo.h"
#include "Merge.h"
#include "FilterVideoScale.h"
#include "LCMediaInfo.h"

LCMediaInfo m_lcMediaInfo;
Merge mergeObj;

std::string jstring2string(JNIEnv *env, jstring jStr) {
    const char *cstr = env->GetStringUTFChars(jStr, nullptr);
    std::string str = std::string(cstr);
    env->ReleaseStringUTFChars(jStr, cstr);
    return str;
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_lc_fve_FFmpegNative_stringFromJNI(
        JNIEnv *env,
        jobject /* this */) {
    std::string hello = "Hello from C++";
    hello += avcodec_configuration();
    LOGD("test ffmpeg jni");

    AVCodec *codec = avcodec_find_encoder_by_name("libx264");
    if (!codec) {
        LOGD("not found libx264");
    } else {
        LOGD("yes found libx264");
    }

    AVCodec *codec1 = avcodec_find_encoder_by_name("libmp3lame");
    if (!codec1) {
        LOGD("not found libmp3lame");
    } else {
        LOGD("yes found libmp3lame");
    }

    AVCodec *codec2 = avcodec_find_encoder_by_name("libfdk_aac");
    if (!codec2) {
        LOGD("not found libfdk_aac");
    } else {
        LOGD("yes found libfdk_aac");
    }


    return env->NewStringUTF(hello.c_str());
}

extern "C"
JNIEXPORT void JNICALL
Java_com_lc_fve_FFmpegNative_doJpgToVideo(JNIEnv *env, jobject thiz, jstring src_path,
                                          jstring dst_path) {
    string srcPath = jstring2string(env, src_path);
    string dstPath = jstring2string(env, dst_path);
    LOGD("srcPath = %s", srcPath.c_str());
    LOGD("dstPath = %s", dstPath.c_str());
    JpgVideo jpgVideo;
    jpgVideo.doJpgToVideo(srcPath, dstPath);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_lc_fve_FFmpegNative_setEncodeParams(JNIEnv *env, jobject thiz, jint width, jint height,
                                             jint video_bitrate, jint fps) {
    mergeObj.setEncodeParam(width, height, video_bitrate, fps);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_lc_fve_FFmpegNative_mergeFiles(JNIEnv *env, jobject thiz, jobjectArray src_path_list,
                                        jstring dst) {
    vector<string> srcPathVector = vector<string>();
    srcPathVector.clear();
    jsize size = env->GetArrayLength(src_path_list);
    for (int i = 0; i < size; i++) {
        jstring jStr = (jstring) (env->GetObjectArrayElement(src_path_list, i));
        string tmpStr = jstring2string(env, jStr);
        LOGD("tmpStr = %s", tmpStr.c_str());
        srcPathVector.push_back(tmpStr);
    }
    string dstPath = jstring2string(env, dst);
    mergeObj.mergeFiles(srcPathVector, dstPath);
}

extern "C"
JNIEXPORT jint JNICALL
Java_com_lc_fve_FFmpegNative_getMergeStatus(JNIEnv *env, jobject thiz) {

    return (jint)mergeObj.getProcessStatus();
}

extern "C"
JNIEXPORT void JNICALL
Java_com_lc_fve_FFmpegNative_addMusic(JNIEnv *env, jobject thiz, jstring video_src,
                                      jstring audio_src, jstring dst_path, jstring start_time) {
    string vPath = jstring2string(env, video_src);
    string aPath = jstring2string(env, audio_src);
    string dpath = jstring2string(env, dst_path);
    string start = jstring2string(env, start_time);

    mergeObj.addMusic(vPath, aPath, dpath, start);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_lc_fve_FFmpegNative_deleteAudio(JNIEnv *env, jobject thiz, jstring video_src,
                                         jstring video_dst) {
    string srcPath = jstring2string(env, video_src);
    string dstPath = jstring2string(env, video_dst);
    mergeObj.deleteAudio(srcPath, dstPath);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_lc_fve_FFmpegNative_doVideoScale(JNIEnv *env, jobject thiz, jstring src, jstring dst) {

    string srcPath = jstring2string(env, src);
    string dstPath = jstring2string(env, dst);

    FilterVideoScale filterVideoScale;
    filterVideoScale.doVideoScale(srcPath, dstPath, 0, 0);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_lc_fve_FFmpegNative_startPlayWithFile(JNIEnv *env, jobject thiz, jstring file_name) {
    std::string fileString = env->GetStringUTFChars(file_name,0);
    LOGD("PATH STRING: %s",fileString.c_str());

    m_lcMediaInfo.StopPlayVideo();
    m_lcMediaInfo.SetVideoFilePath(fileString);
    m_lcMediaInfo.InitVideoCodec();
}
extern "C"
JNIEXPORT jint JNICALL
Java_com_lc_fve_FFmpegNative_getVideoWidth(JNIEnv *env, jobject thiz) {
    return m_lcMediaInfo.GetVideoWidth();
}
extern "C"
JNIEXPORT jint JNICALL
Java_com_lc_fve_FFmpegNative_getVideoHeight(JNIEnv *env, jobject thiz) {
    return m_lcMediaInfo.GetVideoHeight();
}
extern "C"
JNIEXPORT jint JNICALL
Java_com_lc_fve_FFmpegNative_getVideoRotateAngle(JNIEnv *env, jobject thiz) {
    return m_lcMediaInfo.GetRotateAngle();
}