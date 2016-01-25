// fginst.h : main header file for the FGINST application
//

#if !defined(AFX_FGINST_H__63B9824D_D668_4850_ABD5_E51E5AC5724A__INCLUDED_)
#define AFX_FGINST_H__63B9824D_D668_4850_ABD5_E51E5AC5724A__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"		// main symbols

/////////////////////////////////////////////////////////////////////////////
// CFginstApp:
// See fginst.cpp for the implementation of this class
//

class CFginstApp : public CWinApp
{
public:
	CFginstApp();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CFginstApp)
	public:
	virtual BOOL InitInstance();
	//}}AFX_VIRTUAL

// Implementation

	//{{AFX_MSG(CFginstApp)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_FGINST_H__63B9824D_D668_4850_ABD5_E51E5AC5724A__INCLUDED_)
