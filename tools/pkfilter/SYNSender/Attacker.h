// Attacker.h : main header file for the ATTACKER application
//

#if !defined(AFX_ATTACKER_H__0D5E761D_ED6E_498C_98BE_2EA72C6AA67D__INCLUDED_)
#define AFX_ATTACKER_H__0D5E761D_ED6E_498C_98BE_2EA72C6AA67D__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"		// main symbols

/////////////////////////////////////////////////////////////////////////////
// CAttackerApp:
// See Attacker.cpp for the implementation of this class
//

class CAttackerApp : public CWinApp
{
public:
	CAttackerApp();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAttackerApp)
	public:
	virtual BOOL InitInstance();
	//}}AFX_VIRTUAL

// Implementation

	//{{AFX_MSG(CAttackerApp)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_ATTACKER_H__0D5E761D_ED6E_498C_98BE_2EA72C6AA67D__INCLUDED_)
