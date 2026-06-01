# FfmpegVideoEdit

FfmpegVideoEdit is an Android video editing demo project built around FFmpeg, JNI, and native C/C++ media processing.

The project shows how to call FFmpeg commands from Android Java code, how to bridge Java and native C++ through JNI, and how to handle common mobile video editing operations such as image-to-video conversion, video merging, audio removal, audio/video muxing, scaling, compression, and horizontal video splicing.

## Features

- Select images and videos from local storage
- Convert JPG images to MP4 videos
- Merge multiple image/video files into one MP4
- Scale and rotate images or videos
- Compress videos by target bitrate
- Remove audio from a video
- Add or merge audio with video
- Horizontally splice two videos with FFmpeg `hstack`
- Add black borders to images to fit a target resolution
- Read video metadata through native FFmpeg APIs, including width, height, rotation angle, and duration
- Use JNI to call native C++ implementations from Java

## Tech Stack

- Android Java
- C/C++ with JNI
- FFmpeg
- mobile-ffmpeg
- CMake
- Android NDK
- libyuv
- FAAC
- AndroidX AppCompat
- Material Components
- PictureSelector
- Glide
- PermissionX

## Project Structure

```text
FfmpegVideoEdit/
├── app/
│   ├── CMakeLists.txt
│   ├── build.gradle
│   └── src/main/
│       ├── AndroidManifest.xml
│       ├── java/com/lc/fve/
│       │   ├── MainActivity.java
│       │   ├── FFmpegCmd.java
│       │   ├── FFmpegNative.java
│       │   └── utils/
│       ├── cpp/
│       │   ├── native-lib.cpp
│       │   ├── JpgVideo.cpp / JpgVideo.h
│       │   ├── Merge.cpp / Merge.h
│       │   ├── FilterVideoScale.cpp / FilterVideoScale.h
│       │   ├── YuvUtils.cpp / YuvUtils.h
│       │   ├── LCVideoWriter.cpp / LCVideoWriter.h
│       │   ├── LCMediaInfo.cpp / LCMediaInfo.h
│       │   └── 3rdparty/
│       │       ├── libFFmpeg/
│       │       ├── libfaac/
│       │       └── libyuv/
│       └── res/
├── build.gradle
├── settings.gradle
├── gradlew
└── gradlew.bat
```

## Main Modules

### `MainActivity`

The main UI entry point. It handles permission requests, media selection, output file path setup, and button click events for different editing actions.

Typical actions include:

- Selecting media from the gallery
- Starting JPG-to-video conversion
- Merging selected media files
- Adding background music
- Scaling video
- Removing audio
- Horizontally splicing two videos
- Reading video information
- Adding black borders to images

### `FFmpegCmd`

A Java-side FFmpeg command wrapper based on `mobile-ffmpeg`.

It builds and executes FFmpeg command strings for operations such as:

- `jpgToVideo()`
- `scaleRotateJpg()`
- `rotateScaleVideo()`
- `compressVideo()`
- `mergeFiles()`
- `cutVideo()`
- `addBlackBorder()`
- `audioVideoMerge()`
- `deleteAudio()`
- `horizontalSplicingVideo()`

This module is useful for learning how to assemble FFmpeg commands dynamically inside an Android app.

### `FFmpegNative`

The Java JNI bridge for the native `fve` library.

It exposes native methods for:

- Getting video width, height, rotation, and duration
- Converting JPG to video through native code
- Setting encode parameters
- Merging files through native code
- Adding music
- Removing audio
- Scaling video

### Native C++ Layer

The native layer is built as a shared library named `fve`.

CMake links the native code with prebuilt FFmpeg, FAAC, libyuv, Android system libraries, OpenGL ES 3.0, OpenSL ES, and Android log libraries.

The native source files include media encoding, video merging, image-to-video conversion, scaling, YUV utilities, video writing, and media information parsing.

## Requirements

- Android Studio
- Android Gradle Plugin 7.0.0
- Gradle wrapper included in the repository
- Android SDK 30
- Android NDK
- CMake 3.10.2
- Android device or emulator running Android 6.0 or later

The app is configured with:

```gradle
compileSdk 30
minSdk 23
targetSdk 30
```

The current NDK ABI filters are:

```gradle
abiFilters 'armeabi-v7a', 'arm64-v8a'
```

## Build and Run

1. Clone the repository:

```bash
git clone https://github.com/Adolphsa/FfmpegVideoEdit.git
cd FfmpegVideoEdit
```

2. Open the project in Android Studio.

3. Make sure the Android SDK, NDK, and CMake are installed.

4. Sync Gradle.

5. Build and install the app:

```bash
./gradlew assembleDebug
```

On Windows:

```bat
gradlew.bat assembleDebug
```

6. Install the generated APK on a device:

```bash
adb install app/build/outputs/apk/debug/app-debug.apk
```

## Runtime Permissions

The app requests storage and camera related permissions:

```xml
<uses-permission android:name="android.permission.READ_EXTERNAL_STORAGE" />
<uses-permission android:name="android.permission.WRITE_EXTERNAL_STORAGE" />
<uses-permission android:name="android.permission.CAMERA" />
```

It also uses `requestLegacyExternalStorage="true"`, which is mainly relevant for Android 10 storage behavior.

## Example FFmpeg Operations

### Convert JPG to MP4

The Java command wrapper converts an image into a short MP4 video with H.264 encoding:

```bash
ffmpeg -y -f lavfi -i anullsrc=channel_layout=stereo:sample_rate=44100 \
  -loop 1 -i input.jpg \
  -pix_fmt yuv420p -c:v libx264 -r 0.3 -t 3 -s 720x1280 output.mp4
```

### Merge Multiple Files

The project preprocesses selected JPG/MP4 files, writes a concat list file, then merges them:

```bash
ffmpeg -y -f concat -safe 0 -i file_path.txt -c copy merge_result.mp4
```

### Remove Audio

```bash
ffmpeg -i input.mp4 -an -vcodec copy no_audio.mp4
```

### Compress Video

```bash
ffmpeg -y -i input.mp4 -b:v 2M -acodec aac -vcodec libx264 -pix_fmt yuv420p output.mp4
```

### Horizontal Video Splicing

```bash
ffmpeg -i left.mp4 -i right.mp4 -filter_complex hstack -preset veryfast output.mp4
```

## Notes

- Some media paths in `MainActivity` are sample file names under the app external files directory. You may need to place test media files on the device before running those operations.
- The project uses both Java-side `mobile-ffmpeg` commands and native C++ FFmpeg integration.
- The native build depends on the prebuilt third-party libraries under `app/src/main/cpp/3rdparty`.
- `jcenter()` is still present in `settings.gradle`; modern Android projects should prefer `google()` and `mavenCentral()` where possible.
- `mobile-ffmpeg` is no longer actively maintained upstream. For new production projects, consider evaluating a maintained FFmpeg Android distribution.

## License

This project is licensed under the MIT License.

You are free to use, modify, and distribute this project under the terms of the MIT License. See the LICENSE file for details.
