#include "stdafx.h"
#include "myx264.h"
//#include "isomedia.h"

    x264_param_t param;
   x264_t *h;
    x264_picture_t pic;
	uint8_t *mux_buffer = NULL;
	int mux_buffer_size = 0;
struct AVCodecContext *ctx;  // Codec Context
struct AVFrame *picture;   // Frame	
struct AVCodec *codec;

void Initx264(int weith,int height,int fps)
{
   x264_param_default( &param );
	//x264_param_default_preset(&param, "fast" , "zerolatency" );
            //param.rc.i_lookahead = 0;
            //param.i_sync_lookahead = 0;
			//param.cpu=0;
            param.i_bframe = 0;
			param.i_keyint_max=150;
            //param.b_sliced_threads = 1;
            //param.b_vfr_input = 0;
            //param.rc.b_mb_tree = 0;
	//

	param.i_width=weith;
	param.i_height=height;
	param.i_fps_num=fps;
    param.i_fps_den       = 1;
	param.i_level_idc =10;
	param.rc.i_rc_method=X264_RC_ABR;
	param.rc.f_rf_constant=20;
	param.rc.i_vbv_max_bitrate=WIDTH/3*8;
	param.rc.i_bitrate =WIDTH/3*2*fps/20;

	h=x264_encoder_open(&param);
	if(h==NULL)return;

    x264_picture_alloc( &pic, X264_CSP_I420, param.i_width, param.i_height );

}

void Encode(BYTE * pIn,BYTE *pOut[],int index,int &length)
{
	static int i_frame=0;
	length=0;
    pic.i_pts = (int64_t)i_frame * param.i_fps_den;
//	BYTE * ptr=pOut;
	x264_picture_t pic_out;
    x264_nal_t *nal;
    int i_nal, i;
    int i_file = 0;
	//pic
	pic.i_pts=i_frame * param.i_fps_den;
	memcpy(pic.img.plane[0],pIn,param.i_width*param.i_height*3/2);

	int err=x264_encoder_encode( h, &nal, &i_nal, &pic, &pic_out );
    for( i = 0; i < i_nal; i++ )
    {
        int i_size;

        if( mux_buffer_size < nal[i].i_payload * 3/2 + 4 )
        {
            mux_buffer_size = nal[i].i_payload * 2 + 4;
            x264_free( mux_buffer );
            mux_buffer = (uint8_t *)x264_malloc( mux_buffer_size );
        }

        i_size = mux_buffer_size;
        x264_nal_encode( mux_buffer, &i_size, 1, &nal[i] );
		memcpy(3+pOut[(i+index)%15],mux_buffer,i_size);
		pOut[(i+index)%15][1]=(BYTE)i_size;
		pOut[(i+index)%15][2]=(BYTE)(i_size>>8);
		pOut[(i+index)%15][0]=1;
		length+=i_size;
//		ptr+=i_size;
       // i_file += p_write_nalu( hout, mux_buffer, i_size );
    }
	i_frame++;
	return;

}

int Initx264dec()
{
	//avcodec_init();
	picture = NULL;
	avcodec_register_all();
	codec = avcodec_find_decoder(CODEC_ID_H264);
	if (!codec)
	{
		printf("codec error!");
	}
	ctx = avcodec_alloc_context3(codec); 
	if(!ctx)
	{
		printf("ctx error!");
		return -1;
	}

	ctx->frame_number = 1;
	ctx->codec_id = CODEC_ID_H264;
	ctx->codec_type = AVMEDIA_TYPE_VIDEO;

    if (avcodec_open2(ctx,codec, 0) < 0)
	{
		printf("can not do avcodec_open2 ");
		return -1; 
	}
    picture = avcodec_alloc_frame();
	if(!picture)
		return -1;
	//int ii=decode_init(c);
	return 0;
}

// 将264解码为YUV，由以前的函数改写，参数len此处没用，pBufnal指向nalu所在单元
int Decode(BYTE *pYUV, int *len, BYTE *pBufnal)
{
	AVPacket packet = {0};
	av_init_packet(&packet);

	int nalLen=pBufnal[0]+((pBufnal[1]<<8)&0xff00);

	packet.data = pBufnal+2; 
	packet.size = nalLen;

	//printf("packet.data: %d\n", packet.data);

	int frameFinished = nalLen;
	//consumed_bytes= decode_frame(c, picture, len, pBufnal+2, nalLen); 
	int flag = avcodec_decode_video2(ctx, picture, &frameFinished, &packet); 
	if (flag < 0)
	{
		printf("problem decoding frame");
		//return -1;
	}

	if(frameFinished)
	{

		int i,j,k;			
		//fwrite(picture->data[0] + i * picture->linesize[0], 1, c->width, outf);
		for(i=0;i<ctx->height;i++)
			memcpy(pYUV+ctx->width*i, picture->data[0]+picture->linesize[0]*i,ctx->width);
		for(j=0;j<ctx->height/2;j++)
			memcpy(pYUV+ctx->width*i+ctx->width/2*j,picture->data[1]+picture->linesize[1]*j,ctx->width/2);
		for(k=0;k<ctx->height/2;k++)
			memcpy(pYUV+ctx->width*i+ctx->width/2*j+ctx->width/2*k, picture->data[2]+picture->linesize[2]*k, ctx->width/2);

	}

	av_free_packet(&packet);

	return frameFinished;
}

#define uint8_t BYTE
void YUY2toI420(int inWidth, int inHeight, uint8_t *pSrc, uint8_t *pDest)
    {
        int i, j;
        uint8_t *u = pDest + (inWidth * inHeight);
        uint8_t *v = u + (inWidth * inHeight) / 4;

        for (i = 0; i < inHeight/2; i++)
        {

            uint8_t *src_l1 = pSrc + inWidth*2*2*i;
            uint8_t *src_l2 = src_l1 + inWidth*2;
            uint8_t *y_l1 = pDest + inWidth*2*i;
            uint8_t *y_l2 = y_l1 + inWidth;
            for (j = 0; j < inWidth/2; j++)
            {
                // two pels in one go
                *y_l1++ = src_l1[0];
                *u++ = src_l1[1];
                *y_l1++ = src_l1[2];
                *v++ = src_l1[3];
                *y_l2++ = src_l2[0];
                *y_l2++ = src_l2[2];
                src_l1 += 4;
                src_l2 += 4;
            }
        }
		return;
    }

void I420toYUY2(int w,int h,BYTE * i420, BYTE *yuy2)
{
	BYTE * yy,*uu,*vv;
	yy=i420;
	uu=i420+w*h;
	vv=i420+w*h+w*h/4;
	for(int i=0;i<h;i++)
	{
		for(int j=0;j<w;j=j+2)
		{
			if((i%2)==0)
			{
				yuy2[i*w+j*2+0]=yy[i*w+j];
				yuy2[i*w+j*2+1]=uu[i*w/4+j];
				yuy2[i*w+j*2+2]=yy[i*w+j+1];
				yuy2[i*w+j*2+3]=vv[i*w/4+j];
			}
			else
			{
				yuy2[i*w+j*2+0]=yy[i*w+j];
				yuy2[i*w+j*2+1]=uu[(i-1)*w/4+j];
				yuy2[i*w+j*2+2]=yy[i*w+j+1];
				yuy2[i*w+j*2+3]=vv[(i-1)*w/4+j];

			}

		}
	}
	return;
}

bool YV12_to_RGB24(unsigned char* pYV12, unsigned char* pRGB24, int iWidth, int iHeight)
{
 if(!pYV12 || !pRGB24)
    return false;
 const long nYLen = long(iHeight * iWidth);
 const int nHfWidth = (iWidth>>1);
 if(nYLen<1 || nHfWidth<1) 
    return false;
 // yv12数据格式，其中Y分量长度为width * height, U和V分量长度都为width * height / 4
 // |WIDTH |
 // y......y--------
 // y......y   HEIGHT
 // y......y
 // y......y--------
 // v..v
 // v..v
 // u..u
 // u..u
 unsigned char* yData = pYV12;
 unsigned char* uData = &yData[nYLen];
 unsigned char* vData = &uData[nYLen>>2];
 if(!uData || !vData)
    return false;
 // Convert YV12 to RGB24
 // 
 // formula
 //                                       [1            1                        1             ]
 // [r g b] = [y u-128 v-128] [0            0.34375             0             ]
 //                                       [1.375      0.703125          1.734375]
 // another formula
 //                                       [1                   1                      1            ]
 // [r g b] = [y u-128 v-128] [0                   0.698001         0            ]
 //                                       [1.370705      0.703125         1.732446]
 int rgb[3];
 int i, j, m, n, x, y;
 m = -iWidth;
 n = -nHfWidth;
 for(y=0; y < iHeight; y++)
 {
    m += iWidth;
    if(!(y % 2))
    n += nHfWidth;
    for(x=0; x < iWidth; x++)
    {
    i = m + x;
    j = n + (x>>1);
    rgb[2] = int(yData[i] + 1.370705 * (vData[j] - 128)); // r分量值
    rgb[1] = int(yData[i] - 0.698001 * (uData[j] - 128)  - 0.703125 * (vData[j] - 128)); // g分量值
    rgb[0] = int(yData[i] + 1.732446 * (uData[j] - 128)); // b分量值
    j = nYLen - iWidth - m + x;
    i = (j<<1) + j;
    for(j=0; j<3; j++)
    {
   if(rgb[j]>=0 && rgb[j]<=255)
   pRGB24[i + j] = rgb[j];
   else
   pRGB24[i + j] = (rgb[j] < 0) ? 0 : 255;
    }
    }
 }
 return true;
}
 
//
BYTE clip255(LONG v)  
{  
 if(v<0) v=0;  
 else if(v>255) v=255;  
 return (BYTE)v;  
}  
void YUY2_RGB(BYTE *YUY2buff,BYTE *RGBbuff,DWORD dwSize)  
{  
//  
//C = Y - 16  
//D = U - 128  
//E = V - 128  
//R = clip(( 298 * C           + 409 * E + 128) >> 8)  
//G = clip(( 298 * C - 100 * D - 208 * E + 128) >> 8)  
//B = clip(( 298 * C + 516 * D           + 128) >> 8)  
 BYTE *orgRGBbuff = RGBbuff;  
 for(DWORD count=0;count<dwSize;count+=4)  
 {  
  //Y0 U0 Y1 V0  
  BYTE Y0 = *YUY2buff;  
  BYTE U = *(++YUY2buff);  
  BYTE Y1 = *(++YUY2buff);  
  BYTE V = *(++YUY2buff);  
  ++YUY2buff;  
  LONG Y,C,D,E;  
  BYTE R,G,B;  
    
  Y = Y0;  
  C = Y-16;  
  D = U-128;  
  E = V-128;  
  R = clip255(( 298 * C           + 409 * E + 128) >> 8);  
  G = clip255(( 298 * C - 100 * D - 208 * E + 128) >> 8);  
  B = clip255(( 298 * C + 516 * D           + 128) >> 8);  
  *(RGBbuff) =   B;            
  *(++RGBbuff) = G;    
  *(++RGBbuff) = R;          
  Y = Y1;  
  C = Y-16;  
  D = U-128;  
  E = V-128;  
  R = clip255(( 298 * C           + 409 * E + 128) >> 8);  
  G = clip255(( 298 * C - 100 * D - 208 * E + 128) >> 8);  
  B = clip255(( 298 * C + 516 * D           + 128) >> 8);  
  *(++RGBbuff) =   B;            
  *(++RGBbuff) = G;    
  *(++RGBbuff) = R;          
  ++RGBbuff;  
    
 }  
}  
//
void YUY2toRGB(BYTE *YUY2buff,BYTE *RGBbuff,DWORD dwSize)  
{  
//  
//C = Y - 16  
//D = U - 128  
//E = V - 128  
//R = clip(( 298 * C           + 409 * E + 128) >> 8)  
//G = clip(( 298 * C - 100 * D - 208 * E + 128) >> 8)  
//B = clip(( 298 * C + 516 * D           + 128) >> 8)  
 BYTE *pRGBend = RGBbuff+dwSize*3/2-1;  
 for(DWORD count=0;count<dwSize;count+=4)  
 {  
  //Y0 U0 Y1 V0  
  BYTE Y0 = *YUY2buff;  
  BYTE U = *(++YUY2buff);  
  BYTE Y1 = *(++YUY2buff);  
  BYTE V = *(++YUY2buff);  
  ++YUY2buff;  
  LONG Y,C,D,E;  
  BYTE R,G,B;  
    
  Y = Y0;  
  C = Y-16;  
  D = U-128;  
  E = V-128;  
  R = clip255(( 298 * C           + 409 * E + 128) >> 8);  
  G = clip255(( 298 * C - 100 * D - 208 * E + 128) >> 8);  
  B = clip255(( 298 * C + 516 * D           + 128) >> 8);  
  *(pRGBend) =   R;            
  *(--pRGBend) = G;    
  *(--pRGBend) = B;          
  Y = Y1;  
  C = Y-16;  
  D = U-128;  
  E = V-128;  
  R = clip255(( 298 * C           + 409 * E + 128) >> 8);  
  G = clip255(( 298 * C - 100 * D - 208 * E + 128) >> 8);  
  B = clip255(( 298 * C + 516 * D           + 128) >> 8);  
  *(--pRGBend) =   R;            
  *(--pRGBend) = G;    
  *(--pRGBend) = B;          
  --pRGBend;  
    
 }  
}  