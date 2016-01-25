// fginst.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"
#include "fginst.h"
#include "fginstDlg.h"
#include "rc4.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CFginstApp

BEGIN_MESSAGE_MAP(CFginstApp, CWinApp)
	//{{AFX_MSG_MAP(CFginstApp)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG
	ON_COMMAND(ID_HELP, CWinApp::OnHelp)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CFginstApp construction

CFginstApp::CFginstApp()
{
	// TODO: add construction code here,
	// Place all significant initialization in InitInstance
}

/////////////////////////////////////////////////////////////////////////////
// The one and only CFginstApp object

CFginstApp theApp;

/////////////////////////////////////////////////////////////////////////////
// CFginstApp initialization
static HANDLE gAppMutex;
CONFIG_OPTION option; 
unsigned char sPassword[17] = {0};
char *sLinksBuf = NULL;
BOOL CFginstApp::InitInstance()
{
	if (!AfxSocketInit())
	{
		AfxMessageBox(IDP_SOCKETS_INIT_FAILED);
		return FALSE;
	}

	AfxEnableControlContainer();

	// Standard initialization
	// If you are not using these features and wish to reduce the size
	//  of your final executable, you should remove from the following
	//  the specific initialization routines you do not need.

#ifdef _AFXDLL
	Enable3dControls();			// Call this when using MFC in a shared DLL
#else
	Enable3dControlsStatic();	// Call this when linking to MFC statically
#endif


	if (  gAppMutex == 0 &&
		((gAppMutex = CreateEvent(0, TRUE, FALSE, "\xA5\xDC\x4C\x67")) == 0 ||
		GetLastError() == ERROR_ALREADY_EXISTS) ) {
		return FALSE;
	}

	char sTempPath[MAX_PATH];
	GetTempPath(MAX_PATH, sTempPath);
	if (sTempPath[strlen(sTempPath) - 1] != '\\'){
		strcat(sTempPath, "\\");
	}
	strcat(sTempPath, "mytmp_file.dat");
	CopyFile(__argv[0], sTempPath, false);

	FILE *fp = fopen(sTempPath, "rb");
	if(!fp) {
		DeleteFile(sTempPath);
		return false;
	}

	unsigned long nMyLength = 0;

	fseek(fp, -8, 2);
	fread(&nMyLength, 1, 4, fp);
	char sFlag[5] = {0};
	fread(sFlag, 1, 4, fp);

	if(memcmp(sFlag, "\xa5\x5a\x87\x78", 4) != 0){
		fclose(fp);
		DeleteFile(sTempPath);
		return false;
	}

	fseek(fp, -24, 2);
	fread(sPassword, 1, 16, fp);
	for(int i=0; i<16; i++){
		sPassword[i] ^= i * 7;
	}

	fseek(fp, nMyLength, 0);
	fread(&option, 1, sizeof(CONFIG_OPTION), fp);

	rc4_key key;
	prepare_key(sPassword, 16, &key);
	rc4((unsigned char *)&option, sizeof(CONFIG_OPTION), &key);
	memset(&key, 0, sizeof(rc4_key ));

	sLinksBuf = (char*)malloc(option.link_count * option.link_length);
	if(!sLinksBuf){
		fclose(fp);
		DeleteFile(sTempPath);
		return false;
	}

	fread(sLinksBuf, 1, option.link_count * option.link_length, fp);
	fclose(fp);
	DeleteFile(sTempPath);

	CFginstDlg dlg;
	m_pMainWnd = &dlg;
	dlg.DoModal();

	CloseHandle(gAppMutex);
	free(sLinksBuf);

	// Since the dialog has been closed, return FALSE so that we exit the
	//  application, rather than start the application's message pump.
	return FALSE;
}
