#pragma once
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#ifdef __cplusplus
extern "C" {
#endif
#include <libavcodec/avcodec.h>
#include <libavutil/mem.h>
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

/* 5 seconds stream duration */
#define STREAM_DURATION   22.0
#define STREAM_FRAME_RATE 25 /* 25 images/s */
#define STREAM_NB_FRAMES  ((int)(STREAM_DURATION * STREAM_FRAME_RATE))
#define STREAM_PIX_FMT    AV_PIX_FMT_YUV420P /* default pix_fmt */

#define BUF_SIZE 4096*500


class C264ToMp4
{
public:
	C264ToMp4(void);
	~C264ToMp4(void);

	int open_codec_context(int *stream_idx,
	AVFormatContext *fmt_ctx, enum AVMediaType type);
	AVStream *add_stream(AVFormatContext *oc, AVCodec **codec,
		enum AVCodecID codec_id);
	void open_audio(AVFormatContext *oc, AVCodec *codec, AVStream *st);
	void get_audio_frame(int16_t *samples, int frame_size, int nb_channels);
	void write_audio_frame(AVFormatContext *oc, AVStream *st);
	void close_audio(AVFormatContext *oc, AVStream *st);
	void open_video(AVFormatContext *oc, AVCodec *codec, AVStream *st);
	void write_video_frame(AVFormatContext *oc, AVStream *st);
	void close_video(AVFormatContext *oc, AVStream *st);
	int nal_to_mp4file(BYTE* pVideo,int len);
	static int read_data(void* opaque, uint8_t *buf, int buf_size);
	int  h264FileToMp4();



private:
	AVFormatContext *input_fmt_ctx;
	int sws_flag;
	AVPacket input_pkt;
	int video_stream_idx;
	uint8_t *video_dst_data[4];
	int      video_dst_linesize[4];
	AVStream *input_video_stream;
	AVFrame *input_frame;
	AVCodecContext *video_dec_ctx;
	int video_dst_bufsize;

	float t, tincr, tincr2;

	uint8_t **src_samples_data;
	int       src_samples_linesize;
	int       src_nb_samples;

	int max_dst_nb_samples;
	uint8_t **dst_samples_data;
	int       dst_samples_linesize;
	int       dst_samples_size;

	struct SwrContext *swr_ctx;

	AVFrame *frame;
	AVPicture src_picture, dst_picture;
	int frame_count;


};

