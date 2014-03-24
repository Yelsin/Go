#ifndef STREAM_TO_MP4_H
#define STREAM_TO_MP4_H
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#ifdef __cplusplus
extern "C" {
#endif
#include <libavutil/opt.h>
#include <libavutil/mathematics.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
#include <libavutil/imgutils.h>
#ifdef __cplusplus
}
#endif


#pragma comment(lib,"avcodec.lib")
#pragma comment(lib,"avformat.lib")
#pragma comment(lib,"avutil.lib")
#pragma comment(lib,"swscale.lib")
#pragma comment(lib,"avfilter.lib")
#pragma comment(lib,"postproc.lib")
#pragma comment(lib,"swresample.lib")
#pragma comment(lib,"avdevice.lib")

#define BUF_SIZE 4096*500

#if _MSC_VER
#define snprintf _snprintf
#endif

/* 5 seconds stream duration */
#define STREAM_DURATION   200.0
#define STREAM_FRAME_RATE 25 /* 25 images/s */
#define STREAM_NB_FRAMES  ((int)(STREAM_DURATION * STREAM_FRAME_RATE))
#define STREAM_PIX_FMT    AV_PIX_FMT_YUV420P /* default pix_fmt */

static AVFormatContext *input_fmt_ctx = NULL;
static int sws_flags = SWS_BICUBIC;
static AVPacket input_pkt;
static int video_stream_idx = -1;
static uint8_t *video_dst_data[4] = {NULL};
static int      video_dst_linesize[4];
static AVStream *input_video_stream;
static AVFrame *input_frame;
static AVCodecContext *video_dec_ctx = NULL;
static int video_dst_bufsize;


int open_codec_context(int *stream_idx,AVFormatContext *fmt_ctx, enum AVMediaType type);

/* Add an output stream. */
AVStream *add_stream(AVFormatContext *oc, AVCodec **codec,
        enum AVCodecID codec_id);
/**************************************************************/
/* audio output */

static float m_time, tincr, tincr2;
static uint8_t **src_samples_data;
static int       src_samples_linesize;
static int       src_nb_samples;

static int max_dst_nb_samples;
static uint8_t **dst_samples_data;
static int       dst_samples_linesize;
static int       dst_samples_size;

static struct SwrContext *swr_ctx = NULL;
void open_audio(AVFormatContext *oc, AVCodec *codec, AVStream *st);
/* Prepare a 16 bit dummy audio frame of 'frame_size' samples and
* 'nb_channels' channels. */
void get_audio_frame(int16_t *samples, int frame_size, int nb_channels);
void write_audio_frame(AVFormatContext *oc, AVStream *st);
void close_audio(AVFormatContext *oc, AVStream *st);


/* video output */
extern AVFrame *frame;
static AVPicture src_picture, dst_picture;
static int frame_count;
void open_video(AVFormatContext *oc, AVCodec *codec, AVStream *st);
void write_video_frame(AVFormatContext *oc, AVStream *st);
void close_video(AVFormatContext *oc, AVStream *st);


bool AddStream(AVFormatContext *&m_pOc,AVStream *&m_pVideoSt, int nWidth, int nHeight);
bool CreateMp4(const char* pszFileName, int nWidth, int nHeight, AVFormatContext *&m_pOc, AVStream *&m_pVideoSt);
//bool CreateMp4(const char* pszFileName, AVFormatContext *m_pOc, AVStream *m_pVideoSt);
bool WriteVideo(AVFormatContext *m_pOc, AVStream *m_pVideoSt, unsigned char* data, int nLen);
void CloseMp4(AVFormatContext *&m_pOc, AVStream *&m_pVideoSt);

#endif //STREAM_TO_MP4_H