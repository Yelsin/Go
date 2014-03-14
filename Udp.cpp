// Udp.cpp : implementation file
//

#include "stdafx.h"
//#include "Go.h"
#include "Udp.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CUdp
#define LOGFILE "c:\\go.log"
CUdp::CUdp()
{
	for(int i=0;i<MAX_WINBUF;i++)this->m_ClientIndex[i].bUsed=false;
}

CUdp::~CUdp()
{
}

bool logfile(char * s)
{
//#ifdef _DEBUG	
	FILE *fp=fopen(LOGFILE,"a+");
	if(fp==NULL)return false;
	unsigned int size=fwrite(s,1,strlen(s),fp);
	if(size<strlen(s))
	{
		fclose(fp);
		return false;
	}
	fclose(fp);
//#endif
	return true;
}
bool logfile(int i)
{
//#ifdef _DEBUG	
	FILE *fp=fopen(LOGFILE,"a+");
	if(fp==NULL)return false;
	char s[255];
	sprintf(s,"%d ",i);
	unsigned int size=fwrite(s,1,strlen(s),fp);
	if(size<strlen(s))
	{
		fclose(fp);
		return false;
	}
	fclose(fp);
//#endif
	return true;
}

bool logfile(int i,BYTE*p,int len)
{
//#ifdef _DEBUG	
	CString sss;
	sss.Format("C:\\go\\%d.264",i);
	static FILE *fp=fopen(sss.GetBuffer(0),"w+b");
	if(fp==NULL)return false;
	unsigned int size=fwrite(p,1,len,fp);

//#endif
	return true;
}

bool logyuv(BYTE*p,int len)
{
//#ifdef _DEBUG	
	CString sss="C:\\test.yuv";
	static FILE *fp=fopen(sss.GetBuffer(0),"w+b");
	if(fp==NULL)return false;
	unsigned int size=fwrite(p,1,len,fp);

//#endif
	return true;
}
// Do not edit the following lines, which are needed by ClassWizard.
#if 0
BEGIN_MESSAGE_MAP(CUdp, CSocket)
	//{{AFX_MSG_MAP(CUdp)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()
#endif	// 0

/////////////////////////////////////////////////////////////////////////////
// CUdp member functions

void CUdp::OnReceive(int nErrorCode) 
{
	// TODO: Add your specialized code here and/or call the base class
	m_Section.Lock();
	unsigned int iRead;
	int index=this->FindBuf();
	iRead=Receive(m_buf[index]+1,8096);
	switch (iRead)
	{
		case SOCKET_ERROR:
			if (GetLastError() != WSAEWOULDBLOCK) 
			{
				//AfxMessageBox ("Error occurred");
				//Close();
			}
			break;
		default:
			OnRcv(index,iRead);
			this->m_buf[index][0]=0;
			break;
	}
	m_Section.Unlock();
//	CSocket::OnReceive(nErrorCode);
}

void CUdp::Init(HWND hwnd)
{
	this->m_hwndDlg=hwnd;
	if(AfxSocketInit(NULL)==0)
	{
		AfxMessageBox("Init socket fail!");
		return ;
	}; 
	this->Create(0,SOCK_DGRAM);
	for(int i=0;i<MAX_UDPBUF;i++)
	{
		this->m_buf[i]=new BYTE[5000];
		this->m_buf[i][0]=0;//0 for can used; 1 for already used
	}
}
void CUdp::Init(HWND hwnd,int port)
{
	this->m_hwndDlg=hwnd;
	if(AfxSocketInit(NULL)==0)
	{
		AfxMessageBox("Init socket fail!");
		return ;
	}; 
	this->Create(port,SOCK_DGRAM);
	for(int i=0;i<MAX_UDPBUF;i++)
	{
		this->m_buf[i]=new BYTE[5000];
		this->m_buf[i][0]=0;//0 for can used; 1 for already used
	}
}
void CUdp::Exit()
{
	for(int i=0;i<MAX_UDPBUF;i++)
		delete this->m_buf[i];
}

void CUdp::SendVideo(BYTE*pVideo,int len)
{
	int index=this->FindBuf();
	UDP_HEAD head;
	head.msg=UDP_VIDEO;
	memcpy(this->m_buf[index]+1,&head,sizeof(UDP_HEAD));
	memcpy(this->m_buf[index]+1+sizeof(UDP_HEAD),pVideo,len);
	for(int i=0;i<MAX_WINBUF;i++)
	{
		if(this->m_ClientIndex[i].bUsed)
		{
			this->SendTo(this->m_buf[index]+1,sizeof(UDP_HEAD)+len,this->m_ClientIndex[i].port,this->m_ClientIndex[index].ip);
		}
	}
	this->m_buf[index][0]=0;
}

void CUdp::Regist()
{
	int index=this->FindBuf();
	UDP_HEAD head;
	ClientIndex cid;
//
	head.msg=UDP_REG;
	CString ss;unsigned int pp;
	this->GetSockName(ss,pp);
	strcpy(cid.ip,"127.0.0.1");
	cid.port=pp;
	this->m_port=pp;
/*	logfile(ss.GetBuffer(0));
	logfile(":");
	logfile(pp);
	logfile("\r\n");*/
	memcpy(this->m_buf[index]+1,&head,sizeof(UDP_HEAD));
	memcpy(this->m_buf[index]+1+sizeof(UDP_HEAD),&cid,sizeof(cid));
	this->SendTo(this->m_buf[index]+1,sizeof(UDP_HEAD)+sizeof(ClientIndex),SERVER_PORT,(LPCTSTR)("127.0.0.1"));
	this->m_buf[index][0]=0;
}

int CUdp::FindBuf()
{
	int index=0;
	for(int i=0;i<MAX_UDPBUF;i++)
	{
		if(this->m_buf[i][0]==0)
		{
			this->m_buf[i][0]=1;
			return i;
		}
	}
	logfile("error:udp buffer no used!");
	return -1;
}


void CUdp::OnRcv(int index, int iRead)
{
	UDP_HEAD *pMSG=(UDP_HEAD*)(this->m_buf[index]+1);
	ClientIndex *pClient;
	int c;//unused index client;
	int i=0;
	switch (pMSG->msg)
	{
		case UDP_REG:
			for(i=0;i<MAX_WINBUF;i++)
			{
				if(!this->m_ClientIndex[i].bUsed)
				{
					c=i;
					pClient=(ClientIndex*)(this->m_buf[index]+1+sizeof(UDP_HEAD));
					memcpy(this->m_ClientIndex[c].ip,pClient->ip,255);
			//		AfxMessageBox(this->m_ClientIndex[c].ip);
					this->m_ClientIndex[c].port=pClient->port;
					this->m_ClientIndex[c].bUsed=true;
					this->m_buf[index][0]=0;
					break;
				}
			}
			break;
		case UDP_VIDEO:
			this->RcvVideo(this->m_buf[index],iRead);
		default:
			break;
	}
}

void CUdp::RcvVideo(BYTE*p,int len)
{
	::PostMessage(this->m_hwndDlg,MSG_VIDEO,(WPARAM)p,len);
}

