#include <iostream>
#include <stdio.h>

extern "C" {
#include <libavformat/avformat.h>
#include <libavformat/avio.h>
}
using namespace std;
#define ADTS_HEADER_LEN  7;

//void adts_header(char *szAdtsHeader, int dataLen) {
//
//    int audio_object_type = 2;
//    int sampling_frequency_index = 7;
//    int channel_config = 2;
//
//    int adtsLen = dataLen + 7;
//
//    szAdtsHeader[0] = 0xff;         //syncword:0xfff                          高8bits
//    szAdtsHeader[1] = 0xf0;         //syncword:0xfff                          低4bits
//    szAdtsHeader[1] |= (0 << 3);    //MPEG Version:0 for MPEG-4,1 for MPEG-2  1bit
//    szAdtsHeader[1] |= (0 << 1);    //Layer:0                                 2bits
//    szAdtsHeader[1] |= 1;           //protection absent:1                     1bit
//
//    szAdtsHeader[2] =
//            (audio_object_type - 1) << 6;            //profile:audio_object_type - 1                      2bits
//    szAdtsHeader[2] |=
//            (sampling_frequency_index & 0x0f) << 2; //sampling frequency index:sampling_frequency_index  4bits
//    szAdtsHeader[2] |= (0 << 1);                             //private bit:0                                      1bit
//    szAdtsHeader[2] |=
//            (channel_config & 0x04) >> 2;           //channel configuration:channel_config               高1bit
//
//    szAdtsHeader[3] = (channel_config & 0x03) << 6;     //channel configuration:channel_config      低2bits
//    szAdtsHeader[3] |= (0 << 5);                      //original：0                               1bit
//    szAdtsHeader[3] |= (0 << 4);                      //home：0                                   1bit
//    szAdtsHeader[3] |= (0 << 3);                      //copyright id bit：0                       1bit
//    szAdtsHeader[3] |= (0 << 2);                      //copyright id start：0                     1bit
//    szAdtsHeader[3] |= ((adtsLen & 0x1800) >> 11);           //frame length：value   高2bits
//
//    szAdtsHeader[4] = (uint8_t) ((adtsLen & 0x7f8) >> 3);     //frame length:value    中间8bits
//    szAdtsHeader[5] = (uint8_t) ((adtsLen & 0x7) << 5);       //frame length:value    低3bits
//    szAdtsHeader[5] |= 0x1f;                                 //buffer fullness:0x7ff 高5bits
//    szAdtsHeader[6] = 0xfc;
//}


//文件操作
int fileFunc() {
    //重命名文件
    int moveRet = avpriv_io_move("/Users/yanzhe/ffmpeg/ffmpeg_install/ffmpegClion/test.md",
                                 "/Users/yanzhe/ffmpeg/ffmpeg_install/ffmpegClion/1212.md");
    printf("%d\n", moveRet);
    //删除文件，为啥要传入绝对路径才成功呢？
    int ret = avpriv_io_delete("/Users/yanzhe/ffmpeg/ffmpeg_install/ffmpegClion/1212.md");
    printf("%d\n", ret);
    if (ret < 0) {
        av_log(NULL, AV_LOG_INFO, "%s\n", "删除失败");
        return -1;
    }
    return -1;
}

//读取文件操作
int dirFunc() {
    //上下文对象
    AVIODirContext *context = NULL;
    //目录上下文
    AVIODirEntry *avioDirEntry = NULL;
    //打开目录
    int openRet = avio_open_dir(&context, "/Users/yanzhe/ffmpeg/ffmpeg_install/ffmpegClion", NULL);
    if (openRet < 0) {
        av_log(NULL, AV_LOG_INFO, "%s\n", "打开目录失败");
        return -1;
    }
    while (1) {
        //读取目录
        int readRet = avio_read_dir(context, &avioDirEntry);
        if (readRet < 0) {
            av_log(NULL, AV_LOG_INFO, "%s\n", "读取失败");
            goto __fail;
        }
        //avioDirEntry为空，循环结束
        if (!avioDirEntry) {
            break;
        }
        av_log(NULL, AV_LOG_INFO, "%s %d\n", avioDirEntry->name, avioDirEntry->size);
        //释放资源
        avio_free_directory_entry(&avioDirEntry);
    }
    //释放资源
    __fail:
    avio_close_dir(&context);
    return -1;
}

//获取多媒体信息操作
int mediaInfo() {
    av_register_all(); //注册服务
    char *mediaUrl = "/Users/yanzhe/ffmpeg/ffmpeg_install/ffmpeg/bin/input.mp4";
    AVFormatContext *avFormatContext = avformat_alloc_context();
    int openRet = avformat_open_input(&avFormatContext, mediaUrl, NULL, NULL);
    if (openRet < 0) {
        av_log(NULL, AV_LOG_INFO, "%s\n", "打开文件失败");
    }
    av_dump_format(avFormatContext, 0, mediaUrl, 0);//输入流还是输出流 这是输入流填0输出流填1
    avformat_close_input(&avFormatContext);
}



int main() {
    av_log_set_level(AV_LOG_DEBUG);
//    fileFunc();
//    dirFunc();
//    mediaInfo();
    return 0;
}



