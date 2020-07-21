//
// Created by 颜哲 on 2020/7/21.
// 媒体文件抽取音频视频流
//
#include <iostream>
#include <stdio.h>

extern "C" {
#include <libavformat/avformat.h>
#include <libavformat/avio.h>
}
using namespace std;
#define ADTS_HEADER_LEN  7;
#ifndef AV_WB32
#   define AV_WB32(p, val) do {                 \
        uint32_t d = (val);                     \
        ((uint8_t*)(p))[3] = (d);               \
        ((uint8_t*)(p))[2] = (d)>>8;            \
        ((uint8_t*)(p))[1] = (d)>>16;           \
        ((uint8_t*)(p))[0] = (d)>>24;           \
    } while(0)
#endif

#ifndef AV_RB16
#   define AV_RB16(x)                           \
    ((((const uint8_t*)(x))[0] << 8) |          \
      ((const uint8_t*)(x))[1])
#endif

static int alloc_and_copy(AVPacket *out,
                          const uint8_t *sps_pps, uint32_t sps_pps_size,
                          const uint8_t *in, uint32_t in_size)
{
    uint32_t offset         = out->size;
    uint8_t nal_header_size = offset ? 3 : 4;
    int err;

    err = av_grow_packet(out, sps_pps_size + in_size + nal_header_size);
    if (err < 0)
        return err;

    if (sps_pps)
        memcpy(out->data + offset, sps_pps, sps_pps_size);
    memcpy(out->data + sps_pps_size + nal_header_size + offset, in, in_size);
    if (!offset) {
        AV_WB32(out->data + sps_pps_size, 1);
    } else {
        (out->data + offset + sps_pps_size)[0] =
        (out->data + offset + sps_pps_size)[1] = 0;
        (out->data + offset + sps_pps_size)[2] = 1;
    }

    return 0;
}

int h264_extradata_to_annexb(const uint8_t *codec_extradata, const int codec_extradata_size, AVPacket *out_extradata, int padding)
{
    uint16_t unit_size;
    uint64_t total_size                 = 0;
    uint8_t *out                        = NULL, unit_nb, sps_done = 0,
            sps_seen                   = 0, pps_seen = 0, sps_offset = 0, pps_offset = 0;
    const uint8_t *extradata            = codec_extradata + 4;
    static const uint8_t nalu_header[4] = { 0, 0, 0, 1 };
    int length_size = (*extradata++ & 0x3) + 1; // retrieve length coded size, 用于指示表示编码数据长度所需字节数

    sps_offset = pps_offset = -1;

    /* retrieve sps and pps unit(s) */
    unit_nb = *extradata++ & 0x1f; /* number of sps unit(s) */
    if (!unit_nb) {
        goto pps;
    }else {
        sps_offset = 0;
        sps_seen = 1;
    }

    while (unit_nb--) {
        int err;

        unit_size   = AV_RB16(extradata);
        total_size += unit_size + 4;
        if (total_size > INT_MAX - padding) {
            av_log(NULL, AV_LOG_ERROR,
                   "Too big extradata size, corrupted stream or invalid MP4/AVCC bitstream\n");
            av_free(out);
            return AVERROR(EINVAL);
        }
        if (extradata + 2 + unit_size > codec_extradata + codec_extradata_size) {
            av_log(NULL, AV_LOG_ERROR, "Packet header is not contained in global extradata, "
                                       "corrupted stream or invalid MP4/AVCC bitstream\n");
            av_free(out);
            return AVERROR(EINVAL);
        }
        if ((err = av_reallocp(&out, total_size + padding)) < 0)
            return err;
        memcpy(out + total_size - unit_size - 4, nalu_header, 4);
        memcpy(out + total_size - unit_size, extradata + 2, unit_size);
        extradata += 2 + unit_size;
        pps:
        if (!unit_nb && !sps_done++) {
            unit_nb = *extradata++; /* number of pps unit(s) */
            if (unit_nb) {
                pps_offset = total_size;
                pps_seen = 1;
            }
        }
    }

    if (out)
        memset(out + total_size, 0, padding);

    if (!sps_seen)
        av_log(NULL, AV_LOG_WARNING,
               "Warning: SPS NALU missing or invalid. "
               "The resulting stream may not play.\n");

    if (!pps_seen)
        av_log(NULL, AV_LOG_WARNING,
               "Warning: PPS NALU missing or invalid. "
               "The resulting stream may not play.\n");

    out_extradata->data      = out;
    out_extradata->size      = total_size;

    return length_size;
}

int h264_mp4toannexb(AVFormatContext *fmt_ctx, AVPacket *in, FILE *dst_fd) {

    AVPacket *out = NULL;
    AVPacket spspps_pkt;

    int len;
    uint8_t unit_type;
    int32_t nal_size;
    uint32_t cumul_size = 0;
    const uint8_t *buf;
    const uint8_t *buf_end;
    int buf_size;
    int ret = 0, i;

    out = av_packet_alloc();

    buf = in->data;
    buf_size = in->size;
    buf_end = in->data + in->size;

    do {
        ret = AVERROR(EINVAL);
        if (buf + 4 /*s->length_size*/ > buf_end)
            goto fail;

        for (nal_size = 0, i = 0; i < 4/*s->length_size*/; i++)
            nal_size = (nal_size << 8) | buf[i];

        buf += 4; /*s->length_size;*/
        unit_type = *buf & 0x1f;

        if (nal_size > buf_end - buf || nal_size < 0)
            goto fail;

        /*
        if (unit_type == 7)
            s->idr_sps_seen = s->new_idr = 1;
        else if (unit_type == 8) {
            s->idr_pps_seen = s->new_idr = 1;
            */
        /* if SPS has not been seen yet, prepend the AVCC one to PPS */
        /*
        if (!s->idr_sps_seen) {
            if (s->sps_offset == -1)
                av_log(ctx, AV_LOG_WARNING, "SPS not present in the stream, nor in AVCC, stream may be unreadable\n");
            else {
                if ((ret = alloc_and_copy(out,
                                     ctx->par_out->extradata + s->sps_offset,
                                     s->pps_offset != -1 ? s->pps_offset : ctx->par_out->extradata_size - s->sps_offset,
                                     buf, nal_size)) < 0)
                    goto fail;
                s->idr_sps_seen = 1;
                goto next_nal;
            }
        }
    }
    */

        /* if this is a new IDR picture following an IDR picture, reset the idr flag.
         * Just check first_mb_in_slice to be 0 as this is the simplest solution.
         * This could be checking idr_pic_id instead, but would complexify the parsing. */
        /*
        if (!s->new_idr && unit_type == 5 && (buf[1] & 0x80))
            s->new_idr = 1;

        */
        /* prepend only to the first type 5 NAL unit of an IDR picture, if no sps/pps are already present */
        if (/*s->new_idr && */unit_type == 5 /*&& !s->idr_sps_seen && !s->idr_pps_seen*/) {

            h264_extradata_to_annexb(fmt_ctx->streams[in->stream_index]->codec->extradata,
                                     fmt_ctx->streams[in->stream_index]->codec->extradata_size,
                                     &spspps_pkt,
                                     AV_INPUT_BUFFER_PADDING_SIZE);

            if ((ret = alloc_and_copy(out,
                                      spspps_pkt.data, spspps_pkt.size,
                                      buf, nal_size)) < 0)
                goto fail;
            /*s->new_idr = 0;*/
            /* if only SPS has been seen, also insert PPS */
        }
            /*else if (s->new_idr && unit_type == 5 && s->idr_sps_seen && !s->idr_pps_seen) {
                if (s->pps_offset == -1) {
                    av_log(ctx, AV_LOG_WARNING, "PPS not present in the stream, nor in AVCC, stream may be unreadable\n");
                    if ((ret = alloc_and_copy(out, NULL, 0, buf, nal_size)) < 0)
                        goto fail;
                } else if ((ret = alloc_and_copy(out,
                                            ctx->par_out->extradata + s->pps_offset, ctx->par_out->extradata_size - s->pps_offset,
                                            buf, nal_size)) < 0)
                    goto fail;
            }*/ else {
            if ((ret = alloc_and_copy(out, NULL, 0, buf, nal_size)) < 0)
                goto fail;
            /*
            if (!s->new_idr && unit_type == 1) {
                s->new_idr = 1;
                s->idr_sps_seen = 0;
                s->idr_pps_seen = 0;
            }
            */
        }


        len = fwrite(out->data, 1, out->size, dst_fd);
        if (len != out->size) {
            av_log(NULL, AV_LOG_DEBUG, "warning, length of writed data isn't equal pkt.size(%d, %d)\n",
                   len,
                   out->size);
        }
        fflush(dst_fd);

        next_nal:
        buf += nal_size;
        cumul_size += nal_size + 4;//s->length_size;
    } while (cumul_size < buf_size);

    /*
    ret = av_packet_copy_props(out, in);
    if (ret < 0)
        goto fail;

    */
    fail:
    av_packet_free(&out);

    return ret;
}

//抽取音频数据
void adts_header2(char *header, int dataLen) {
    // aac级别，0: AAC Main 1:AAC LC (Low Complexity) 2:AAC SSR (Scalable Sample Rate) 3:AAC LTP (Long Term Prediction)
    int aac_type = 3; //这里我填0 1 2 3 都可以
    // 采样率下标，根据dump信息找到你的下标
    //0->96000 1->88200 2->64000 3->48000 4->44100 5->32000 6->24000 7->22050
    int sampling_frequency_index = 3; //这里要找你对应的AAC的下标
    // 声道数
    int channel_config = 2;

    // ADTS帧长度,包括ADTS长度和AAC声音数据长度的和。
    int adtsLen = dataLen + 7;

    // syncword,标识一个帧的开始，固定为0xFFF,占12bit(byte0占8位,byte1占前4位)
    header[0] = 0xff;
    header[1] = 0xf0;

    // ID,MPEG 标示符。0表示MPEG-4，1表示MPEG-2。占1bit(byte1第5位)
    header[1] |= (0 << 3);

    // layer,固定为0，占2bit(byte1第6、7位)
    header[1] |= (0 << 1);

    // protection_absent，标识是否进行误码校验。0表示有CRC校验，1表示没有CRC校验。占1bit(byte1第8位)
    header[1] |= 1;

    // profile,标识使用哪个级别的AAC。1: AAC Main 2:AAC LC 3:AAC SSR 4:AAC LTP。占2bit(byte2第1、2位)
    header[2] = aac_type << 6;

    // sampling_frequency_index,采样率的下标。占4bit(byte2第3、4、5、6位)
    header[2] |= (sampling_frequency_index & 0x0f) << 2;

    // private_bit,私有位，编码时设置为0，解码时忽略。占1bit(byte2第7位)
    header[2] |= (0 << 1);

    // channel_configuration,声道数。占3bit(byte2第8位和byte3第1、2位)
    header[2] |= (channel_config & 0x04) >> 2;
    header[3] = (channel_config & 0x03) << 6;

    // original_copy,编码时设置为0，解码时忽略。占1bit(byte3第3位)
    header[3] |= (0 << 5);

    // home,编码时设置为0，解码时忽略。占1bit(byte3第4位)
    header[3] |= (0 << 4);

    // copyrighted_id_bit,编码时设置为0，解码时忽略。占1bit(byte3第5位)
    header[3] |= (0 << 3);

    // copyrighted_id_start,编码时设置为0，解码时忽略。占1bit(byte3第6位)
    header[3] |= (0 << 2);

    // aac_frame_length,ADTS帧长度,包括ADTS长度和AAC声音数据长度的和。占13bit(byte3第7、8位，byte4全部，byte5第1-3位)
    header[3] |= ((adtsLen & 0x1800) >> 11);
    header[4] = (uint8_t) ((adtsLen & 0x7f8) >> 3);
    header[5] = (uint8_t) ((adtsLen & 0x7) << 5);

    // adts_buffer_fullness，固定为0x7FF。表示是码率可变的码流 。占11bit(byte5后5位，byte6前6位)
    header[5] |= 0x1f;
    header[6] = 0xfc;

    // number_of_raw_data_blocks_in_frame,值为a的话表示ADST帧中有a+1个原始帧，(一个AAC原始帧包含一段时间内1024个采样及相关数据)。占2bit（byte6第7、8位）。
    header[6] |= 0;

}

int getMediaInfo() {
    av_register_all(); //注册服务
    char *mediaUrl = "/Users/yanzhe/ffmpeg/ffmpeg_install/ffmpeg/bin/input.mp4";
    char *aacUrl = "/Users/yanzhe/ffmpeg/ffmpeg_install/ffmpeg/bin/input.aac";
    char *videoUrl = "/Users/yanzhe/ffmpeg/ffmpeg_install/ffmpeg/bin/input.h264";
    AVFormatContext *avFormatContext = avformat_alloc_context();
    int openRet = avformat_open_input(&avFormatContext, mediaUrl, NULL, NULL);
    if (openRet < 0) {
        av_log(NULL, AV_LOG_INFO, "%s\n", "打开文件失败");
    }
    FILE *outputFile = fopen(aacUrl, "wb");
    FILE *videoOutputFile = fopen(videoUrl, "wb");
    if (!outputFile) {
        avformat_close_input(&avFormatContext);
        av_log(NULL, AV_LOG_INFO, "%s\n", "打开输出文件失败");
        return -1;
    }
    av_dump_format(avFormatContext, 0, mediaUrl, 0);//输入流还是输出流 这是输入流填0输出流填1
    //找到一个流
    int audio_index = av_find_best_stream(avFormatContext, AVMEDIA_TYPE_AUDIO, -1, -1, NULL, 0);
    int video_index = av_find_best_stream(avFormatContext, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);
    if (audio_index < 0 || video_index < 0) {
        av_log(NULL, AV_LOG_INFO, "%s\n", "best stream error");
        avformat_close_input(&avFormatContext);
        fclose(outputFile);
        fclose(videoOutputFile);
        return -1;
    }
    AVPacket avPacket;
    av_init_packet(&avPacket);
    while (av_read_frame(avFormatContext, &avPacket) >= 0) {
        //读取的流index和上面的音频流一直
        if (avPacket.stream_index == audio_index) {
            char adts_header_buf[7];
            adts_header2(adts_header_buf, avPacket.size);
            fwrite(adts_header_buf, sizeof(char), 7, outputFile);
            int len = fwrite(avPacket.data, 1, avPacket.size, outputFile);
            if (len != avPacket.size) {
                av_log(NULL, AV_LOG_INFO, "%s\n", "write_warning");
            }
        } else if (avPacket.stream_index == video_index) {
            int len = h264_mp4toannexb(avFormatContext, &avPacket, videoOutputFile);
            if (len != avPacket.size) {
                av_log(NULL, AV_LOG_INFO, "%s\n", "write_warning");
            }
        }
        av_packet_unref(&avPacket);
    }
    avformat_close_input(&avFormatContext);
    if (outputFile) {
        fclose(outputFile);
        fclose(videoOutputFile);
    }
}

int main() {
    av_log_set_level(AV_LOG_DEBUG);
    getMediaInfo();
    return 0;
}