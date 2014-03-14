// VideoCap.h: interface for the VideoCap class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_VIDEOCAP_H__94A98D3D_8016_4350_9489_88090BAAA037__INCLUDED_)
#define AFX_VIDEOCAP_H__94A98D3D_8016_4350_9489_88090BAAA037__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
#define step 3


#include "videorender.h"
class VideoCap  
{
public://构造&析构函数
	VideoCap();
	virtual ~VideoCap();
public://公共接口
	HRESULT Init();		//初始化，在设置完成后....
	HRESULT Start();		//开始采集，初始化完成后....
	HRESULT Stop();		//停止采集.....
	//设置
	HRESULT SetCallBack();//设置回调函数指针,必须是全局函数
	HRESULT SetCapSize(int x,int y,int fps);//设置采集窗口大小
	HRESULT SetHwnd(HWND h);
	///操作
	PBITMAPINFO pBmpInfo;
private://内部成员变量
	HRESULT Clear();		//清理
	HRESULT SetCapParm();//设置采集参数给VFW
private://内部成员函数
	HWND m_capWnd;		//采集窗口
	int m_height;
	int m_width;
	int m_fps;
	int m_mode;			//采集模式 0:(qcif 176x144) 1:(yuv2 640x480)
	char m_videoformat[1000];
};
//公共回调函数指针


#endif // !defined(AFX_VIDEOCAP_H__94A98D3D_8016_4350_9489_88090BAAA037__INCLUDED_)
