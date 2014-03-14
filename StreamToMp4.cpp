#include "stdafx.h"
#include "StreamToMp4.h"

C264ToMp4::C264ToMp4(void)
{
	//AVFormatContext *input_fmt_ctx = NULL;
	int sws_flag = SWS_BICUBIC;
	//AVPacket input_pkt;
	int video_stream_idx = -1;
	uint8_t *video_dst_data[4] = {NULL};
	//int      video_dst_linesize[4];
	//AVStream *input_video_stream;
	//AVFrame *input_frame;
	AVCodecContext *video_dec_ctx = NULL;
	//int video_dst_bufsize;

	//float t, tincr, tincr2;

	//uint8_t **src_samples_data;
	//int       src_samples_linesize;
	//int       src_nb_samples;

	//int max_dst_nb_samples;
	//uint8_t **dst_samples_data;
	//int       dst_samples_linesize;
	//int       dst_samples_size;

	struct SwrContext *swr_ctx = NULL;

	//AVFrame *frame;
	//AVPicture src_picture, dst_picture;
	//int frame_count;
}


C264ToMp4::~C264ToMp4(void)
{
}

int C264ToMp4::open_codec_context(int *stream_idx,
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
AVStream * C264ToMp4::add_stream(AVFormatContext *oc, AVCodec **codec,
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

void C264ToMp4::open_audio(AVFormatContext *oc, AVCodec *codec, AVStream *st)
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
    t     = 0;
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
void C264ToMp4::get_audio_frame(int16_t *samples, int frame_size, int nb_channels)
{
    int j, i, v;
    int16_t *q;

    q = samples;
    for (j = 0; j < frame_size; j++) {
        v = (int)(sin(t) * 10000);
        for (i = 0; i < nb_channels; i++)
            *q++ = v;
        t     += tincr;
        tincr += tincr2;
    }
}

void C264ToMp4::write_audio_frame(AVFormatContext *oc, AVStream *st)
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

void C264ToMp4::close_audio(AVFormatContext *oc, AVStream *st)
{
    avcodec_close(st->codec);
    av_free(src_samples_data[0]);
    av_free(dst_samples_data[0]);
}

/* video output */

void C264ToMp4::open_video(AVFormatContext *oc, AVCodec *codec, AVStream *st)
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

void C264ToMp4::write_video_frame(AVFormatContext *oc, AVStream *st)
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

void C264ToMp4::close_video(AVFormatContext *oc, AVStream *st)
{
    avcodec_close(st->codec);
    //av_free(src_picture.data[0]);
    //av_free(dst_picture.data[0]);
    av_free(frame);
}

//int C264ToMp4::read_data(void* opaque, uint8_t *buf, int buf_size)
//{
//	struct buffer_data *bd = (struct buffer_data *)opaque;
//	buf_size = FFMIN(buf_size, bd->size);
//	printf("ptr:%p size:%d\n", bd->ptr, bd->size);
//	/* copy internal buffer data to buf */
//	memcpy(buf, bd->ptr, buf_size);
//	bd->ptr  += buf_size;
//	bd->size -= buf_size;
//	return buf_size;
//}

//int C264ToMp4::nal_to_mp4file(BYTE* pVideo,int len)
//{
//	uint8_t *buf = (uint8_t*)av_malloc(sizeof(uint8_t)*BUF_SIZE);
//	AVCodec *video_codec, *audio_codec;
//	AVCodecContext *pVideoCodecCtx = NULL;
//	AVCodecContext *pAudioCodecCtx = NULL;
//	AVIOContext * pb = NULL;
//	AVInputFormat *piFmt = NULL;
//	AVOutputFormat *fmt = NULL;
//	AVFormatContext *oc= NULL;
//	const char* filename = "264ToMp4.mp4";
//	AVStream *audio_st, *video_st;
//	struct buffer_data bd = {0};
//	int ret;
//	int decoded;
//	int got_frame;
//	double audio_time, video_time;
//
//
//	av_register_all();
//
//	bd.ptr = pVideo;
//	bd.size = len;
//
//	//step1:申请一个AVIOContext
//	pb = avio_alloc_context(buf, BUF_SIZE, 0, NULL, read_data, NULL, NULL);
//	if (!pb) {
//		fprintf(stderr, "avio alloc failed!\n");
//		//return -1;
//	}
//	//step2:探测流格式
//	if (av_probe_input_buffer(pb, &piFmt, "", NULL, 0, 0) < 0) {
//		fprintf(stderr, "probe failed!\n");
//		//return -1;
//	} else {
//		fprintf(stdout, "probe success!\n");
//		fprintf(stdout, "format: %s[%s]\n", piFmt->name, piFmt->long_name);
//	}
//
//	input_fmt_ctx = avformat_alloc_context();
//	input_fmt_ctx->pb = pb;	//step3:这一步很关键
//	//step4:打开流
//	if (avformat_open_input(&input_fmt_ctx, "", piFmt, NULL) < 0) {
//		fprintf(stderr, "avformat open failed.\n");
//		//return -1;
//	} else {
//		fprintf(stdout, "open stream success!\n");
//	}
//	//以下就和文件处理一致了
//	if (av_find_stream_info(input_fmt_ctx) < 0) {
//		fprintf(stderr, "could not fine stream.\n");
//		//return -1;
//	}
//
//	if (open_codec_context(&video_stream_idx, input_fmt_ctx, AVMEDIA_TYPE_VIDEO) >= 0) {
//        input_video_stream = input_fmt_ctx->streams[video_stream_idx];
//        video_dec_ctx = input_video_stream->codec;
//        ret = av_image_alloc(video_dst_data, video_dst_linesize,
//                video_dec_ctx->width, video_dec_ctx->height,
//                video_dec_ctx->pix_fmt, 1);
//        if (ret < 0) {
//            fprintf(stderr, "Could not allocate raw video buffer\n");
//        }
//        video_dst_bufsize = ret;
//    }
//
//
//    input_frame = avcodec_alloc_frame();
//    if (!input_frame) {
//        fprintf(stderr, "Could not allocate frame\n");
//        ret = AVERROR(ENOMEM);
//        exit(-1);
//    }
//
//    /* allocate the output media context */
//    avformat_alloc_output_context2(&oc, NULL, NULL, filename);
//    if (!oc) {
//        printf("Could not deduce output format from file extension: using MPEG.\n");
//        avformat_alloc_output_context2(&oc, NULL, "mpeg", filename);
//    }
//    if (!oc) {
//        return 1;
//    }
//    fmt = oc->oformat;
//	
//    video_st = NULL;
//    //audio_st = NULL;
//
//    fmt->video_codec = AV_CODEC_ID_H264;
//    if (fmt->video_codec != AV_CODEC_ID_NONE) {
//        video_st = add_stream(oc, &video_codec, AV_CODEC_ID_H264);
//    }
//    if (video_st->codec) {
//        video_st->codec->width = video_dec_ctx->width;
//        video_st->codec->height = video_dec_ctx->height;
//    }
//   /* fmt->audio_codec = AV_CODEC_ID_MP3;
//    if (fmt->audio_codec != AV_CODEC_ID_NONE) {
//        audio_st = add_stream(oc, &audio_codec, AV_CODEC_ID_MP3);
//    }*/
//
//    /* Now that all the parameters are set, we can open the audio and
//     * video codecs and allocate the necessary encode buffers. */
//    if (video_st)
//        open_video(oc, video_codec, video_st);
//    /*if (audio_st)
//        open_audio(oc, audio_codec, audio_st);*/
//
//    av_dump_format(oc, 0, filename, 1);
//
//
//    /* open the output file, if needed */
//    if (!(fmt->flags & AVFMT_NOFILE)) {
//        ret = avio_open(&oc->pb, filename, AVIO_FLAG_WRITE);
//        if (ret < 0) {
//            //fprintf(stderr, "Could not open '%s': %s\n", filename,
//           //         av_err2str(ret));
//            fprintf(stderr, "Could not open '%s': \n", filename);
//            return 1;
//        }
//    }
//
//    /* Write the stream header, if any. */
//    ret = avformat_write_header(oc, NULL);
//    if (ret < 0) {
//        //fprintf(stderr, "Error occurred when opening output file: %s\n",
//        //        av_err2str(ret));
//		fprintf(stderr, "Error occurred when opening output file: \n");
//        return 1;
//    }
//
//    av_init_packet(&input_pkt);
//    input_pkt.data = NULL;
//    input_pkt.size = 0;
//
//
//    if (frame)
//        frame->pts = 0;
//    for (;;) {
//        /* Compute current audio and video time. */
//        //audio_time = audio_st ? audio_st->pts.val * av_q2d(audio_st->time_base) : 0.0;
//        video_time = video_st ? video_st->pts.val * av_q2d(video_st->time_base) : 0.0;
//
//		if (!video_st || video_time >= STREAM_DURATION)
//            break;
//
//        /* write interleaved audio and video frames */
//        /*if (!video_st || (video_st && audio_st && audio_time < video_time)) {
//            write_audio_frame(oc, audio_st);
//        }*/
//		else {
//            av_read_frame(input_fmt_ctx, &input_pkt);
//
//            decoded = input_pkt.size;
//            if (input_pkt.stream_index == video_stream_idx) {
//                /* decode video frame */
//                ret = avcodec_decode_video2(video_dec_ctx, input_frame, &got_frame, &input_pkt);
//                if (ret < 0) {
//                    fprintf(stderr, "Error decoding video frame\n");
//                    return ret;
//                }
//
//                if (got_frame) {
//                    av_image_copy((uint8_t**)&dst_picture, video_dst_linesize,
//                            (const uint8_t **)(input_frame->data), input_frame->linesize,
//                            video_dec_ctx->pix_fmt, video_dec_ctx->width, video_dec_ctx->height);
//                }
//            } 
//
//            write_video_frame(oc, video_st);
//            frame->pts += av_rescale_q(1, video_st->codec->time_base, video_st->time_base);
//        }
//    }
//
//    av_write_trailer(oc);
//
//    /* Close each codec. */
//    if (video_st)
//        close_video(oc, video_st);
//    /*if (audio_st)
//        close_audio(oc, audio_st);*/
//
//    if (!(fmt->flags & AVFMT_NOFILE))
//        avio_close(oc->pb);
//
//    avformat_free_context(oc);
//    av_free(input_frame);
//    av_free(video_dst_data[0]);
//    avcodec_close(video_dec_ctx);
//    return 0;
//}

//int C264ToMp4::h264FileToMp4()
//{
//    const char *filename;
//    const char *input_file;
//	AVFormatContext *input_fmt_ctx = NULL;
//    AVOutputFormat *fmt;
//    AVFormatContext *oc;
//    AVStream *video_st;
//    AVCodec *video_codec;
//    double video_time;
//    int ret;
//    int decoded;
//    int got_frame;
//
//    /* Initialize libavcodec, and register all codecs and formats. */
//    av_register_all();
//
//    /*if (argc != 3) {
//        printf("usage: %s output_file inputfile\n"
//                "API example program to output a media file with libavformat.\n"
//                "This program generates a synthetic audio and video stream, encodes and\n"
//                "muxes them into a file named output_file.\n"
//                "The output format is automatically guessed according to the file extension.\n"
//                "Raw images can also be output by using '%%d' in the filename.\n"
//                "\n", argv[0]);
//        return 1;
//    }*/
//
//    //input_file = argv[2];
//   // filename = argv[1];
//	input_file = "200.264";
//    filename = "my200.mp4";
//	//input_file = "E:\\sample.avi";
//    //filename = "E:\\sample.mp4";
//    if (avformat_open_input( &input_fmt_ctx, input_file, NULL, NULL) < 0) {
//        fprintf(stderr, "Could not open source file %s\n", input_file);
//        exit(-1);
//    }
//
//    /* retrieve stream information */
//    if (avformat_find_stream_info(input_fmt_ctx, NULL) < 0) {
//        fprintf(stderr, "Could not find stream information\n");
//        exit(1);
//    }
//
//    if (open_codec_context(&video_stream_idx, input_fmt_ctx, AVMEDIA_TYPE_VIDEO) >= 0) {
//        input_video_stream = input_fmt_ctx->streams[video_stream_idx];
//        video_dec_ctx = input_video_stream->codec;
//        ret = av_image_alloc(video_dst_data, video_dst_linesize,
//                video_dec_ctx->width, video_dec_ctx->height,
//                video_dec_ctx->pix_fmt, 1);
//        if (ret < 0) {
//            fprintf(stderr, "Could not allocate raw video buffer\n");
//        }
//        video_dst_bufsize = ret;
//    }
//
//
//    input_frame = avcodec_alloc_frame();
//    if (!input_frame) {
//        fprintf(stderr, "Could not allocate frame\n");
//        ret = AVERROR(ENOMEM);
//        exit(-1);
//    }
//
//    /* allocate the output media context */
//    avformat_alloc_output_context2(&oc, NULL, NULL, filename);
//    if (!oc) {
//        printf("Could not deduce output format from file extension: using MPEG.\n");
//        avformat_alloc_output_context2(&oc, NULL, "mpeg", filename);
//    }
//    if (!oc) {
//        return 1;
//    }
//    fmt = oc->oformat;
//	
//    video_st = NULL;
//    //audio_st = NULL;
//
//    fmt->video_codec = AV_CODEC_ID_H264;
//    if (fmt->video_codec != AV_CODEC_ID_NONE) {
//        video_st = add_stream(oc, &video_codec, AV_CODEC_ID_H264);
//    }
//    if (video_st->codec) {
//        video_st->codec->width = video_dec_ctx->width;
//        video_st->codec->height = video_dec_ctx->height;
//    }
//   /* fmt->audio_codec = AV_CODEC_ID_MP3;
//    if (fmt->audio_codec != AV_CODEC_ID_NONE) {
//        audio_st = add_stream(oc, &audio_codec, AV_CODEC_ID_MP3);
//    }*/
//
//    /* Now that all the parameters are set, we can open the audio and
//     * video codecs and allocate the necessary encode buffers. */
//    if (video_st)
//        open_video(oc, video_codec, video_st);
//    /*if (audio_st)
//        open_audio(oc, audio_codec, audio_st);*/
//
//    av_dump_format(oc, 0, filename, 1);
//
//
//    /* open the output file, if needed */
//    if (!(fmt->flags & AVFMT_NOFILE)) {
//        ret = avio_open(&oc->pb, filename, AVIO_FLAG_WRITE);
//        if (ret < 0) {
//            //fprintf(stderr, "Could not open '%s': %s\n", filename,
//           //         av_err2str(ret));
//            fprintf(stderr, "Could not open '%s': \n", filename);
//            return 1;
//        }
//    }
//
//    /* Write the stream header, if any. */
//    ret = avformat_write_header(oc, NULL);
//    if (ret < 0) {
//        //fprintf(stderr, "Error occurred when opening output file: %s\n",
//        //        av_err2str(ret));
//		fprintf(stderr, "Error occurred when opening output file: \n");
//        return 1;
//    }
//
//    av_init_packet(&input_pkt);
//    input_pkt.data = NULL;
//    input_pkt.size = 0;
//
//
//    if (frame)
//        frame->pts = 0;
//    for (;;) {
//        /* Compute current audio and video time. */
//        //audio_time = audio_st ? audio_st->pts.val * av_q2d(audio_st->time_base) : 0.0;
//        video_time = video_st ? video_st->pts.val * av_q2d(video_st->time_base) : 0.0;
//
//		if (!video_st || video_time >= STREAM_DURATION)
//            break;
//
//        /* write interleaved audio and video frames */
//        /*if (!video_st || (video_st && audio_st && audio_time < video_time)) {
//            write_audio_frame(oc, audio_st);
//        }*/
//		else {
//            av_read_frame(input_fmt_ctx, &input_pkt);
//
//            decoded = input_pkt.size;
//            if (input_pkt.stream_index == video_stream_idx) {
//                /* decode video frame */
//                ret = avcodec_decode_video2(video_dec_ctx, input_frame, &got_frame, &input_pkt);
//                if (ret < 0) {
//                    fprintf(stderr, "Error decoding video frame\n");
//                    return ret;
//                }
//
//                if (got_frame) {
//                    av_image_copy((uint8_t**)&dst_picture, video_dst_linesize,
//                            (const uint8_t **)(input_frame->data), input_frame->linesize,
//                            video_dec_ctx->pix_fmt, video_dec_ctx->width, video_dec_ctx->height);
//                }
//            } 
//
//            write_video_frame(oc, video_st);
//            frame->pts += av_rescale_q(1, video_st->codec->time_base, video_st->time_base);
//        }
//    }
//
//    av_write_trailer(oc);
//
//    /* Close each codec. */
//    if (video_st)
//        close_video(oc, video_st);
//    /*if (audio_st)
//        close_audio(oc, audio_st);*/
//
//    if (!(fmt->flags & AVFMT_NOFILE))
//        avio_close(oc->pb);
//
//    avformat_free_context(oc);
//    av_free(input_frame);
//    av_free(video_dst_data[0]);
//    avcodec_close(video_dec_ctx);
//    return 0;
//}