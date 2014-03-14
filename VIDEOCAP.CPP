// VideoCap.cpp: implementation of the VideoCap class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "VideoCap.h"


#pragma comment( lib, "vfw32.lib" )
//#pragma   comment(lib,"libcmt.lib") 
#include <vfw.h>

HWND g_hMywnd;

HRESULT CALLBACK OnCaptureVideo(HWND mwnd,LPVIDEOHDR lphdr)
{
	unsigned char *buffer=lphdr->lpData;
	unsigned long length=lphdr->dwBytesUsed;
	PostMessage(g_hMywnd,MSG_VIDEO,length,(LPARAM)buffer);
	
	return TRUE;

}
////////////////
//È«¾Ö
//////////////////////////////////////////////////////////////////////
// Call back Function
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

VideoCap::VideoCap()
{
	this->Clear();
	this->m_width=400;
	this->m_height=300;
}

VideoCap::~VideoCap()
{
	this->Clear();
}

//////////////////////////////////////////////////////////////////////
// Pulbic Interface
//////////////////////////////////////////////////////////////////////

HRESULT VideoCap::Init()
{
	m_capWnd = capCreateCaptureWindow("Capture",
		WS_POPUP,0,0,m_width,m_height,0,0x0);
	if(m_capWnd==NULL)return E_FAIL;
    capSetCallbackOnVideoStream(m_capWnd,OnCaptureVideo);
	///test
	LPSTR name=new char[100];
	LPSTR ver=new char[100];
	///
	try
	{
		/*if( ! capDriverConnect(m_capWnd,0))
		{
			this->Clear();
			return E_FAIL;
		}*/
		while( ! capDriverConnect(m_capWnd,0))
		{
			continue;
		}
	}
	catch(int i)
	{
		i++;
		AfxMessageBox("fali");
	}
	m_mode=0;//qcif
	if(FAILED(this->SetCapParm()))
	{
		this->Clear();
		AfxMessageBox("format video error");
		return E_FAIL;
	}
	long dwSize = capGetVideoFormatSize(m_capWnd);


	capGetVideoFormat(m_capWnd, m_videoformat, dwSize);
	this->pBmpInfo=(PBITMAPINFO)m_videoformat;
	return S_OK;
}

HRESULT VideoCap::SetCallBack(
	)
{
//	VideoCallBack=CallBack;
	return S_OK;
}

HRESULT VideoCap::SetCapSize(int x,int y,int fps)
{
	this->m_height=y;
	this->m_width=x;
	this->m_fps=fps;
	return S_OK;
}

HRESULT VideoCap::Start()
{
	capPreview(m_capWnd, TRUE);  
	if(capCaptureSequenceNoFile(m_capWnd)==FALSE)
		return E_FAIL;
	return S_OK;
}

HRESULT VideoCap::Stop()
{
//	Sleep(500);
	capCaptureStop(m_capWnd);
	Sleep(500);
	capCaptureAbort(m_capWnd);
	Sleep(500);	
	capSetCallbackOnVideoStream(m_capWnd, NULL);
     
	Sleep(500);		// This delay is important...
			
	// Finally disconnect the driver
    BOOL b;
	b=capDriverDisconnect(m_capWnd);
	return S_OK;
}
//////////////////////////////////////////////////////////////////////
// Privte Function
//////////////////////////////////////////////////////////////////////

HRESULT VideoCap::Clear()
{
	this->m_capWnd=NULL;

	return S_OK;
}

HRESULT VideoCap::SetCapParm()
{
	CAPTUREPARMS CapParms={0};	
	capCaptureGetSetup(m_capWnd,&CapParms,sizeof(CapParms));
	CapParms.fAbortLeftMouse = FALSE;
	CapParms.fAbortRightMouse = FALSE;
	CapParms.fYield = TRUE;
	CapParms.fCaptureAudio = FALSE;
	CapParms.wPercentDropForError = 50;
	CapParms.dwRequestMicroSecPerFrame=66667*15/this->m_fps;
	if(capCaptureSetSetup(m_capWnd,&CapParms,sizeof(CapParms))==FALSE)
	{
	return E_FAIL;
	}

	// Set Video Format 
	BITMAPINFO bmpinfo;
	capGetVideoFormat(m_capWnd,&bmpinfo,sizeof(bmpinfo));
	bmpinfo.bmiHeader.biWidth=this->m_width;
	bmpinfo.bmiHeader.biHeight=this->m_height;
	//bmpinfo.bmiHeader.biBitCount=24;
	//bmpinfo.bmiHeader.biCompression=BI_RGB;
	//bmpinfo.bmiHeader.biCompression=BI_BITFIELDS;
	BOOL ret=capSetVideoFormat(m_capWnd,&bmpinfo,sizeof(bmpinfo));

	if(ret==false)
		return E_FAIL;
	return S_OK;
}
HRESULT VideoCap::SetHwnd(HWND h)
{
	g_hMywnd=h;
	return S_OK;
}

