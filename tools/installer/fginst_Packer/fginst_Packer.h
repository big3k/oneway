// fginst_Packer.h : main header file for the FGINST_PACKER application
//

#if !defined(AFX_FGINST_PACKER_H__656A0A18_3FB4_427E_B963_5CC31C7BC6A9__INCLUDED_)
#define AFX_FGINST_PACKER_H__656A0A18_3FB4_427E_B963_5CC31C7BC6A9__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"		// main symbols

/////////////////////////////////////////////////////////////////////////////
// CFginst_PackerApp:
// See fginst_Packer.cpp for the implementation of this class
//

class CFginst_PackerApp : public CWinApp
{
public:
	CFginst_PackerApp();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CFginst_PackerApp)
	public:
	virtual BOOL InitInstance();
	//}}AFX_VIRTUAL

// Implementation

	//{{AFX_MSG(CFginst_PackerApp)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_FGINST_PACKER_H__656A0A18_3FB4_427E_B963_5CC31C7BC6A9__INCLUDED_)
