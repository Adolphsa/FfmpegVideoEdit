# For more information about using CMake with Android Studio, read the
# documentation: https://d.android.com/studio/projects/add-native-code.html

# Sets the minimum version of CMake required to build the native library.

cmake_minimum_required(VERSION 3.10.2)

# Declares and names the project.

project("fve")

# Creates and names a library, sets it as either STATIC
# or SHARED, and provides the relative paths to its source code.
# You can define multiple libraries, and CMake builds them for you.
# Gradle automatically packages shared libraries with your APK.

set(FFMPEG_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src/main/cpp/3rdparty/libFFmpeg)
SET(FAAC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src/main/cpp/3rdparty/libfaac)
set(LIBYUV_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src/main/cpp/3rdparty/libyuv)
set(CPPDIR ${CMAKE_CURRENT_SOURCE_DIR}/src/main/cpp)

#ffmpeg头文件
include_directories(${FFMPEG_DIR}/${ANDROID_ABI}/include)
link_directories(${FFMPEG_DIR}/${ANDROID_ABI}/lib)
set(FFMPEG_LIB avformat avcodec avfilter avutil swresample swscale fdk-aac x264 z c m dl)
#SET(FFMPEG_LIB avformat avcodec avdevice avfilter avutil swresample swscale fdk-aac x264 z c m dl)

#faac
INCLUDE_DIRECTORIES(${FAAC_DIR}/${ANDROID_ABI}/include)
LINK_DIRECTORIES(${FAAC_DIR}/${ANDROID_ABI}/lib)
SET(FAAC_LIB faac)

#libyuv
include_directories(${LIBYUV_DIR}/${ANDROID_ABI}/include)
link_directories(${LIBYUV_DIR}/${ANDROID_ABI}/lib)
set(LIBYUV_LIB yuv)

set(ANDROID_SYSLIBS android log GLESv3 OpenSLES)

set(LIB_SRC ${CPPDIR}/native-lib.cpp
        ${CPPDIR}/JpgVideo.cpp
        ${CPPDIR}/Merge.cpp
        ${CPPDIR}/FilterVideoScale.cpp
        ${CPPDIR}/YuvUtils.cpp
        ${CPPDIR}/LCVideoWriter.cpp
        ${CPPDIR}/LCMediaInfo.cpp)

add_library( # Sets the name of the library.
        fve

        # Sets the library as a shared library.
        SHARED

        # Provides a relative path to your source file(s).
        ${LIB_SRC})


target_link_libraries(fve
        ${ANDROID_SYSLIBS}
        ${FFMPEG_LIB}
        ${FAAC_LIB}
        ${LIBYUV_LIB}
        )