package com.lc.fve;

import android.util.Log;

import com.arthenica.mobileffmpeg.FFmpeg;
import com.arthenica.mobileffmpeg.FFprobe;
import com.arthenica.mobileffmpeg.MediaInformation;
import com.lc.fve.utils.StringUtils;

import org.json.JSONObject;

import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.OutputStreamWriter;
import java.util.ArrayList;
import java.util.List;

/**
 * Created by lucas on 2021/11/8.
 */
public class FFmpegCmd {
    private static final String TAG = "FFmpegCmd";
    private FFmpegCmd() {}

    private static class SingletonInstance {
        private static final FFmpegCmd INSTANCE = new FFmpegCmd();
    }

    public static FFmpegCmd getInstance() {
        return SingletonInstance.INSTANCE;
    }

    /**
     * 旋转缩放图片
     * @param srcPath 源图片
     * @param dstPath 目标图片
     * @param rotate 角度
     * @return
     */
    public int scaleRotateJpg(String srcPath, String dstPath, int rotate) {
        //ffmpeg -i lc.jpg -vf scale=720:1280 -vf transpose=2 lc3.jpg
        StringBuilder cmdString = new StringBuilder();
        cmdString.append("-i");
        cmdString.append(" ");
        cmdString.append(srcPath);
        cmdString.append(" -vf scale=720:1280");
        cmdString.append(" -vf transpose=");
        cmdString.append(rotate);
        cmdString.append(" ");
        cmdString.append(dstPath);
        Log.d(TAG, "scaleRotateJpg: " + cmdString.toString());
        return FFmpeg.execute(cmdString.toString());
    }

    /**
     * jpg转mp4 时长默认3s fps=1 width=720 height=1280
     * @param srcPath jpg图片路径
     * @param dstPath 生成的MP4路径
     * @param rotate 需要旋转的角度 如果不需要旋转  传入-1
     *               0:逆时针旋转90度并垂直翻转
     *              1:顺时针旋转90度
     *              2:逆时针旋转90度
     *              3:顺时针旋转90度后并垂直翻转
     * @return
     */
    public int jpgToVideo(String srcPath, String dstPath, int rotate){
        //ffmpeg -y -f lavfi -i anullsrc=channel_layout=stereo:sample_rate=44100 -loop 1 -i lc.jpg -pix_fmt yuv420p -c:v libx264 -r 25 -t 3 -s 720x1280 -vf transpose=0 out.mp4
        StringBuilder cmdString = new StringBuilder();
        cmdString.append("-y");
        cmdString.append(" -f");
        cmdString.append(" lavfi");
        cmdString.append(" -i");
        cmdString.append(" anullsrc=channel_layout=stereo:sample_rate=44100");
        cmdString.append(" -loop");
        cmdString.append(" 1");
        cmdString.append(" -i");
        cmdString.append(" ");
        cmdString.append(srcPath);
        cmdString.append(" -pix_fmt");
        cmdString.append(" yuv420p");
        cmdString.append(" -c:v libx264");
        cmdString.append(" -r");
        cmdString.append(" 0.3");
        cmdString.append(" -t");
        cmdString.append(" 3");
        cmdString.append(" -s");
        cmdString.append(" 720x1280");
        if (rotate != -1) {
            cmdString.append(" -vf transpose=");
            cmdString.append(rotate);
        }
        cmdString.append(" ");
        cmdString.append(dstPath);
        Log.d(TAG, "jpgToVideo: " + cmdString.toString());
        return FFmpeg.execute(cmdString.toString());
    }

    /**
     * 旋转缩放视频至720*1280
     * @param srcPath 源视频
     * @param dstPath 目标视频
     * @param rotate 旋转角度
     * @return
     */
    public int rotateScaleVideo(String srcPath, String dstPath, int rotate) {
        //ffmpeg -i test_rotate_90.mp4 -vf scale=720:1280 -acodec aac -vcodec libx264 -pix_fmt yuv420p -r 25 out3.mp4
        StringBuilder cmdString = new StringBuilder();
        cmdString.append("-i");
        cmdString.append(" ");
        cmdString.append(srcPath);
        cmdString.append(" -vf scale=720:1280");
        cmdString.append(" -acodec aac");
        cmdString.append(" -vcodec libx264");
        cmdString.append(" -pix_fmt yuv420p");
        cmdString.append(" -r 25");
        cmdString.append(" ");
        cmdString.append(dstPath);
        Log.d(TAG, "rotateScaleVideo: " + cmdString.toString());
        return FFmpeg.execute(cmdString.toString());
    }

    /**
     * 合并文件
     * @param pathList 待合并的路径
     * @param txtPath 路径文本
     * @param dstPath 目标路径
     * @return
     */
    public int mergeFiles(List<String> pathList, String txtPath, String dstPath) {
        //ffmpeg -i out2.mp4 -i out4.mp4 -filter_complex "[0:0] [0:1] [1:0] [1:1] concat=n=2:v=1:a=1 [v] [a]" -map "[v]" -map "[a]" out_merge.mp4
        //ffmpeg -f concat -i aa.txt -c copy out_merge2.mp4

        //如果目标文件存在就删掉
        File dstFile = new File(dstPath);
        if (dstFile.exists()) dstFile.delete();

        //首先预处理图片和视频  生成路径txt文件
        List<String> prePathList = preHandleFile(pathList, txtPath);

        StringBuilder cmdString = new StringBuilder();
        cmdString.append("-f");
        cmdString.append(" concat");
        cmdString.append(" -safe 0");
        cmdString.append(" -i");
        cmdString.append(" ");
        cmdString.append(txtPath);
        cmdString.append(" -c copy");
        cmdString.append(" ");
        cmdString.append(dstPath);
        Log.d(TAG, "mergeFiles: " + cmdString.toString());
        int ret = FFmpeg.execute(cmdString.toString());

        //合并完成后删除预处理文件
        for (String prePath : prePathList) {
            File f = new File(prePath);
            if (f.exists()) {
                f.delete();
            }
        }
        return ret;

    }

    /**
     * 裁剪视频 从0s开始 裁剪时长30s
     * @param srcPath 源路径
     * @param dstPath 目标路径
     * @return
     */
    public int cutVideo(String srcPath, String dstPath) {
        //ffmpeg -i input.mp4 -ss 00:00:00 -codec copy -t 30 output.mp4
        StringBuilder cmdString = new StringBuilder();
        cmdString.append("-i");
        cmdString.append(" ");
        cmdString.append(srcPath);
        cmdString.append(" -ss 00:00:00");
        cmdString.append(" -codec copy");
        cmdString.append(" -t 30");
        cmdString.append(" ");
        cmdString.append(dstPath);
        Log.d(TAG, "cutVideo: " + cmdString.toString());
        return FFmpeg.execute(cmdString.toString());
    }

    /**
     * 当图片分辨率小于目标分辨率时   为图片添加黑边
     * @param srcPath 源路径
     * @param dstPath 目标路径
     * ffmpeg -i text_640x480.jpg -vf pad=a:b:c:d:black hahaa.jpg
     * 其中a、b、c、d分别代表的参数是a为输出的宽度，b为输出的高度，c为需要左移的距离，d为序要下移的距离（单位默认为pixel）
     * @return
     */
    public int addBlackBorder(String srcPath, String dstPath, int srcW, int srcH, int dstW, int dstH) {
        //ffmpeg -i text_640x480.jpg -vf pad=720:1280:(720-640)/2:(1280-480)/2:black hahaa.jpg
        StringBuilder cmdString = new StringBuilder();
        cmdString.append("-i");
        cmdString.append(" ");
        cmdString.append(srcPath);
        cmdString.append(" -vf");
        cmdString.append(" pad=");
        cmdString.append(dstW);
        cmdString.append(":");
        cmdString.append(dstH);
        cmdString.append(":");
        cmdString.append((dstW-srcW)/2);
        cmdString.append(":");
        cmdString.append((dstH-srcH)/2);
        cmdString.append(":");
        cmdString.append("black");
        cmdString.append(" ");
        cmdString.append(dstPath);
        Log.d(TAG, "addBlackBorder: " + cmdString.toString());
        return FFmpeg.execute(cmdString.toString());
    }

    /**
     * 音频视频合成
     * @param videoPath 视频路径
     * @param audioPath 音频路径
     * @param mergePath 合并之后的路径
     * @return
     */
    public int audioVideoMerge(String videoPath, String audioPath, String mergePath) {
        //ffmpeg -i input1.mp4  -i input2_AAC.mp4 -vcodec copy -acodec copy output2.mp4
        StringBuilder cmdString = new StringBuilder();
        cmdString.append("-i");
        cmdString.append(" ");
        cmdString.append(videoPath);
        cmdString.append(" -i");
        cmdString.append(" ");
        cmdString.append(audioPath);
        cmdString.append(" -vcodec copy");
        cmdString.append(" -acodec copy");
        cmdString.append(" ");
        cmdString.append(mergePath);
        Log.d(TAG, "audioVideoMerge: " + cmdString.toString());
        return FFmpeg.execute(cmdString.toString());
    }

    /**
     * 使一个视频中的音频静音，即只保留视频
     * @param srcPath 原视频路径
     * @param dstPath 目标视频路径
     * @return
     */
    public int deleteAudio(String srcPath, String dstPath){
        //ffmpeg -i input.mp4 -an -vcodec copy output.mp4
        StringBuilder cmdString = new StringBuilder();
        cmdString.append("-i");
        cmdString.append(" ");
        cmdString.append(srcPath);
        cmdString.append(" -an");
        cmdString.append(" -vcodec copy");
        cmdString.append(" ");
        cmdString.append(dstPath);
        Log.d(TAG, "deleteAudio: " + cmdString.toString());
        return FFmpeg.execute(cmdString.toString());
    }

    /**
     * 同步接口
     * 水平拼接视频
     * @param leftVideoPath 左边的视频地址
     * @param rightVideoPath 右边的视频地址
     * @param dstPath 目标视频地址
     * @return 0表示执行成功 255表示取消  其他表示失败
     */
    public int horizontalSplicingVideo(String leftVideoPath, String rightVideoPath, String dstPath)
    {
        //ffmpeg -i 1.mp4 -i 2.mp4 -filter_complex hstack -preset fast 3.mp4
        StringBuilder cmdString = new StringBuilder();
        cmdString.append("-i");
        cmdString.append(" ");
        cmdString.append(leftVideoPath);
        cmdString.append(" ");
        cmdString.append("-i");
        cmdString.append(" ");
        cmdString.append(rightVideoPath);
        cmdString.append(" ");
        cmdString.append("-filter_complex hstack -preset veryfast");
        cmdString.append(" ");
        cmdString.append(dstPath);
        Log.d(TAG, "horizontalSplicingVideo: " + cmdString.toString());
        return FFmpeg.execute(cmdString.toString());
    }

    /**
     * 预处理图片或视频
     * @param srcPathList 待处理的文件列表
     * @param txtPath 文本路径
     * @return
     */
    private List<String> preHandleFile(List<String> srcPathList, String txtPath) {
        FFmpegNative ffmpegNative = new FFmpegNative();
        List<String> handledPathList = new ArrayList<>();
        for (int i = 0; i < srcPathList.size(); i++) {
            String tempPath = srcPathList.get(i);
            if (StringUtils.isJpg(tempPath)) {
                //是JPG图片  先转换成3S的MP4文件
                //1.获取旋转角度
                int imageDegree = StringUtils.getImageRotate(tempPath);
                Log.d(TAG, "mergeFile: rotate = " + imageDegree);
                //2.获取旋转参数
                int rotateParam = StringUtils.getFfmpegRotateParam(imageDegree);
                //3.生成一个缩放旋转后的图片地址
                String jpgPrePath = StringUtils.getJpgPrePath(tempPath);
                File file1 = new File(jpgPrePath);
                if (file1.exists()) file1.delete();
                scaleRotateJpg(tempPath, jpgPrePath, rotateParam);
                //4.生成一个临时图片转视频的地址
                String jpgConvertResultPath = StringUtils.getJpgConvertResultPath(tempPath);
                File file2 = new File(jpgConvertResultPath);
                if (file2.exists()) file2.delete();
                jpgToVideo(jpgPrePath, jpgConvertResultPath, -1);
                //5.将处理好的路径放入list中
                handledPathList.add(jpgConvertResultPath);
                //6.删除临时缩放的文件
                File file3 = new File(jpgPrePath);
                if (file3.exists()) file3.delete();
            } else if (StringUtils.isMp4(tempPath)) {
                //是MP4视频  先进行旋转 缩放处理
                //获取视频角度  长宽信息进行重新编码
                //1.ffmpeg打开文件
                ffmpegNative.startVideoPlayerWithPath(tempPath);
                //2.获取视频旋转角度
                int videoRotate = ffmpegNative.getVideoRotateFormNdk();
                Log.d(TAG, "mergeFile: videoRotate = " + videoRotate);
                //3.生成ffmpeg旋转参数
                int rotateParam = StringUtils.getFfmpegRotateParam(videoRotate);
                //4.生成一个临时文件路径
                String mp4ConvertResultPath = StringUtils.getMp4ConvertResultPath(tempPath);
                File file = new File(mp4ConvertResultPath);
                if (file.exists()) file.delete();
                rotateScaleVideo(tempPath, mp4ConvertResultPath, -1);
                handledPathList.add(mp4ConvertResultPath);
            }
        }

        //生成文本路径
        createPathTxtFile(handledPathList, txtPath);

        return handledPathList;
    }

    /**
     * 生成文本路径
     * @param handledPathList 已经处理好的路径列表
     * @param txtPath 文本路径
     */
    private void createPathTxtFile(List<String> handledPathList, String txtPath) {
        File f = new File(txtPath);
        if (f.exists()) {
            f.delete();
        }

        try {
            FileOutputStream fout = new FileOutputStream(f);
            OutputStreamWriter outputStreamWriter = new OutputStreamWriter(fout);
            for (String s : handledPathList) {
                outputStreamWriter.write("file ");
                outputStreamWriter.write("'");
                outputStreamWriter.write(s);
                outputStreamWriter.write("'");
                outputStreamWriter.write("\n");
            }
            outputStreamWriter.close();
            fout.close();
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

}
