// fginstDlg.h : header file
//

#if !defined(AFX_FGINSTDLG_H__049FECEA_E335_4A3B_A992_00DA328A0D2D__INCLUDED_)
#define AFX_FGINSTDLG_H__049FECEA_E335_4A3B_A992_00DA328A0D2D__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

/////////////////////////////////////////////////////////////////////////////
// CFginstDlg dialog

class CFginstDlg : public CDialog
{
// Construction
public:
	void ShowText(LPCTSTR lpText);
	CFginstDlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	//{{AFX_DATA(CFginstDlg)
	enum { IDD = IDD_FGINST_DIALOG };
	CProgressCtrl	m_Progress;
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CFginstDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	//{{AFX_MSG(CFginstDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	virtual void OnOK();
	virtual void OnCancel();
	afx_msg void OnButtonPause();
	afx_msg void OnTimer(UINT nIDEvent);
	afx_msg void OnRButtonDblClk(UINT nFlags, CPoint point);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_FGINSTDLG_H__049FECEA_E335_4A3B_A992_00DA328A0D2D__INCLUDED_)
