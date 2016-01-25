// fginst_PackerDlg.cpp : implementation file
//

#include "stdafx.h"
#include "fginst_Packer.h"
#include "fginst_PackerDlg.h"
#include "io.h"
#include "rc4.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CFginst_PackerDlg dialog

CFginst_PackerDlg::CFginst_PackerDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CFginst_PackerDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CFginst_PackerDlg)
	m_csNote = _T("");
	//}}AFX_DATA_INIT
	// Note that LoadIcon does not require a subsequent DestroyIcon in Win32
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CFginst_PackerDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CFginst_PackerDlg)
	DDX_Text(pDX, IDC_EDIT1, m_csNote);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CFginst_PackerDlg, CDialog)
	//{{AFX_MSG_MAP(CFginst_PackerDlg)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_WM_TIMER()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CFginst_PackerDlg message handlers

BOOL CFginst_PackerDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon
	
	if(__argc == 2 && strcmpi(__argv[1], "-c") == 0){
		SetTimer(0, 300, NULL);
	}
	
	return TRUE;  // return TRUE  unless you set the focus to a control
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CFginst_PackerDlg::OnPaint() 
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
HCURSOR CFginst_PackerDlg::OnQueryDragIcon()
{
	return (HCURSOR) m_hIcon;
}

void CFginst_PackerDlg::OnCancel() 
{
	// TODO: Add extra cleanup here
	
	CDialog::OnCancel();
}

struct CONFIG_OPTION{
	char time_server[31];
	char file_hash[33];
	char filename_base[21];
	char filename_group[11];
	int link_count;
	unsigned int link_length;
}; 
 
void CFginst_PackerDlg::OnOK() 
{
	int i;
// 	char sPath[MAX_PATH];
// 	GetCurrentDirectory(MAX_PATH, sPath);
// 	if(sPath[strlen(sPath) - 1] != '\\'){
// 		strcat(sPath, "\\");
// 	}

	CString csDir = __argv[0];
	csDir = csDir.Left(csDir.ReverseFind('\\') + 1);
	SetCurrentDirectory(csDir);

	//m_csNote
	CString csIniPath = csDir;
	csIniPath += "config.ini";
	if(_access(csIniPath, 0) == -1){
		m_csNote = "文件config.txt不存在。请检查！";
		UpdateData(0);
		return ;
	}
	
	CONFIG_OPTION option;
	memset(&option, 0, sizeof(CONFIG_OPTION));

	char *bufdata = NULL, *bufoption = NULL;
	char sDatFile[MAX_PATH] = {0};
	char sOutFile[MAX_PATH] = {0};
	FILE *fp = NULL;

	GetPrivateProfileString("OPTIONS", "PACK_DATA", "", sDatFile, MAX_PATH, csIniPath);
	if(fp = fopen(sDatFile, "rb"), fp == NULL){
		m_csNote = "Data文件打开失败。请检查！";
		UpdateData(0);
		return ;
	}

	fseek(fp, 0L, 2);
	long length = ftell(fp);
	rewind(fp);

	bufdata = (char*)malloc(length + 1);
	if(!sDatFile) {
		m_csNote = "读取Data文件到内存失败。请检查！";
		UpdateData(0);
		fclose(fp);
		return ;
	}

	fread(bufdata, 1, length, fp);
	fclose(fp);
	fp = NULL;

	GetPrivateProfileString("OPTIONS", "OUT_FILENAME", "", sOutFile, MAX_PATH, csIniPath);
	GetPrivateProfileString("OPTIONS", "TIME_SERVER", "", option.time_server, 31, csIniPath);
	GetPrivateProfileString("OPTIONS", "FILE_HASH", "", option.file_hash, 33, csIniPath);
	CString csHash = option.file_hash;
	csHash.MakeUpper();
	strcpy(option.file_hash, csHash);

	GetPrivateProfileString("OPTIONS", "FILE_NAME_BASE", "", option.filename_base, 21, csIniPath);
	GetPrivateProfileString("OPTIONS", "FILE_NAME_GROUP", "", option.filename_group, 11, csIniPath);
	option.link_count = GetPrivateProfileInt("LINKS", "LINKS_COUNT", 0, csIniPath);
	if (option.link_count > 30){
		m_csNote = "LinkS_COUNT超过30。请检查！";
		UpdateData(0);
		free(bufdata);
		return ;
	}

	char sLine[1024];
	char sLinkTitle[10] = {0};
	for (i=0; i<option.link_count; i++){
		sprintf(sLinkTitle, "LINK%d", i+1);
		GetPrivateProfileString("LINKS", sLinkTitle, "", sLine, 1024, csIniPath);
		if(option.link_length < strlen(sLine)){
			option.link_length = strlen(sLine);
		}

// 		if(strstr(sLine, "://")){
// 			m_csNote = "Link不能包含\"://\", 请检查！";
// 			UpdateData(0);
// 			free(bufdata);
// 			return ;
// 		}
	}
	option.link_length ++; //Add a 0x00 at the eand of each link

	if(fp = fopen(sOutFile, "wb"), fp == NULL){
		m_csNote = "输出文件写失败。请检查！";
		UpdateData(0);
		free(bufdata);
		return ;
	}

	bufoption = (char*)&option;

	//Gen pass every packing 
	unsigned char sPassword[17] = {0};
	srand(time(NULL));
	for (i=0;i<17; i++) {
		sPassword[i] = rand()%256;
	}


	rc4_key key;
	prepare_key(sPassword, 16, &key);
	rc4((unsigned char *)bufoption, sizeof(CONFIG_OPTION), &key);

	fwrite(bufdata, 1, length, fp);
	fwrite(bufoption, 1, sizeof(CONFIG_OPTION), fp);

	//decryt it
	prepare_key(sPassword, 16, &key);
	rc4((unsigned char *)bufoption, sizeof(CONFIG_OPTION), &key);

	for (i=0; i<option.link_count; i++){
		sprintf(sLinkTitle, "LINK%d", i+1);
		memset(sLine, 0, 1024);
		GetPrivateProfileString("LINKS", sLinkTitle, "", sLine, 1024, csIniPath);

		sPassword[16] = i * 5;
		prepare_key(sPassword, 17, &key);
		rc4((unsigned char *)sLine, option.link_length, &key);
		fwrite(sLine, 1, option.link_length, fp);
	}

	//Save the password
	for(i=0; i<16; i++){
		sPassword[i] ^= i * 7;
	}
	fwrite(sPassword, 1, 16, fp);
	
	//Save the Datfile length and flag
	fwrite(&length, 1, 4, fp);
 	fwrite("\xa5\x5a\x87\x78", 1, 4, fp);
 	fclose(fp);

	m_csNote = "文件输出完成。";
	UpdateData(0);
	free(bufdata);

}


void CFginst_PackerDlg::OnTimer(UINT nIDEvent) 
{
	if(0 == nIDEvent){
		KillTimer(0);
		this->OnOK();
		Sleep(50);
		CDialog::OnCancel();
	}
	
	CDialog::OnTimer(nIDEvent);
}
