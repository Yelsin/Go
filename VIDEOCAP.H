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
public://����&��������
	VideoCap();
	virtual ~VideoCap();
public://�����ӿ�
	HRESULT Init();		//��ʼ������������ɺ�....
	HRESULT Start();		//��ʼ�ɼ�����ʼ����ɺ�....
	HRESULT Stop();		//ֹͣ�ɼ�.....
	//����
	HRESULT SetCallBack();//���ûص�����ָ��,������ȫ�ֺ���
	HRESULT SetCapSize(int x,int y,int fps);//���òɼ����ڴ�С
	HRESULT SetHwnd(HWND h);
	///����
	PBITMAPINFO pBmpInfo;
private://�ڲ���Ա����
	HRESULT Clear();		//����
	HRESULT SetCapParm();//���òɼ�������VFW
private://�ڲ���Ա����
	HWND m_capWnd;		//�ɼ�����
	int m_height;
	int m_width;
	int m_fps;
	int m_mode;			//�ɼ�ģʽ 0:(qcif 176x144) 1:(yuv2 640x480)
	char m_videoformat[1000];
};
//�����ص�����ָ��


#endif // !defined(AFX_VIDEOCAP_H__94A98D3D_8016_4350_9489_88090BAAA037__INCLUDED_)