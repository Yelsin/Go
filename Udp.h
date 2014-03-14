#if !defined(AFX_UDP_H__E458D60B_6728_4D81_81F1_A920E6E64B0D__INCLUDED_)
#define AFX_UDP_H__E458D60B_6728_4D81_81F1_A920E6E64B0D__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// Udp.h : header file
//
//protoss
typedef struct
{
	BYTE msg;//Э������define
	unsigned long stamp;//ʱ���
	unsigned long id;//������
}UDP_HEAD;
typedef struct
{
	bool bUsed;
	char ip[255];
	int port;
}ClientIndex;
#define UDP_REG  100
#define UDP_START 0
#define UDP_ACCEP_START 1
#define UDP_STOP 2;
#define UDP_ACCEPT_STOP 3
#define UDP_VIDEO 4
#define UDP_AUDIO 5
#define UDP_FILESTREAM 6
#define UDP_ACCEP_FILESTREAM 7
#define UDP_NEED_FILESTREAM 8
#define UDP_INFORMATIONEX 9
#define UDP_ACCEPT_INFORMATIONEX 9

////
#include "stdafx.h"
#include <afxsock.h>
#include <afxmt.h>
/////////////////////////////////////////////////////////////////////////////
// CUdp command target
bool logfile(int i,BYTE*p,int len);
bool logyuv(BYTE* yuv,int len);
class CUdp : public CSocket
{
// Attributes
public:

// Operations
public:
	CUdp();
	virtual ~CUdp();

// Overrides
public://INIT
	void Exit();
	void Init(HWND hwnd);//�ͻ���
	void Init(HWND hwnd,int port);//��������
public://method
	void Start(int index);//�������ˣ�IndexΪ�ͻ���ݱ�־
	void Stop(int index);
	void Regist();//�ͻ�ע��
	void SendVideo(BYTE*pVideo,int len);
	void SendAudio(int index,BYTE*pAudio,int len);
	unsigned int GetPort(){return m_port;}
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CUdp)
	public:
	virtual void OnReceive(int nErrorCode);
	//}}AFX_VIRTUAL

	// Generated message map functions
	//{{AFX_MSG(CUdp)
		// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_MSG

// Implementation
private:
	void RcvVideo(BYTE*p,int len);
	int FindBuf();
protected:

private:
	void OnRcv(int index,int iRead);
	BYTE* m_buf[MAX_UDPBUF];
	HWND m_hwndDlg;
	ClientIndex m_ClientIndex[MAX_WINBUF];
	CCriticalSection m_Section;//ͬ������
	unsigned int m_port;
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_UDP_H__E458D60B_6728_4D81_81F1_A920E6E64B0D__INCLUDED_)
