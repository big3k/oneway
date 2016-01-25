// AttackerDlg.h : header file
//

#if !defined(AFX_ATTACKERDLG_H__8456DC89_947E_41AF_9892_DA13C972DBF4__INCLUDED_)
#define AFX_ATTACKERDLG_H__8456DC89_947E_41AF_9892_DA13C972DBF4__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

/////////////////////////////////////////////////////////////////////////////
// CAttackerDlg dialog

#include <afxmt.h>
#include "HyperLink.h"


#define ERROR_INVALID_SOURCE "Invalid source IP address"
#define ERROR_INVALID_DESTINATION "Invalid destination IP address"

class CSpoofSocket;

class CAttackerDlg : public CDialog
{
	static BOOL m_bStop;
	static CCriticalSection	m_critStop;

// Construction
public:
	CAttackerDlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	//{{AFX_DATA(CAttackerDlg)
	enum { IDD = IDD_ATTACKER_DIALOG };
	CIPAddressCtrl	m_DestIP;
	CIPAddressCtrl	m_SourceIP;
	BOOL	m_TcpOptions;
	long	m_nNumOfIPToCheck;
	BOOL	m_bIPReverse;
	int		m_nDelayInMiliSecondPer1016;
	CString	m_sImportFileName;
	CString	m_nIPChecked;
	int		m_bManually;
	CString	m_sPortToCheck;
	int		m_nNumOfIPPerSend;
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAttackerDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	void DisplaySocketError(CSpoofSocket* sock);
	HICON m_hIcon;

	// Generated message map functions
	//{{AFX_MSG(CAttackerDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnSend();
	afx_msg void OnQuit();
	afx_msg void OnRADIOmanual();
	afx_msg void OnRADIOfile();
	afx_msg void OnBUTTONbrowse();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
private:
	unsigned long m_nCurrentIP;
	int m_nCurrentDestPort;
	BYTE m_nDestIP4;
	BYTE m_nDestIP3;
	BYTE m_nDestIP2;
	BYTE m_nDestIP1;
	BYTE m_nSourceIP4;
	BYTE m_nSourceIP3;
	BYTE m_nSourceIP2;
	BYTE m_nSourceIP1;
	LPSTR IPCtrlToSTR(CIPAddressCtrl* ctrl);
	void SynFloodManually(int porttocheck);
	void SynFloodUsingFile(int porttocheck);
	void saveIPs();
	void initIPs();
	BOOL generateNextIP();

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_ATTACKERDLG_H__8456DC89_947E_41AF_9892_DA13C972DBF4__INCLUDED_)
