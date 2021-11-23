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

    private native void startPlayWithFile(String fileName);
    private native int getVideoWidth();
    private native int getVideoHeight();
    private native int getVideoRotateAngle();
    private native float getVideoTotalSeconds();

    /**
     * 合并图片
     * @param srcPath 图片地址
     * @param dstPath 目标地址
     */
    public native void doJpgToVideo(String srcPath, String dstPath);


    /**
     *
     * @param width 目标宽
     * @param height 目标高
     * @param videoBitrate 比特率
     * @param fps 帧率
     *  videoBitrate默认为512000 fps默认为25
     */
    public native void setEncodeParams(int width, int height, int videoBitrate, int fps);


    /**
     * 合并视频文件
     * @param srcPathList 视频文件路径
     * @param dst 目标路径
     */
    public native void mergeFiles(String[] srcPathList, String dst);

    /**
     * 获取合并处理状态
     * @return -1表示处理出错 0表示开始处理  1表示处理完成
     */
    public native int getMergeStatus();


    /**
     *添加音乐
     * @param videoSrc 视频路径
     * @param audioSrc 音频路径
     * @param dstPath 目标路径
     * @param startTime 开始时间 "00:00:00"
     */
    public native void addMusic(String videoSrc, String audioSrc, String dstPath, String startTime);

    /**
     * 去除视频声音
     * @param videoSrc 源路径
     * @param videoDst 目标路径
     */
    public native void deleteAudio(String videoSrc, String videoDst);

    public native void doVideoScale(String src,String dst);

    public void startVideoPlayerWithPath(String fileString)
    {
        startPlayWithFile(fileString);
    }

    public int getVideoWidthFormNdk() {
        return getVideoWidth();
    }

    public int getVideoHeightFormNdk() {
        return getVideoHeight();
    }

    public int getVideoRotateFormNdk() {
        return getVideoRotateAngle();
    }

    public int getVideoDurationFormNdk() {
        return (int)getVideoTotalSeconds();
    }
}
