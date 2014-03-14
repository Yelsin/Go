// Go.h : main header file for the GO application
//

#if !defined(AFX_GO_H__E1A6EA18_8573_4E13_9558_0F884D505678__INCLUDED_)
#define AFX_GO_H__E1A6EA18_8573_4E13_9558_0F884D505678__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"		// main symbols

/////////////////////////////////////////////////////////////////////////////
// CGoApp:
// See Go.cpp for the implementation of this class
//

class CGoApp : public CWinApp
{
public:
	CGoApp();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CGoApp)
	public:
	virtual BOOL InitInstance();
	//}}AFX_VIRTUAL

// Implementation

	//{{AFX_MSG(CGoApp)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_GO_H__E1A6EA18_8573_4E13_9558_0F884D505678__INCLUDED_)
