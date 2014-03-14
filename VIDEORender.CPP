// VideoRender.cpp: implementation of the VideoRender class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "VideoRender.h"
#include<vfw.h>
//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
BITMAPINFOHEADER bmpH;
HDRAWDIB hDib;
VideoRender::VideoRender()
{

}

VideoRender::~VideoRender()
{

}

void VideoRender::SetDC(HDC dc)
{
	this->m_hDC=dc;
}

void VideoRender::SetSize(int w,int h)
{
	this->m_height=h;
	this->m_width=w;

}

void VideoRender::Init()
{
	bmpH.biSize=sizeof(BITMAPINFOHEADER);
	bmpH.biWidth=this->m_width;
	bmpH.biHeight=this->m_height;
	bmpH.biPlanes=1;
	bmpH.biBitCount =24;
	bmpH.biCompression=BI_RGB;
	bmpH.biSizeImage=0;
	bmpH.biXPelsPerMeter=0;
	bmpH.biYPelsPerMeter=0;
	bmpH.biClrUsed=0;
	bmpH.biClrImportant=0;
	hDib=DrawDibOpen();
	if(hDib!=NULL)
	{
	 
		DrawDibBegin(hDib,
					   m_hDC,
					   -1,				// don't stretch
					   -1,				// don't stretch
					   &bmpH,
					   this->m_width,     // width of image
					   this->m_height,        // height of image
					   0				
					   );
	
	}
}

void VideoRender::Close()
{
	DrawDibEnd(hDib);
	DrawDibClose(hDib);
}

void VideoRender::DrawRGB(unsigned char *pData,unsigned long len)
{
		DrawDibDraw(hDib,
				  m_hDC,
				  0,		// dest : left pos
				  0,		// dest : top pos
				  -1,					 // don't zoom x
				  -1,					 // don't zoom y
				  &bmpH,			 // bmp header info
				  pData,					 // bmp data
				  0,					 // src :left
				  0,					 // src :top
				  m_width,				 // src : width
				  m_height,				 // src : height
				  DDF_SAME_DRAW			 // use prev params....
				  );
}

void VideoRender::DrawRGB(unsigned char *pData,unsigned long len,HDC hdc)
{
		DrawDibDraw(hDib,
				  hdc,
				  0,		// dest : left pos
				  0,		// dest : top pos
				  -1,					 // don't zoom x
				  -1,					 // don't zoom y
				  &bmpH,			 // bmp header info
				  pData,					 // bmp data
				  0,					 // src :left
				  0,					 // src :top
				  m_width,				 // src : width
				  m_height,				 // src : height
				  DDF_SAME_DRAW			 // use prev params....
				  );
}