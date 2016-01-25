// fginst_PackerDlg.h : header file
//

#if !defined(AFX_FGINST_PACKERDLG_H__E6CFB210_C6DE_4E78_A1A0_B55D02AD6FAC__INCLUDED_)
#define AFX_FGINST_PACKERDLG_H__E6CFB210_C6DE_4E78_A1A0_B55D02AD6FAC__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

/////////////////////////////////////////////////////////////////////////////
// CFginst_PackerDlg dialog

class CFginst_PackerDlg : public CDialog
{
// Construction
public:
	CFginst_PackerDlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	//{{AFX_DATA(CFginst_PackerDlg)
	enum { IDD = IDD_FGINST_PACKER_DIALOG };
	CString	m_csNote;
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CFginst_PackerDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	//{{AFX_MSG(CFginst_PackerDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	virtual void OnOK();
	virtual void OnCancel();
	afx_msg void OnTimer(UINT nIDEvent);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_FGINST_PACKERDLG_H__E6CFB210_C6DE_4E78_A1A0_B55D02AD6FAC__INCLUDED_)
