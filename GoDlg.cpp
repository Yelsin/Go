// GoDlg.cpp : implementation file
//

#include "stdafx.h"
#include "Go.h"
#include "GoDlg.h"
#include "lib263.h"
#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CAboutDlg dialog used for App About

class CAboutDlg : public CDialog
{
public:
	CAboutDlg();

	// Dialog Data
	//{{AFX_DATA(CAboutDlg)
	enum { IDD = IDD_ABOUTBOX };
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAboutDlg)
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

	// Implementation
protected:
	//{{AFX_MSG(CAboutDlg)
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
	//{{AFX_DATA_INIT(CAboutDlg)
	//}}AFX_DATA_INIT
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAboutDlg)
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
	//{{AFX_MSG_MAP(CAboutDlg)
	// No message handlers
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CGoDlg dialog

CGoDlg::CGoDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CGoDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CGoDlg)
	// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
	// Note that LoadIcon does not require a subsequent DestroyIcon in Win32
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CGoDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CGoDlg)
	DDX_Control(pDX, IDC_INFO, m_info);
	DDX_Control(pDX, IDC_PIC, m_pic);
	//}}AFX_DATA_MAP
	DDX_Control(pDX, IDC_LEN, m_len);
	DDX_Control(pDX, IDC_264, m_pic264);
	DDX_Control(pDX, IDC_TIME, m_showtime);
	DDX_Control(pDX, IDC_TOTALLEN, m_tlen);
	DDX_Control(pDX, IDC_KBS, m_kbs);
	DDX_Control(pDX, IDC_TIMESPAN, m_timespan);
}

BEGIN_MESSAGE_MAP(CGoDlg, CDialog)
	//{{AFX_MSG_MAP(CGoDlg)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_MESSAGE(MSG_VIDEO,OnVideo)
	ON_WM_TIMER()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CGoDlg message handlers

BOOL CGoDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// Add "About..." menu item to system menu.

	// IDM_ABOUTBOX must be in the system command range.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		CString strAboutMenu;
		strAboutMenu.LoadString(IDS_ABOUTBOX);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	// TODO: Add extra initialization here
	this->Init();
	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CGoDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialog::OnSysCommand(nID, lParam);
	}
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CGoDlg::OnPaint() 
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, (WPARAM) dc.GetSafeHdc(), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialog::OnPaint();
	}
}

// The system calls this to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CGoDlg::OnQueryDragIcon()
{
	return (HCURSOR) m_hIcon;
}


bool CGoDlg::Init()
{
	this->m_bCap=false;
	this->m_bLinked=false;
	this->m_total=0;
	for(int i=0;i<MAX_WINBUF;i++)this->m_pBuf[i]=new BYTE[800000];
	for(int i=0;i<MAX_WINBUF+10;i++)
	{
		this->m_p264[i]=new BYTE[WIDTH*HEIGHT*3+100];
		this->m_p264[i][0]=0;
	}
	this->m_p264dec=new BYTE[WIDTH*HEIGHT*3];
	this->m_pI420=new BYTE[WIDTH*HEIGHT*3];
	this->m_pYUY2=new BYTE[WIDTH*HEIGHT*3];
	this->m_dcPic=(this->m_pic.GetDC())->m_hDC;
	this->m_render.SetSize(WIDTH,HEIGHT);
	this->m_render.SetDC(this->m_pic.GetDC()->m_hDC);
	this->m_render.Init();

	this->m_dcPic264=(this->m_pic264.GetDC())->m_hDC;
	this->m_render264.SetSize(WIDTH,HEIGHT);
	this->m_render264.SetDC(this->m_pic264.GetDC()->m_hDC);
	this->m_render264.Init();

	//	Init263(WIDTH,HEIGHT);
	this->SetTimer(2,1000,0);
	//	this->m_udp.Init(this->m_hWnd);
	//	this->m_udp.Regist();

	m_cap.SetCallBack();
	m_cap.SetCapSize(WIDTH,HEIGHT,25);
	//	m_cap.SetCapSize(176,144);
	m_cap.SetHwnd(this->GetSafeHwnd());
	m_cap.Init();
	m_cap.Start();
	this->m_bCap=true;

	Initx264(WIDTH,HEIGHT,25);
	Initx264dec();
	return true;
}

LRESULT CGoDlg::OnVideo(WPARAM l,LPARAM w)
{
	this->m_bLinked=true;
	static int iii=0;
	iii++;
	//	if(iii%4!=0)return 0;
	CString sss;
	sss.Format("帧:%d",iii);
	this->m_info.SetWindowText(sss);
	//time
	long time1,time2,span1,span2;
	time1=::GetCurrentTime();
	////////264
	BYTE *pRGB;
	BYTE *pYUV;
	pYUV=(BYTE*)w;
	YUY2toI420(WIDTH,HEIGHT,pYUV,this->m_pI420);
	static int index=0;
	index=this->FindIndex264();
	if(index==-1)return E_FAIL;
	int length=0;
	Encode(this->m_pI420,m_p264,index,length);
	time2=::GetCurrentTime();
	span1=time2-time1;
	this->m_total+=length;
	this->Deal264();
	time1=::GetCurrentTime();
	span2=time1-time2;
	//*	logfile(100,m_p264[index],length);
	//	logyuv(this->m_pI420,l);
	sss.Format("长度:%d",length);
	this->m_len.SetWindowTextA(sss);
	sss.Format("编码:%ld,解码%ld(毫秒)",span1,span2);
	this->m_timespan.SetWindowTextA(sss);
	pRGB=this->m_pBuf[index];
	YUY2toRGB((BYTE*)w,pRGB,l);

	//logfile(this->m_udp.GetPort(),(BYTE*)l,w);
	//logfile(this->m_udp.GetPort(),(BYTE*)l,w);
	//logfile(this->m_udp.GetPort(),(BYTE*)l,w);
	//Decompress((unsigned char*)(l+1),w,(BYTE**)&pOut);
	//////////263
	this->m_render.DrawRGB(pRGB,l*1.5);

	//	if(iii%2==0)this->OnVideo(l,w);
	//	((BYTE*)l)[0]=0;
	return 0;
	////

}


void CGoDlg::OnTimer(UINT nIDEvent) 
{
	// TODO: Add your message handler code here and/or call default
	static long int tick=0;
	tick++;
	CString s;
	s.Format("秒:%d",tick);
	this->m_showtime.SetWindowTextA(s);
	s.Format("总长:%dKB",this->m_total/1024);
	this->m_tlen.SetWindowTextA(s);
	s.Format("码率:%dkb/s",this->m_total/tick/1024*8);
	this->m_kbs.SetWindowTextA(s);
	//	CDialog::OnTimer(nIDEvent);
}

int CGoDlg::FindIndex264()
{
	for(int i=0;i<MAX_WINBUF+10;i++)
	{
		if(this->m_p264[i][0]==0)
			return i;
	}
	return -1;
}




int CGoDlg::Deal264()
{
	
	for(int i=0;i<15;i++)
	{
		if(this->m_p264[i][0]==1)
		{
			// m_p264[i]代表一个NALU单元，第一位为标志位，表示是否可用，第二位和第三位代表NALU长度，第四位开始为NALU数据
			this->m_p264[i][0]=0; 
			//todo:
			int cost,len;
			len=this->m_p264[i][1]+((this->m_p264[i][2])<<8);
			logfile(200,&(this->m_p264[i][3]),len);

			// 此处添加处理NALU的代码


			cost=Decode(this->m_p264dec,&len,&(this->m_p264[i][1]));
			if(cost<=0)continue;
			YV12_to_RGB24(this->m_p264dec,this->m_pBuf[1],WIDTH,HEIGHT);
			this->m_render264.DrawRGB(this->m_pBuf[1],WIDTH*HEIGHT*3);
		}
	}


	return 0;
}