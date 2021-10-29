package com.lc.fve.utils;

import android.graphics.Bitmap;
import android.util.Log;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;

/**
 * Asset工具类
 * Created by lucas on 2021/10/22.
 */
public class AssetUtils {
    private static final String TAG = "AssetUtils";

    public static void copyBitmapToPhone(Bitmap bitmap, File file) {
        try {
            FileOutputStream fos = new FileOutputStream(file);
            bitmap.compress(Bitmap.CompressFormat.JPEG, 100, fos);//将图片复制到私有空间
            fos.flush();
            Log.i(TAG, "copyImageUriToPhone: 复制完成");
        } catch (IOException e) {
            e.printStackTrace();
        }
    }
}
