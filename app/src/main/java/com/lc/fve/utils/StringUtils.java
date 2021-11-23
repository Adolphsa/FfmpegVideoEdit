package com.lc.fve.utils;

import android.media.MediaMetadataRetriever;
import android.util.Log;
import android.util.Size;

import androidx.exifinterface.media.ExifInterface;

import java.io.IOException;

/**
 * 字符工具
 * Created by lucas on 2021/11/11.
 */
public class StringUtils {
    private static final String TAG = "StringUtils";

    public static boolean isMp4(String str) {
        int index = str.lastIndexOf(".");
        String fileType = str.substring(index + 1);
        Log.d(TAG, "isMp4: fileType = " + fileType);
        return fileType.equalsIgnoreCase("mp4");
    }

    public static boolean isJpg(String str) {
        int index = str.lastIndexOf(".");
        String fileType = str.substring(index + 1);
        Log.d(TAG, "isJpg: fileType = " + fileType);
        return (fileType.equalsIgnoreCase("jpg") || fileType.equalsIgnoreCase("jpeg"));
    }

    public static String getJpgPrePath(String srcPath) {
        int index = srcPath.lastIndexOf(".");
        StringBuilder resultPath = new StringBuilder();
        resultPath.append(srcPath.substring(0, index));
        resultPath.append("_pre");
        resultPath.append(srcPath.substring(index));
        return resultPath.toString();
    }

    public static String getJpgConvertResultPath(String srcPath) {
        int index = srcPath.lastIndexOf(".");
        StringBuilder resultPath = new StringBuilder();
        resultPath.append(srcPath.substring(0, index));
        resultPath.append(".mp4");
        return resultPath.toString();
    }

    public static String getMp4ConvertResultPath(String srcPath) {
        int index = srcPath.lastIndexOf(".");
        StringBuilder resultPath = new StringBuilder();
        resultPath.append(srcPath.substring(0, index));
        resultPath.append("_pre");
        resultPath.append(".mp4");
        return resultPath.toString();
    }

    public static int getImageRotate(String srcPath) {
        // 图片旋转角度，该角度表示在正常图片的基础上逆时针旋转的角度
        int degree = 0;
        ExifInterface exifInterface = null;
        try {
            exifInterface = new ExifInterface(srcPath);
            int  orientation = exifInterface.getAttributeInt(ExifInterface.TAG_ORIENTATION, ExifInterface.ORIENTATION_NORMAL);
            switch (orientation) {
                case ExifInterface.ORIENTATION_ROTATE_90:
                    degree = 90;
                    break;
                case ExifInterface.ORIENTATION_ROTATE_180:
                    degree = 180;
                    break;
                case ExifInterface.ORIENTATION_ROTATE_270:
                    degree = 270;
                    break;
            }
        } catch (IOException e) {
            e.printStackTrace();
        }

        return degree;
    }

    public static int getFfmpegRotateParam(int degree) {
        int rotateParam = -1;
        if (degree == 90) {
            rotateParam = 1;
        } else if (degree == 270) {
            rotateParam = 2;
        }

        return rotateParam;
    }

    public static Size getImageSize(String srcPath) {
        int width = 0;
        int height = 0;
        ExifInterface exifInterface = null;

        try {
            exifInterface = new ExifInterface(srcPath);
            width = exifInterface.getAttributeInt(ExifInterface.TAG_IMAGE_WIDTH, 0);
            height = exifInterface.getAttributeInt(ExifInterface.TAG_IMAGE_LENGTH, 0);
        } catch (IOException e) {
            e.printStackTrace();
        }

        return new Size(width, height);
    }

    /**
     * get Local video and audio duration
     *
     * @return
     */
    public static int getLocalVideoDuration(String videoPath) {
        //时长(毫秒)
        int duration;
        try {
            MediaMetadataRetriever mmr = new  MediaMetadataRetriever();
            mmr.setDataSource(videoPath);
            duration = Integer.parseInt(mmr.extractMetadata
                    (MediaMetadataRetriever.METADATA_KEY_DURATION))/1000;
        } catch (Exception e) {
            e.printStackTrace();
            return 0;
        }
        return duration;
    }

    /**
     * get Local video rotate
     *
     * @return
     */
    public static int getLocalVideoRotate(String videoPath) {
        //角度
        int rotate;
        try {
            MediaMetadataRetriever mmr = new  MediaMetadataRetriever();
            mmr.setDataSource(videoPath);
            rotate = Integer.parseInt(mmr.extractMetadata(MediaMetadataRetriever.METADATA_KEY_VIDEO_ROTATION));
        } catch (Exception e) {
            e.printStackTrace();
            return 0;
        }
        return rotate;
    }
}
