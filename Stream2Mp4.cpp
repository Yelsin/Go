#include "stdafx.h"
#include <stdio.h>
#include "Stream2Mp4.h"

int open_codec_context(int *stream_idx,
        AVFormatContext *fmt_ctx, enum AVMediaType type)
{
    int ret;
    AVStream *st;
    AVCodecContext *dec_ctx = NULL;
    AVCodec *dec = NULL;

    ret = av_find_best_stream(fmt_ctx, type, -1, -1, NULL, 0);
    if (ret < 0) {
        return ret;
    } else {
        *stream_idx = ret;
        st = fmt_ctx->streams[*stream_idx];

        /* find decoder for the stream */
        dec_ctx = st->codec;
        dec = avcodec_find_decoder(dec_ctx->codec_id);
        if (!dec) {
            fprintf(stderr, "Failed to find %s codec\n",
                    av_get_media_type_string(type));
            return ret;
        }

        if ((ret = avcodec_open2(dec_ctx, dec, NULL)) < 0) {
            fprintf(stderr, "Failed to open %s codec\n",
                    av_get_media_type_string(type));
            return ret;
        }
    }

    return 0;
}

/* Add an output stream. */
AVStream *add_stream(AVFormatContext *oc, AVCodec **codec,
        enum AVCodecID codec_id)
{
    AVCodecContext *c;
    AVStream *st;

    /* find the encoder */
    *codec = avcodec_find_encoder(codec_id);
    if (!(*codec)) {
        fprintf(stderr, "Could not find encoder for '%s'\n",
                avcodec_get_name(codec_id));
        exit(1);
    }

    st = avformat_new_stream(oc, *codec);
    if (!st) {
        fprintf(stderr, "Could not allocate stream\n");
        exit(1);
    }
    st->id = oc->nb_streams-1;
    c = st->codec;

    switch ((*codec)->type) {
        case AVMEDIA_TYPE_AUDIO:
            c->sample_fmt  = AV_SAMPLE_FMT_FLTP;
            c->bit_rate    = 64000;
            c->sample_rate = 44100;
            c->channels    = 2;
            break;

        case AVMEDIA_TYPE_VIDEO:
            c->codec_id = codec_id;

            c->bit_rate = 400000;
            c->time_base.den = STREAM_FRAME_RATE;
            c->time_base.num = 1;
            c->gop_size      = 12; 
            c->pix_fmt       = STREAM_PIX_FMT;
            break;

        default:
            break;
    }

    /* Some formats want stream headers to be separate. */
    if (oc->oformat->flags & AVFMT_GLOBALHEADER)
        c->flags |= CODEC_FLAG_GLOBAL_HEADER;

    return st;
}

/**************************************************************/
/* audio output */


void open_audio(AVFormatContext *oc, AVCodec *codec, AVStream *st)
{
    AVCodecContext *c;
    int ret;

    c = st->codec;

    /* open it */
    ret = avcodec_open2(c, codec, NULL);
    if (ret < 0) {
        //fprintf(stderr, "Could not open audio codec: %s\n", av_err2str(ret));
        fprintf(stderr, "Could not open audio codec: \n");
        exit(1);
    }

    /* init signal generator */
    m_time     = 0;
    tincr = 2 * M_PI * 110.0 / c->sample_rate;
    /* increment frequency by 110 Hz per second */
    tincr2 = 2 * M_PI * 110.0 / c->sample_rate / c->sample_rate;

    src_nb_samples = c->codec->capabilities & CODEC_CAP_VARIABLE_FRAME_SIZE ?
        10000 : c->frame_size;

    ret = av_samples_alloc_array_and_samples(&src_samples_data, &src_samples_linesize, c->channels,
            src_nb_samples, c->sample_fmt, 0);
    if (ret < 0) {
        fprintf(stderr, "Could not allocate source samples\n");
        exit(1);
    }

    /* create resampler context */
    if (c->sample_fmt != AV_SAMPLE_FMT_S16) {
        swr_ctx = swr_alloc();
        if (!swr_ctx) {
            fprintf(stderr, "Could not allocate resampler context\n");
            exit(1);
        }

        /* set options */
        av_opt_set_int       (swr_ctx, "in_channel_count",   c->channels,       0);
        av_opt_set_int       (swr_ctx, "in_sample_rate",     c->sample_rate,    0);
        av_opt_set_sample_fmt(swr_ctx, "in_sample_fmt",      AV_SAMPLE_FMT_S16, 0);
        av_opt_set_int       (swr_ctx, "out_channel_count",  c->channels,       0);
        av_opt_set_int       (swr_ctx, "out_sample_rate",    c->sample_rate,    0);
        av_opt_set_sample_fmt(swr_ctx, "out_sample_fmt",     c->sample_fmt,     0);

        /* initialize the resampling context */
        if ((ret = swr_init(swr_ctx)) < 0) {
            fprintf(stderr, "Failed to initialize the resampling context\n");
            exit(1);
        }
    }

    /* compute the number of converted samples: buffering is avoided
     * ensuring that the output buffer will contain at least all the
     * converted input samples */
    max_dst_nb_samples = src_nb_samples;
    ret = av_samples_alloc_array_and_samples(&dst_samples_data, &dst_samples_linesize, c->channels,
            max_dst_nb_samples, c->sample_fmt, 0);
    if (ret < 0) {
        fprintf(stderr, "Could not allocate destination samples\n");
        exit(1);
    }
    dst_samples_size = av_samples_get_buffer_size(NULL, c->channels, max_dst_nb_samples,
            c->sample_fmt, 0);
}

/* Prepare a 16 bit dummy audio frame of 'frame_size' samples and
* 'nb_channels' channels. */
void get_audio_frame(int16_t *samples, int frame_size, int nb_channels)
{
    int j, i, v;
    int16_t *q;

    q = samples;
    for (j = 0; j < frame_size; j++) {
        v = (int)(sin(m_time) * 10000);
        for (i = 0; i < nb_channels; i++)
            *q++ = v;
        m_time     += tincr;
        tincr += tincr2;
    }
}

void write_audio_frame(AVFormatContext *oc, AVStream *st)
{
    AVCodecContext *c;
    AVPacket pkt = { 0 }; // data and size must be 0;
    AVFrame *frame = avcodec_alloc_frame();
    int got_packet, ret, dst_nb_samples;

    av_init_packet(&pkt);
    c = st->codec;

    get_audio_frame((int16_t *)src_samples_data[0], src_nb_samples, c->channels);

    /* convert samples from native format to destination codec format, using the resampler */
    if (swr_ctx) {
        /* compute destination number of samples */
        dst_nb_samples = av_rescale_rnd(swr_get_delay(swr_ctx, c->sample_rate) + src_nb_samples,
                c->sample_rate, c->sample_rate, AV_ROUND_UP);
        if (dst_nb_samples > max_dst_nb_samples) {
            av_free(dst_samples_data[0]);
            ret = av_samples_alloc(dst_samples_data, &dst_samples_linesize, c->channels,
                    dst_nb_samples, c->sample_fmt, 0);
            if (ret < 0)
                exit(1);
            max_dst_nb_samples = dst_nb_samples;
            dst_samples_size = av_samples_get_buffer_size(NULL, c->channels, dst_nb_samples,
                    c->sample_fmt, 0);
        }

        /* convert to destination format */
        ret = swr_convert(swr_ctx,
                dst_samples_data, dst_nb_samples,
                (const uint8_t **)src_samples_data, src_nb_samples);
        if (ret < 0) {
            fprintf(stderr, "Error while converting\n");
            exit(1);
        }
    } else {
        dst_samples_data[0] = src_samples_data[0];
        dst_nb_samples = src_nb_samples;
    }

    frame->nb_samples = dst_nb_samples;
    avcodec_fill_audio_frame(frame, c->channels, c->sample_fmt,
            dst_samples_data[0], dst_samples_size, 0);

    ret = avcodec_encode_audio2(c, &pkt, frame, &got_packet);
    if (ret < 0) {
        //fprintf(stderr, "Error encoding audio frame: %s\n", av_err2str(ret));
        fprintf(stderr, "Error encoding audio frame:\n");
        exit(1);
    }

    if (!got_packet)
        return;

    pkt.stream_index = st->index;

    /* Write the compressed frame to the media file. */
    ret = av_interleaved_write_frame(oc, &pkt);
    if (ret != 0) {
        //fprintf(stderr, "Error while writing audio frame: %s\n",
         //       av_err2str(ret));
        fprintf(stderr, "Error while writing audio frame: \n");
        exit(1);
    }
    avcodec_free_frame(&frame);
}

void close_audio(AVFormatContext *oc, AVStream *st)
{
    avcodec_close(st->codec);
    av_free(src_samples_data[0]);
    av_free(dst_samples_data[0]);
}

/***********************bbs.ChinaFFmpeg.com****孙悟空***********************/
/* video output */

AVFrame* frame = NULL;

void open_video(AVFormatContext *oc, AVCodec *codec, AVStream *st)
{
    int ret;
    AVCodecContext *c = st->codec;

    /* open the codec */
    ret = avcodec_open2(c, codec, NULL);
    if (ret < 0) {
        //fprintf(stderr, "Could not open video codec: %s\n", av_err2str(ret));
        fprintf(stderr, "Could not open video codec: \n");
        exit(1);
    }

    /* allocate and init a re-usable frame */
    frame = avcodec_alloc_frame();
    if (!frame) {
        fprintf(stderr, "Could not allocate video frame\n");
        exit(1);
    }

    /* Allocate the encoded raw picture. */
    ret = avpicture_alloc(&dst_picture, c->pix_fmt, c->width, c->height);
    if (ret < 0) {
        //fprintf(stderr, "Could not allocate picture: %s\n", av_err2str(ret));
        fprintf(stderr, "Could not allocate picture: \n");
        exit(1);
    }

    /* If the output format is not YUV420P, then a temporary YUV420P
     * picture is needed too. It is then converted to the required
     * output format. */
    if (c->pix_fmt != AV_PIX_FMT_YUV420P) {
        ret = avpicture_alloc(&src_picture, AV_PIX_FMT_YUV420P, c->width, c->height);
        if (ret < 0) {
            fprintf(stderr, "Could not allocate temporary picture:\n");
			//fprintf(stderr, "Could not allocate temporary picture: %s\n",
				//av_err2str(ret));
            exit(1);
        }
    }

    /* copy data and linesize picture pointers to frame */
    *((AVPicture *)frame) = dst_picture;
}

void write_video_frame(AVFormatContext *oc, AVStream *st)
{
    int ret;
    static struct SwsContext *sws_ctx;
    AVCodecContext *c = st->codec;

    //    fill_yuv_image(&dst_picture, frame_count, c->width, c->height);


    if (oc->oformat->flags & AVFMT_RAWPICTURE) {
        /* Raw video case - directly store the picture in the packet */
        AVPacket pkt;
        av_init_packet(&pkt);

        pkt.flags        |= AV_PKT_FLAG_KEY;
        pkt.stream_index  = st->index;
        pkt.data          = dst_picture.data[0];
        pkt.size          = sizeof(AVPicture);

        ret = av_interleaved_write_frame(oc, &pkt);
    } else {
        AVPacket pkt = { 0 };
        int got_packet;
        av_init_packet(&pkt);

        /* encode the image */
        ret = avcodec_encode_video2(c, &pkt, frame, &got_packet);
        if (ret < 0) {
            //fprintf(stderr, "Error encoding video frame: %s\n", av_err2str(ret));
            fprintf(stderr, "Error encoding video frame: \n");
            exit(1);
        }
        /* If size is zero, it means the image was buffered. */

        if (!ret && got_packet && pkt.size) {
            pkt.stream_index = st->index;

            /* Write the compressed frame to the media file. */
            ret = av_interleaved_write_frame(oc, &pkt);
        } else {
            ret = 0;
        }
    }
    if (ret != 0) {
        //fprintf(stderr, "Error while writing video frame: %s\n", av_err2str(ret));
        fprintf(stderr, "Error while writing video frame: \n");
        exit(1);
    }
    frame_count++;
}

void close_video(AVFormatContext *oc, AVStream *st)
{
    avcodec_close(st->codec);
    av_free(src_picture.data[0]);
    av_free(dst_picture.data[0]);
    av_free(frame);
}



bool AddStream(AVFormatContext *&m_pOc,AVStream *&m_pVideoSt, int nWidth, int nHeight)
{
	AVCodecContext *pC;
	m_pVideoSt = avformat_new_stream(m_pOc, NULL);
	//m_pVideoSt = av_new_stream(m_pOc, 0);
	if (!m_pVideoSt)
		goto Exit0;


	m_pVideoSt->id = m_pOc->nb_streams-1;
	pC = m_pVideoSt->codec;

	pC->codec_id = CODEC_ID_H264;

	pC->bit_rate = 400000;
	pC->width    = nWidth;
	pC->height   = nHeight;
	pC->time_base.den = STREAM_FRAME_RATE;
	pC->time_base.num = 1;
	pC->gop_size      = 12;
	pC->pix_fmt       = STREAM_PIX_FMT;
	if (pC->codec_id == AV_CODEC_ID_MPEG2VIDEO)
		pC->max_b_frames = 2;
	if (pC->codec_id == AV_CODEC_ID_MPEG1VIDEO) 
		pC->mb_decision = 2;

	if (m_pOc->oformat->flags & AVFMT_GLOBALHEADER)
		pC->flags |= CODEC_FLAG_GLOBAL_HEADER;
	return true;
Exit0:
	return false;
}

// 添加mp4文件头
bool CreateMp4(const char* pszFileName, int nWidth, int nHeight, AVFormatContext* &m_pOc, AVStream* &m_pVideoSt)
{
	av_register_all();
	AVOutputFormat* pFmt;
	avformat_alloc_output_context2(&m_pOc, NULL, NULL, pszFileName);
	if (!m_pOc)
		avformat_alloc_output_context2(&m_pOc, NULL, "mpeg", pszFileName);
	if (!m_pOc)
		goto Exit0;
	pFmt = m_pOc->oformat;

	if (!AddStream(m_pOc, m_pVideoSt, nWidth, nHeight))
		goto Exit0;

	if (!(pFmt->flags & AVFMT_NOFILE))
	{
		if (avio_open(&m_pOc->pb, pszFileName, AVIO_FLAG_WRITE) < 0)
			goto Exit0;
	}

	if (avformat_write_header(m_pOc, NULL) < 0)
		goto Exit0;

	return true;
Exit0:
	return false;
}

// 向mp4文件中写h264数据
bool WriteVideo(AVFormatContext *m_pOc, AVStream *m_pVideoSt, unsigned char* data, int nLen)
{
	//AVCodecContext *c = m_pVideoSt->codec;
	if (nLen > 0)
	{
		AVPacket pkt = {0};
		const AVRational *time_base = &m_pVideoSt->codec->time_base;
		av_init_packet(&pkt);

		pkt.data = (uint8_t *)data;
		pkt.size = nLen;

		pkt.pts = av_rescale_q_rnd(pkt.pts, *time_base, m_pVideoSt->time_base, (AVRounding)(AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX));
		pkt.dts = av_rescale_q_rnd(pkt.dts, *time_base, m_pVideoSt->time_base, (AVRounding)(AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX));
		pkt.duration = av_rescale_q(pkt.duration, *time_base, m_pVideoSt->time_base);
		
		pkt.stream_index = m_pVideoSt->index;
	

		av_interleaved_write_frame(m_pOc, &pkt);

		return true;

		//AVPacket pkt;
		//av_init_packet(&pkt);

		//if (c->coded_frame->pts != AV_NOPTS_VALUE)
		//{
		//	pkt.pts = av_rescale_q(c->coded_frame->pts, c->time_base, m_pVideoSt->time_base);
		//}
		//if(c->coded_frame->key_frame)
		//	pkt.flags |= AV_PKT_FLAG_KEY;
		//pkt.stream_index= m_pVideoSt->index;
		//pkt.data= (uint8_t *)data;
		//pkt.size= nLen;

		///* write the compressed frame in the media file */
		//av_interleaved_write_frame(m_pOc, &pkt);

		//return true;
	}
	return false;
}

// 向mp4文件添加文件尾，关闭资源
void CloseMp4(AVFormatContext* &m_pOc, AVStream* &m_pVideoSt)
{
	if (m_pOc)
		av_write_trailer(m_pOc);

	m_pVideoSt = NULL;

	if (m_pOc && !(m_pOc->oformat->flags & AVFMT_NOFILE))
		avio_close(m_pOc->pb);

	if (m_pOc)
	{
		avformat_free_context(m_pOc);
		m_pOc = NULL;
	}
}

//
//bool CreateMp4(const char* mp4FileName, AVFormatContext *m_pOc, AVStream *m_pVideoSt)
//{
//	AVOutputFormat* pFmt;
//	AVCodec *video_codec;
//	int ret;
//
//	av_register_all();
//
//	avformat_alloc_output_context2(&m_pOc, NULL, NULL, mp4FileName);
//	if (!m_pOc)
//		avformat_alloc_output_context2(&m_pOc, NULL, "mpeg", mp4FileName);
//	if (!m_pOc)
//	{
//		return 1;
//	}
//
//	pFmt = m_pOc->oformat;
//
//    /* Add the audio and video streams using the default format codecs
//     * and initialize the codecs. */
//    m_pVideoSt = NULL;
//	pFmt->video_codec = AV_CODEC_ID_H264;
//
//    if (pFmt->video_codec != AV_CODEC_ID_NONE) {
//        m_pVideoSt = add_stream(m_pOc, &video_codec, AV_CODEC_ID_H264);
//    }
//    /* Now that all the parameters are set, we can open the audio and
//     * video codecs and allocate the necessary encode buffers. */
//    if (m_pVideoSt)
//        open_video(m_pOc, video_codec, m_pVideoSt);
//    av_dump_format(m_pOc, 0, mp4FileName, 1);
//    /* open the output file, if needed */
//    if (!(pFmt->flags & AVFMT_NOFILE)) {
//        ret = avio_open(&m_pOc->pb, mp4FileName, AVIO_FLAG_WRITE);
//        if (ret < 0) {
//            /*fprintf(stderr, "Could not open '%s': %s\n", filename,
//                    av_err2str(ret));*/
//            return false;
//        }
//    }
//    /* Write the stream header, if any. */
//    ret = avformat_write_header(m_pOc, NULL);
//	if (ret < 0)
//	{
//		return false;
//	}
//
//	return true;
//
//}
//
//bool WriteVideo(AVFormatContext *m_pOc, AVStream *m_pVideoSt, unsigned char* data, int nLen)
//{
//	AVCodecContext *c = m_pVideoSt->codec;
//	if (nLen > 0)
//	{
//
//		AVPacket pkt;
//		av_init_packet(&pkt);
//
//		if (c->coded_frame->pts != AV_NOPTS_VALUE)
//		{
//			pkt.pts = av_rescale_q(c->coded_frame->pts, c->time_base, m_pVideoSt->time_base);
//		}
//		if(c->coded_frame->key_frame)
//			pkt.flags |= AV_PKT_FLAG_KEY;
//		pkt.stream_index= m_pVideoSt->index;
//		pkt.data= (uint8_t *)data;
//		pkt.size= nLen;
//
//		/* write the compressed frame in the media file */
//		av_interleaved_write_frame(m_pOc, &pkt);
//
//		return true;
//	}
//	return false;
//}
//
//
//void CloseMp4(AVFormatContext *m_pOc, AVStream *m_pVideoSt)
//{
//
//	if (m_pOc)
//		av_write_trailer(m_pOc);
//
//	m_pVideoSt = NULL;
//
//	if (m_pOc && !(m_pOc->oformat->flags & AVFMT_NOFILE))
//		avio_close(m_pOc->pb);
//
//	if (m_pOc)
//	{
//		avformat_free_context(m_pOc);
//		m_pOc = NULL;
//	}
//}
//