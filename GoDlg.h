// GoDlg.h : header file
//

#if !defined(AFX_GODLG_H__E6F596AB_CC9B_4B83_9D05_A9FCA2606CB3__INCLUDED_)
#define AFX_GODLG_H__E6F596AB_CC9B_4B83_9D05_A9FCA2606CB3__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
#include "videocap.h"
#include "videorender.h"
#include "udp.h"
#include "myx264.h"
#include "afxwin.h"
#include "Stream2Mp4.h"
/////////////////////////////////////////////////////////////////////////////
// CGoDlg dialog

class CGoDlg : public CDialog
{
// Construction
public:
	CGoDlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	//{{AFX_DATA(CGoDlg)
	enum { IDD = IDD_GO_DIALOG };
	CStatic	m_info;
	CStatic	m_pic;
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CGoDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	//{{AFX_MSG(CGoDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg LRESULT OnVideo(WPARAM,LPARAM);
	afx_msg void OnTimer(UINT nIDEvent);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
private:
	VideoCap m_cap;
	VideoRender m_render;
	VideoRender m_render264;
	BYTE * m_pBuf[MAX_WINBUF];
	BYTE * m_p264[MAX_WINBUF+10];
	BYTE * m_p264dec;
	BYTE * m_pI420;
	BYTE * m_pYUY2;
	HDC m_dcPic;
	HDC m_dcPic264;
	bool m_bCap;
	bool m_bLinked;
	CUdp m_udp;
	long m_total;
	
	bool m_bStop; // 停止采集h264视频流

	AVFormatContext *m_pOc;
	AVStream *m_pVideoSt;
	//AVFrame *frame;

private:
	bool Init();
	int FindIndex264();
	//int Deal264();
	int Deal264(AVFormatContext* &m_pOc, AVStream* &m_pVideoSt);
private:
	CStatic m_len;
	CStatic m_pic264;
private:
	CStatic m_showtime;
	CStatic m_tlen;
	CStatic m_kbs;
	CStatic m_timespan;
public:
//	afx_msg void OnDestroy();
	afx_msg void OnClose();
	afx_msg void OnBnClickedButton1();
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_GODLG_H__E6F596AB_CC9B_4B83_9D05_A9FCA2606CB3__INCLUDED_)
