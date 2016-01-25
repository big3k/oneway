// SynSenderConsole.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "SynSenderConsole.h"

#include "SpoofSocket.h"
#include "UDPSocket.h"
#include "TCPCrafter.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// The one and only application object

CWinApp theApp;

using namespace std;

void DisplaySocketError(CSpoofSocket *sock)
{
	//Display an error
	char* cErr;
	cErr=new char[10];
	itoa(sock->GetLastError(),cErr,10);

	char* cMsg;
	cMsg=new char[40];
	strcpy(cMsg,"Winsock error : ");
	strcat(cMsg,cErr);

	printf("%s\n", cMsg);

	delete cMsg;
	delete cErr;
}


void send(const char* sourceIP, const char* startDestIP, unsigned long numOfDestIPs, 
		  unsigned int port, unsigned int delayPerSendInMilliSecond, unsigned int numOfIPPerSend,
		  BOOL debug) 
{
	int nDestIP1 = 0, nDestIP2 = 0, nDestIP3 = 0, nDestIP4 = 0;
	unsigned long nCurrentDestIP = 0;

	CString ip = CString(startDestIP);
	int n1 = ip.Find(".", 1);
	nDestIP1 = atoi(ip.Left(n1));
	int n2 = ip.Find(".", n1 + 1);
	nDestIP2 = atoi(ip.Left(n2).Mid(n1 + 1));
	int n3 = ip.Find(".", n2 + 1);
	nDestIP3 = atoi(ip.Left(n3).Mid(n2 + 1));
	nDestIP4 = atoi(ip.Mid(n3 + 1));

	nCurrentDestIP = nDestIP4 + nDestIP3 * 256l + nDestIP2 * 65536l + nDestIP1 * 16777216l;

	printf ("%d.%d.%d.%d\n", nDestIP1, nDestIP2, nDestIP3, nDestIP4);
//	return;

	if (!CSpoofBase::InitializeSockets())
	{
		printf ("Initializing socket failed.\n");
	} else {
		printf("Initializing socks OK.\n");
	}
	
	//Create the tcp socket
	CTCPCrafter* tcp;
	tcp=new CTCPCrafter();
	unsigned long numIPChecked = 0;

	printf("Crafter is created.\n");
	//Was an error
	BOOL bError=TRUE;
	tcp->SetRaw(TRUE);
	if (tcp->Create())
	{
		printf("TCP create OK.\n");
		bError=FALSE;
		char* cDestinationIP;

		bError=TRUE;

		//Let's send
		tcp->SetSourceAddress(sourceIP);
		printf("Set source address.\n");
		tcp->Bind(sourceIP);
		printf("Bind.\n");

		//set TCP options
		tcp->SetTCPOptions(TRUE);
		//the following options cannot be used at the same time.
//				tcp->GetTCPOptions()->AddOption_Nothing();
//				tcp->GetTCPOptions()->AddOption_ENDLIST();
		tcp->GetTCPOptions()->AddOption_SegmentSize(0x05b4);

		tcp->SetFlags(TCPFlag_RST);
		tcp->SetTTL(111);
		cDestinationIP = new char[16];
		unsigned short nSourcePort;

		numIPChecked = 0;
		nSourcePort = 1024;
		printf("Start sending ....\n");
		while (numIPChecked < numOfDestIPs)
		{
			numIPChecked++;

			nDestIP1 = nCurrentDestIP / 16777216;
			nDestIP2 = (nCurrentDestIP % 16777216) / 65536;
			nDestIP3 = (nCurrentDestIP % 65536) / 256;
			nDestIP4 = nCurrentDestIP % 256;
			nCurrentDestIP++;

			bError=TRUE;
			nSourcePort++;
			int randnum = rand();
			nSourcePort = nSourcePort % 64509 + 1025;
			tcp->SetSequenceNumber(randnum * randnum);

			wsprintf(cDestinationIP, "%d.%d.%d.%d", nDestIP1, nDestIP2, nDestIP3, nDestIP4);
			//check port 
			if (debug) printf("Connecting ...... ");
			if (tcp->Connect(nSourcePort, cDestinationIP, port)) 
			{
				//OK
				bError=FALSE;
				if (debug) {
					printf("%d.%d.%d.%d OK\n", nDestIP1, nDestIP2, nDestIP3, nDestIP4);
				}
			} else 
			{
				if (debug) {
					printf("%d.%d.%d.%d failed\n", nDestIP1, nDestIP2, nDestIP3, nDestIP4);
				}
			}
			if ((numIPChecked % (numOfIPPerSend * 1) == 0) && (numIPChecked > 0))
			{
				Sleep(delayPerSendInMilliSecond); //delay for a while
				if (debug) {
					printf ("Sleeping ...\n");
				}
			}
		}
		delete cDestinationIP;
	} else {
		printf("TCP create failed.\n");
	}

	if (bError)
		//Display error
		DisplaySocketError(tcp);

	tcp->Close();
	delete tcp;
}

void printUsage() 
{
	printf("-s source_ip -d start_destination_ip -n number_of_destination_ip \n");
	printf("-p destination_port -l delay_per_send_in_milli_seconds \n");
	printf("-c number_of_ip_to_send_between_each_delay\n");
	printf("-D debug_flag (true|false)\n");
}

int _tmain(int argc, TCHAR* argv[], TCHAR* envp[])
{
	int nRetCode = 0;

	// initialize MFC and print and error on failure
	if (!AfxWinInit(::GetModuleHandle(NULL), NULL, ::GetCommandLine(), 0))
	{
		// TODO: change error code to suit your needs
		cerr << _T("Fatal Error: MFC initialization failed") << endl;
		nRetCode = 1;
	}
	else
	{
		if (argc != 15) {
			printUsage();
			return 0;
		} else {
			char* sourceip, *destip;
			unsigned long numofdestip;
			unsigned int port, delaypersendinmillisecond, numofippersend;
			sourceip = new char[30];
			destip = new char[30];
			BOOL debug = FALSE;

			for (int i = 1; i < 15; i += 2) {
				if (strcmp(argv[i], "-s") == 0) wsprintf(sourceip, "%s", argv[i + 1]);
				else if (strcmp(argv[i], "-d") == 0) wsprintf(destip, "%s", argv[i + 1]);
				else if (strcmp(argv[i], "-n") == 0) numofdestip = atol(argv[i + 1]);
				else if (strcmp(argv[i], "-p") == 0) port = atol(argv[i + 1]);
				else if (strcmp(argv[i], "-l") == 0) delaypersendinmillisecond = atol(argv[i + 1]);
				else if (strcmp(argv[i], "-c") == 0) numofippersend = atol(argv[i + 1]);
				else if (strcmp(argv[i], "-D") == 0) debug = !strcmp(argv[i + 1], "true");
				else {
					printUsage();
					return 0;
				}
					
			}
			send(sourceip, destip, numofdestip, port, delaypersendinmillisecond, numofippersend, debug);
		}
	}

	return nRetCode;
}


