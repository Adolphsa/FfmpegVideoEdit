package com.lc.fve;

/**
 * Created by lucas on 2021/10/21.
 */
public class FFmpegNative {

    // Used to load the 'fve' library on application startup.
    static {
        System.loadLibrary("fve");
    }

    /**
     * A native method that is implemented by the 'fve' native library,
     * which is packaged with this application.
     */
    public native String stringFromJNI();

    /**
     * 合并图片
     * @param srcPath 图片地址
     * @param dstPath 目标地址
     */
    public native void doJpgToVideo(String srcPath, String dstPath);


    /**
     * 合并视频文件
     * @param srcPathList 视频文件路径
     * @param dst 目标路径
     * @param width 目标宽
     * @param height 目标高
     * @param videoBitrate 比特率
     * @param fps 帧率
     * @param isSetEncoderParam 是否设置编码参数 videoBitrate默认为512000 fps默认为25
     */
    public native void mergeFiles(String[] srcPathList, String dst, int width, int height, int videoBitrate, int fps, int isSetEncoderParam);

    /**
     *添加音乐
     * @param videoSrc 视频路径
     * @param audioSrc 音频路径
     * @param dstPath 目标路径
     * @param startTime 开始时间 "00:00:00"
     */
    public native void addMusic(String videoSrc, String audioSrc, String dstPath, String startTime);

    public native void doVideoScale(String src,String dst);
}
