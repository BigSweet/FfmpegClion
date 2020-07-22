cmake_minimum_required(VERSION 3.15)
project(ffmpegClion)
set(FFMPEG_DIR /usr/local/ffmpeg)
include_directories(${FFMPEG_DIR}/include/) # 头文件搜索路径
link_directories(${FFMPEG_DIR}/lib/) # 动态链接库或静态链接库的搜索路径
set(CMAKE_CXX_STANDARD 14)
add_executable(ffmpegClion main.cpp)
add_executable(ffmpegClion1 media.cpp)
add_executable(ffmpegClion2 mediaExchange.cpp)
add_executable(ffmpegClion3 cutMedia.cpp)
add_executable(ffmpegClion4 mergeMedia.cpp)
add_executable(ffmpegClion5 h264Codec.cpp)
add_executable(ffmpegClion6 videoToPic.cpp)
add_executable(ffmpegClion7 aacCoder.cpp)
target_link_libraries(
        ffmpegClion
        avcodec
        avdevice
        avfilter
        avformat
        swresample
        avutil
        postproc
        swscale
)
target_link_libraries(
        ffmpegClion1
        avcodec
        avdevice
        avfilter
        avformat
        swresample
        avutil
        postproc
        swscale
)
target_link_libraries(
        ffmpegClion2
        avcodec
        avdevice
        avfilter
        avformat
        swresample
        avutil
        postproc
        swscale
)
target_link_libraries(
        ffmpegClion3
        avcodec
        avdevice
        avfilter
        avformat
        swresample
        avutil
        postproc
        swscale
)
target_link_libraries(
        ffmpegClion4
        avcodec
        avdevice
        avfilter
        avformat
        swresample
        avutil
        postproc
        swscale
)
target_link_libraries(
        ffmpegClion5
        avcodec
        avdevice
        avfilter
        avformat
        swresample
        avutil
        postproc
        swscale
)
target_link_libraries(
        ffmpegClion6
        avcodec
        avdevice
        avfilter
        avformat
        swresample
        avutil
        postproc
        swscale
)
target_link_libraries(
        ffmpegClion7
        avcodec
        avdevice
        avfilter
        avformat
        swresample
        avutil
        postproc
        swscale
)