package com.lc.fve;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.appcompat.app.AppCompatActivity;

import android.Manifest;
import android.content.Intent;
import android.graphics.Bitmap;
import android.net.Uri;
import android.os.Bundle;
import android.os.Environment;
import android.util.Log;
import android.view.View;
import android.widget.TextView;
import android.widget.Toast;

import com.lc.fve.databinding.ActivityMainBinding;
import com.lc.fve.utils.AssetUtils;
import com.lc.fve.utils.BitmapUtils;
import com.permissionx.guolindev.PermissionX;
import com.permissionx.guolindev.callback.RequestCallback;

import java.io.File;
import java.io.IOException;
import java.util.List;

public class MainActivity extends AppCompatActivity {
    private static final String TAG = "MainActivity";

    private static final int REQUEST_CHOOSE_IMAGE1 = 1001;

    private ActivityMainBinding binding;

    private String resourceImageDir;
    private String resourceVideoDir;

    File resultVideo;
    File mergeVideo;
    File addMusicFile;
    File scaleResultFile;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        binding = ActivityMainBinding.inflate(getLayoutInflater());
        setContentView(binding.getRoot());

        checkPermission();

        resourceImageDir = this.getExternalFilesDir(Environment.DIRECTORY_PICTURES).getAbsolutePath();
        resourceVideoDir = this.getExternalFilesDir(Environment.DIRECTORY_MOVIES).getAbsolutePath();

        Log.i(TAG, "onCreate: resourceImageDir = " + resourceImageDir);
        Log.i(TAG, "onCreate: resourceVideoDir = " + resourceVideoDir);

        resultVideo = new File(resourceVideoDir, "jpg_result.mp4");
        mergeVideo = new File(resourceVideoDir, "merge_result.mp4");
        addMusicFile = new File(resourceVideoDir, "add_result_music.mp4");
        scaleResultFile = new File(resourceVideoDir, "result_scale.mp4");

        FFmpegNative fmpegNative = new FFmpegNative();

        // Example of a call to a native method
        TextView tv = binding.sampleText;
        tv.setText(fmpegNative.stringFromJNI());

        binding.selectImage.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                startChooseImageIntentForResult(REQUEST_CHOOSE_IMAGE1);
            }
        });

        binding.startJpgToVideo.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                String srcPath = resourceImageDir + File.separator + "name%3d.jpg";
                new Thread(new Runnable() {
                    @Override
                    public void run() {
                        fmpegNative.doJpgToVideo(srcPath, resultVideo.getAbsolutePath());
                    }
                }).start();
            }
        });

        binding.startMergeVideo.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                new Thread(new Runnable() {
                    @Override
                    public void run() {
//                        String srcPath1 = resourceVideoDir + File.separator + "jpg_result.mp4";
                        String srcPath1 = resourceVideoDir + File.separator + "test_1280x720_2.mp4";
                        String srcPath2 = resourceVideoDir + File.separator + "VID_20211028_111908.mp4";

                        String[] stringArr = {srcPath1, srcPath2};
                        fmpegNative.mergeFiles(stringArr, mergeVideo.getAbsolutePath(), 0, 0, 0, 0, 0);
                    }
                }).start();

            }
        });

        binding.startAddMusic.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                new Thread(new Runnable() {
                    @Override
                    public void run() {
                        String videoSrcPath = resourceVideoDir + File.separator + "test_1280x720_4.mp4";
//                        String audioSrcPath = resourceVideoDir + File.separator + "test_441_f32le_2.aac";
                        String audioSrcPath = resourceVideoDir + File.separator + "name_20211029-162031.mp4";


                        fmpegNative.addMusic(videoSrcPath, audioSrcPath, addMusicFile.getAbsolutePath(), "00:00:00");
                    }
                }).start();

            }
        });

        binding.startScaleVideo.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                new Thread(new Runnable() {
                    @Override
                    public void run() {
                        String srcPath1 = resourceVideoDir + File.separator + "VID_20211028_111908.mp4";

                        fmpegNative.doVideoScale(srcPath1, scaleResultFile.getAbsolutePath());
                    }
                }).start();

            }
        });

    }

    private void startChooseImageIntentForResult(int requestCode) {
        Intent intent = new Intent();
        intent.setType("image/*");
        intent.setAction(Intent.ACTION_GET_CONTENT);
        startActivityForResult(Intent.createChooser(intent, "Select Picture"), requestCode);
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, @Nullable Intent data) {
        if (requestCode == REQUEST_CHOOSE_IMAGE1 && resultCode == RESULT_OK) {
            File file = new File(this.getExternalFilesDir(Environment.DIRECTORY_PICTURES), "name001.jpg");
            Uri data1 = data.getData();
            new Thread(new Runnable() {
                @Override
                public void run() {
                    tryReloadImage(data1, file);
                }
            }).start();


        } else {
            super.onActivityResult(requestCode, resultCode, data);
        }

    }

    private void tryReloadImage(Uri imageUri, File file) {
        try {
            if (imageUri == null) return;

            Bitmap imageBitmap = BitmapUtils.getBitmapFromContentUri(getContentResolver(), imageUri);

            if (imageBitmap == null) {
                return;
            }
//            Bitmap resizedBitmap =
//                    Bitmap.createScaledBitmap(
//                            imageBitmap,
//                            480,
//                            640,
//                            true);
//            imageView.setImageBitmap(resizedBitmap);

            //保存bitmap到私有目录
            if (file.exists()) {
                file.delete();
            }
            AssetUtils.copyBitmapToPhone(imageBitmap, file);

        } catch (IOException e) {
            e.printStackTrace();
        }
    }

    private void checkPermission() {
        PermissionX.init(MainActivity.this)
                .permissions(Manifest.permission.WRITE_EXTERNAL_STORAGE,
                        Manifest.permission.READ_EXTERNAL_STORAGE)
                .request(new RequestCallback() {
                    @Override
                    public void onResult(boolean allGranted, @NonNull List<String> grantedList, @NonNull List<String> deniedList) {
                        if (allGranted) {
                            Toast.makeText(MainActivity.this, "权限同意了", Toast.LENGTH_SHORT).show();
                            //在子线程进行IO操作
                        } else {
                            Toast.makeText(MainActivity.this, "权限拒绝", Toast.LENGTH_SHORT).show();
                        }
                    }
                });
    }
}