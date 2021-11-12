package com.lc.fve;

import androidx.activity.result.ActivityResult;
import androidx.activity.result.ActivityResultCallback;
import androidx.activity.result.ActivityResultLauncher;
import androidx.activity.result.contract.ActivityResultContracts;
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
import android.util.Printer;
import android.util.Size;
import android.view.View;
import android.widget.TextView;
import android.widget.Toast;

import com.arthenica.mobileffmpeg.FFmpeg;
import com.lc.fve.databinding.ActivityMainBinding;
import com.lc.fve.utils.AssetUtils;
import com.lc.fve.utils.BitmapUtils;
import com.lc.fve.utils.GlideEngine;
import com.lc.fve.utils.StringUtils;
import com.luck.picture.lib.PictureSelector;
import com.luck.picture.lib.config.PictureConfig;
import com.luck.picture.lib.config.PictureMimeType;
import com.luck.picture.lib.entity.LocalMedia;
import com.permissionx.guolindev.PermissionX;
import com.permissionx.guolindev.callback.RequestCallback;

import java.io.File;
import java.io.IOException;
import java.util.ArrayList;
import java.util.List;

public class MainActivity extends AppCompatActivity {
    private static final String TAG = "MainActivity";

    private ActivityResultLauncher<Intent> launcherResult;

    private static final int REQUEST_CHOOSE_IMAGE1 = 1001;

    private ActivityMainBinding binding;

    private String resourceImageDir;
    private String resourceVideoDir;

    String[] stringArr;
    List<String> selectMediaList;

    File resultVideo;
    File mergeVideo;
    File txtFile;
    File addMusicFile;
    File scaleResultFile;
    File noAudioResultFile;
    File blackBorderFile;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        binding = ActivityMainBinding.inflate(getLayoutInflater());
        setContentView(binding.getRoot());

        // 注册需要写在onCreate或Fragment onAttach里，否则会报java.lang.IllegalStateException异常
        launcherResult = createActivityResultLauncher();

        checkPermission();

        resourceImageDir = this.getExternalFilesDir(Environment.DIRECTORY_PICTURES).getAbsolutePath();
        resourceVideoDir = this.getExternalFilesDir(Environment.DIRECTORY_MOVIES).getAbsolutePath();

        Log.i(TAG, "onCreate: resourceImageDir = " + resourceImageDir);
        Log.i(TAG, "onCreate: resourceVideoDir = " + resourceVideoDir);

        resultVideo = new File(resourceVideoDir, "jpg_result.mp4");
        mergeVideo = new File(resourceVideoDir, "merge_result.mp4");
        addMusicFile = new File(resourceVideoDir, "add_result_music.mp4");
        scaleResultFile = new File(resourceVideoDir, "result_scale.mp4");
        noAudioResultFile = new File(resourceVideoDir, "no_audio.mp4");
        txtFile = new File(resourceVideoDir, "file_path.txt");
        blackBorderFile = new File(resourceImageDir, "black_border_img.jpeg");

        selectMediaList = new ArrayList<>();
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
                String srcPath = resourceImageDir + File.separator + "name004.jpg";
                new Thread(new Runnable() {
                    @Override
                    public void run() {
//                        fmpegNative.doJpgToVideo(srcPath, resultVideo.getAbsolutePath());
                        long startTime = System.currentTimeMillis();
                        FFmpegCmd.getInstance().jpgToVideo(srcPath, resultVideo.getAbsolutePath(), -1);
                        long endTime = System.currentTimeMillis();
                        Log.d(TAG, "run: 耗时 = " + (endTime-startTime)/1000);
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
//                        String srcPath1 = resourceVideoDir + File.separator + "test_1280x720_2.mp4";
//                        String srcPath1 = resourceVideoDir + File.separator + "VID_20211028_111908.mp4";
//                        String srcPath2 = resourceVideoDir + File.separator + "VID_20211101_174852.mp4";

                        PermissionX.init(MainActivity.this)
                                .permissions(Manifest.permission.READ_EXTERNAL_STORAGE,
                                            Manifest.permission.WRITE_EXTERNAL_STORAGE)
                                .request(new RequestCallback() {
                                    @Override
                                    public void onResult(boolean allGranted, @NonNull List<String> grantedList, @NonNull List<String> deniedList) {
                                        if (allGranted) {
//                                            String src1 = resourceVideoDir + File.separator + "hasiqi_video.mp4";
//                                            String src1 = resourceVideoDir + File.separator + "jpg_result.mp4";
//                                            String src1 = resourceVideoDir + File.separator + "test_1280x720_4.mp4";
//                                            String src2 = resourceVideoDir + File.separator + "VID_20211101_174852.mp4";
//                                            String[] tmpStrArr = {src1, src2};
                                            if (mergeVideo.exists()) {
                                                mergeVideo.delete();
                                            }
                                            fmpegNative.mergeFiles(stringArr, mergeVideo.getAbsolutePath());
                                            Log.d(TAG, "onResult: " + fmpegNative.getMergeStatus());
                                        } else  {
                                            Log.d(TAG, "onResult: 没有权限");
                                        }
                                    }
                                });
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
                        String videoSrcPath = resourceVideoDir + File.separator + "no_audio.mp4";
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

        binding.deleteAudio.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                new Thread(new Runnable() {
                    @Override
                    public void run() {
                        String srcPath = resourceVideoDir + File.separator + "test_1280x720_2.mp4";

                        fmpegNative.deleteAudio(srcPath, noAudioResultFile.getAbsolutePath());
                    }
                }).start();
            }
        });

        binding.startSelectMedia.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                startChooseMedia();
            }
        });

        binding.videoSplicing.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                String leftVideo = resourceVideoDir + File.separator + "2.mp4";
                String rightVideo = resourceVideoDir + File.separator + "1.mp4";
                String resultVideo = resourceVideoDir + File.separator + "3.mp4";
                //ffmpeg -i 1.mp4 -i 2.mp4 -filter_complex hstack -preset fast 3.mp4
                FFmpegCmd.getInstance().horizontalSplicingVideo(leftVideo, rightVideo, resultVideo);
            }
        });

        binding.getVideoInfo.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
//                fmpegNative.startVideoPlayerWithPath(stringArr[0]);
//                int videoWidth = fmpegNative.getVideoWidthFormNdk();
//                int videoHeight = fmpegNative.getVideoHeightFormNdk();
//                int videoRotate = fmpegNative.getVideoRotateFormNdk();
//                Log.d(TAG, "onClick: width = " + videoWidth + ", height = " + videoHeight + ", rotate = " + videoRotate);
                new Thread(new Runnable() {
                    @Override
                    public void run() {
                        long startTime = System.currentTimeMillis();
                        FFmpegCmd.getInstance().mergeFiles(selectMediaList, txtFile.getAbsolutePath(), mergeVideo.getAbsolutePath());
                        long endTime = System.currentTimeMillis();
                        Log.d(TAG, "run: 合并耗时 = " + (endTime-startTime)/1000.0);
                    }
                }).start();

            }
        });

        binding.addBlackBorder.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                String imgPath = selectMediaList.get(0);
                Size imageSize = StringUtils.getImageSize(imgPath);
                int width = imageSize.getWidth();
                int height = imageSize.getHeight();
                Log.d(TAG, "onClick: width = " + width + ",height = " + height);
                FFmpegCmd.getInstance().addBlackBorder(imgPath, blackBorderFile.getAbsolutePath(), width, height, 720, 1280);
            }
        });

    }

    private void startChooseMedia() {
        PictureSelector.create(MainActivity.this)
                .openGallery(PictureMimeType.ofAll())
                .imageEngine(GlideEngine.createGlideEngine())
                .isWithVideoImage(true)
                .selectionMode(PictureConfig.MULTIPLE)
                .isPageStrategy(true, true)
                .maxSelectNum(99)
                .maxVideoSelectNum(99)
                .forResult(launcherResult);
    }

    private void startChooseImageIntentForResult(int requestCode) {
        Intent intent = new Intent();
        intent.setType("*/*");
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

    /**
     * 创建一个ActivityResultLauncher
     * @return
     */
    private ActivityResultLauncher<Intent> createActivityResultLauncher() {
        return registerForActivityResult(new ActivityResultContracts.StartActivityForResult(), new ActivityResultCallback<ActivityResult>() {
            @Override
            public void onActivityResult(ActivityResult result) {
                int resultCode = result.getResultCode();
                if (resultCode == RESULT_OK) {
                    List<LocalMedia> selectList = PictureSelector.obtainMultipleResult(result.getData());
                    selectMediaList.clear();
                    stringArr = new String[selectList.size()];
                    for (int i = 0; i < selectList.size(); i++) {
                        String realPath = selectList.get(i).getRealPath();
                        Log.i(TAG, "绝对路径:" + realPath);
                        stringArr[i] = realPath;
                        selectMediaList.add(realPath);
                    }
                }
            }
        });
    }
}