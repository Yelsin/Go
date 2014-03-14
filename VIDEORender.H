// VideoRender.h: interface for the VideoRender class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_VIDEORENDER_H__50CC0807_947C_45F5_A886_C961B8AC7D33__INCLUDED_)
#define AFX_VIDEORENDER_H__50CC0807_947C_45F5_A886_C961B8AC7D33__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class VideoRender  
{
public:
	VideoRender();
	virtual ~VideoRender();
private://成员变量
	HDC m_hDC;
	int m_height;
	int m_width;
public://公共接口
	void SetDC(HDC dc);
	void SetSize(int width,int height);
	void DrawRGB(unsigned char *pData,unsigned long length);
	void DrawRGB(unsigned char *pData,unsigned long length,HDC hdc);
	void Init();
	void Close();
};

#endif // !defined(AFX_VIDEORENDER_H__50CC0807_947C_45F5_A886_C961B8AC7D33__INCLUDED_)
