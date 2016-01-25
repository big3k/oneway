
#include "stdafx.h"
#include "time.h"

unsigned long gRunTime = 100000; //100 sec
unsigned long gSpeedLimit = 150;

CNdisApi api;
TCP_AdapterList	AdList;
int iIndex = -1;
HANDLE hEvent = NULL;
PSTATIC_FILTER_TABLE pFilters = NULL;

typedef struct {
 	unsigned long remote_ip;
 	unsigned short remote_port;
 	unsigned short local_port;
	unsigned long tm;
} PacketInfo;

CTypedPtrList<CPtrList, PacketInfo*> gPacketQueue;

//check the time diff of the SYN and SYN_ACK, if less than minDuration, then bDrop is true
static BOOL PackCmpAndRemoveFromList(PacketInfo* p)
{
	//printf("Examine: %s:%d\n", inet_ntoa(*(struct in_addr*)&p->remote_ip), p->remote_port);
	CString msg;
	BOOL bDrop = FALSE;
	unsigned long tick = GetTickCount();
	PacketInfo* pPak = NULL;
	POSITION pos1, pos2;
    for (pos1 = gPacketQueue.GetHeadPosition(); (pos2 = pos1) != NULL;) {
		pPak = gPacketQueue.GetNext(pos1);
        if (tick - pPak->tm > gSpeedLimit) {
			//no longer monitor this packet
            gPacketQueue.RemoveAt(pos2);
			//printf("Deregister: %s:%d\n", inet_ntoa(*(struct in_addr*)&pPak->remote_ip), pPak->remote_port);
			delete pPak;
			continue;
		} else {
			//if we have a match
			if ((p->local_port == pPak->local_port) &&
				(p->remote_ip == pPak->remote_ip) &&
				(p->remote_port == pPak->remote_port)) {
				bDrop = TRUE;
#ifdef _DEBUG
				printf("Drop: %s:%d time %dms\n", inet_ntoa(*(struct in_addr*)&pPak->remote_ip), pPak->remote_port, tick - pPak->tm);
#endif
			}
		}
	}
	return bDrop;	
}

static int InitWinsockDll()
{
	WORD wVersionRequested;
	WSADATA wsaData;
	int err; 
	wVersionRequested = MAKEWORD(2, 2); 
	err = WSAStartup(wVersionRequested, &wsaData);
	if (err != 0) 
		return (err);
	
	/* Confirm that the WinSock DLL supports 2.2.        */
	/* Note that if the DLL supports versions greater    */
	/* than 2.2 in addition to 2.2, it will still return */
	/* 2.2 in wVersion since that is the version we      */
	/* requested.                                        */ 
	if (LOBYTE(wsaData.wVersion) != 2 ||
		HIBYTE(wsaData.wVersion) != 2) 
	{
		WSACleanup();
		return (-1); 
	}
	
	return (0);
	/* The WinSock DLL is acceptable. Proceed. */ 
}

// This function releases packets in the adapter queue and stops listening the interface
static void ReleaseInterface()
{
	HANDLE handle = (HANDLE)AdList.m_nAdapterHandle[iIndex];

	// Set NULL event to release previously set event object
	api.SetPacketEvent(handle, NULL);

	// Close Event
	if (hEvent) {
		CloseHandle ( hEvent );
		hEvent = NULL;
	}

	// Set default adapter mode
	ADAPTER_MODE Mode;
	Mode.dwFlags = 0;
	Mode.hAdapterHandle = handle;
	api.SetAdapterMode(&Mode);

	// Empty adapter packets queue
	api.FlushAdapterPacketQueue (handle);

	if (pFilters) {
		free (pFilters);
		pFilters = NULL;
	}
}

static void SetupFilter() {
	// Allocate table filters for 10 filters
	DWORD dwTableSize = sizeof(STATIC_FILTER_TABLE) + sizeof(STATIC_FILTER)*9;
	pFilters = (PSTATIC_FILTER_TABLE)malloc(dwTableSize);
	memset (pFilters, 0, dwTableSize);

	pFilters->m_TableSize = 3;

	//**************************************************************************************
	// 1. Outgoing TCP requests filter: REDIRECT OUT TCP packets with destination PORT [1, 65535]
	// Common values
	pFilters->m_StaticFilters[0].m_Adapter.QuadPart = 0; // applied to all adapters
	pFilters->m_StaticFilters[0].m_ValidFields = NETWORK_LAYER_VALID | TRANSPORT_LAYER_VALID;
	pFilters->m_StaticFilters[0].m_FilterAction = FILTER_PACKET_REDIRECT;
	pFilters->m_StaticFilters[0].m_dwDirectionFlags = PACKET_FLAG_ON_SEND;

	// Network layer filter
	pFilters->m_StaticFilters[0].m_NetworkFilter.m_dwUnionSelector = IPV4; 
	pFilters->m_StaticFilters[0].m_NetworkFilter.m_IPv4.m_ValidFields = IP_V4_FILTER_PROTOCOL;
	pFilters->m_StaticFilters[0].m_NetworkFilter.m_IPv4.m_Protocol = IPPROTO_TCP;

	//****************************************************************************************
	// 2. Incoming TCP responses filter: REDIRECT IN TCP packets with source PORT [1, 65536]
	// Common values
	pFilters->m_StaticFilters[1].m_Adapter.QuadPart = 0; // applied to all adapters
	pFilters->m_StaticFilters[1].m_ValidFields = NETWORK_LAYER_VALID | TRANSPORT_LAYER_VALID;
	pFilters->m_StaticFilters[1].m_FilterAction = FILTER_PACKET_REDIRECT;
	pFilters->m_StaticFilters[1].m_dwDirectionFlags = PACKET_FLAG_ON_RECEIVE;

	// Network layer filter
	pFilters->m_StaticFilters[1].m_NetworkFilter.m_dwUnionSelector = IPV4; 
	pFilters->m_StaticFilters[1].m_NetworkFilter.m_IPv4.m_ValidFields = IP_V4_FILTER_PROTOCOL;
	pFilters->m_StaticFilters[1].m_NetworkFilter.m_IPv4.m_Protocol = IPPROTO_TCP;

	//***************************************************************************************
	// 3. Pass all packets (skipped by previous filters) without processing in user mode
	// Common values
	pFilters->m_StaticFilters[2].m_Adapter.QuadPart = 0; // applied to all adapters
	pFilters->m_StaticFilters[2].m_ValidFields = 0;
	pFilters->m_StaticFilters[2].m_FilterAction = FILTER_PACKET_PASS;
	pFilters->m_StaticFilters[2].m_dwDirectionFlags = PACKET_FLAG_ON_RECEIVE | PACKET_FLAG_ON_SEND;

	api.SetPacketFilterTable(pFilters);
}

// pass in a const char*.
static BOOL TestTcpConnection()
{
	SOCKET fd = socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
	if(fd == INVALID_SOCKET) {
		printf ("Invalid socket %d.\n", WSAGetLastError());
		return FALSE;
	}

	//nonblocking and nodelay
	unsigned long nonblock = 1;
	long nodelay = 1;
	ioctlsocket(fd, FIONBIO, &nonblock);
	setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, (const char *)&nodelay, sizeof(int));

	srand(time(NULL));
	char ipAddr[16];
	sprintf(ipAddr, "202.%d.%d.%d", (rand() & 0xFF), (rand() & 0xFF), (rand() & 0xFF));

	struct sockaddr_in server;
	server.sin_addr.s_addr = inet_addr(ipAddr);
	server.sin_family=AF_INET;
	server.sin_port=htons(80);

	//we do not care result, just send out
	connect(fd, (struct sockaddr*)&server, sizeof(server));
	closesocket(fd);
	printf ("Sent packet to %s\n", ipAddr);
	return TRUE;
}

static BOOL isActiveAdaptor(int index) 
{
	HANDLE handle = (HANDLE)(AdList.m_nAdapterHandle[index]);

	// Create notification event
	hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

	// Set event for helper driver
	if ((!hEvent)||(!api.SetPacketEvent(handle, hEvent)))
	{
		printf ("Failed to create notification event or set it for driver.\n");
		return FALSE;
	}

	ADAPTER_MODE Mode;
	Mode.dwFlags = MSTCP_FLAG_SENT_LISTEN | MSTCP_FLAG_RECV_LISTEN;
	Mode.hAdapterHandle = handle;
	api.SetAdapterMode(&Mode);

	SetupFilter();

	// Initialize Request
	ETH_REQUEST			Request;
	INTERMEDIATE_BUFFER PacketBuffer;
	ZeroMemory ( &Request, sizeof(ETH_REQUEST) );
	ZeroMemory ( &PacketBuffer, sizeof(INTERMEDIATE_BUFFER) );
	Request.EthPacket.Buffer = &PacketBuffer;
	Request.hAdapterHandle = handle;

	TestTcpConnection();

	//detect if any tcp packet gets caught
	time_t start = time(NULL);
	BOOL gotPackets = FALSE;
	while ((!gotPackets) && (time(NULL) - start <= 3))
	{
		if (WaitForSingleObject ( hEvent, 1000 ) != WAIT_OBJECT_0) {
			continue;
		}
				
		while(api.ReadPacket(&Request))
		{
			ether_header* pEtherHdr = (ether_header_ptr)PacketBuffer.m_IBuffer;

			if (ntohs(pEtherHdr->h_proto) == ETH_P_IP)
			{
				iphdr_ptr pIpHdr = (iphdr*)(PacketBuffer.m_IBuffer + sizeof(ether_header));

				if (pIpHdr->ip_p == IPPROTO_TCP)
				{
					printf("Seen packet %s\n", 
						(PacketBuffer.m_dwDeviceFlags == PACKET_FLAG_ON_SEND)? "out" : "in");
					gotPackets = TRUE;
					break;
				}
			}			
		}
		ResetEvent(hEvent);
	}

	//release interface
	ReleaseInterface();

	return gotPackets;
}

/**
Detect which adaptor is active. The result will be in iIndex. If not found, iIndex will be -1.
*/
static int detectActiveAdaptor()
{
	iIndex = -1;
	for (UINT i = 0; i < AdList.m_nAdapterCount; ++i)
	{
		if (isActiveAdaptor(i))
		{
			printf("* Internal Name:\t %s\n", AdList.m_szAdapterNameList[i]);
			iIndex = i;
			break;
		} else {
			printf("  Internal Name:\t %s\n", AdList.m_szAdapterNameList[i]);
		}
	}
	return iIndex;
}

int main()
{
	InitWinsockDll();

	if(!api.IsDriverLoaded())
	{
		printf ("Driver not installed on this system of failed to load.\n");
		return 0;
	}

	//
	// Get system installed network interfaces
	//
	api.GetTcpipBoundAdaptersInfo (&AdList);

	iIndex = detectActiveAdaptor();
	printf("find active adapater index %d\n", iIndex);

	HANDLE handle = (HANDLE)(AdList.m_nAdapterHandle[iIndex]);

	//
	// Create notification events and initialize the driver to pass packets thru us
	//
	hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	if (!hEvent)
	{
		printf("Failed to create notification event for network interface \n");
		return 0;
	}

	//
	// Set packet notification event for the network interface
	//
	api.SetPacketEvent(handle, hEvent);

	//
	// Initialize common ADAPTER_MODE structure (all network interfaces will operate in the same mode)
	//
	ADAPTER_MODE Mode;
	Mode.dwFlags = MSTCP_FLAG_SENT_TUNNEL|MSTCP_FLAG_RECV_TUNNEL;
	Mode.hAdapterHandle = handle;

	//
	// Set MSTCP_FLAG_SENT_TUNNEL|MSTCP_FLAG_RECV_TUNNEL for the network interface
	//
	api.SetAdapterMode(&Mode);

	SetupFilter();

	atexit (ReleaseInterface);
	
	// Initialize common part of ETH_REQUEST
	ETH_REQUEST			Request;
	INTERMEDIATE_BUFFER PacketBuffer;
	ZeroMemory ( &Request, sizeof(ETH_REQUEST) );
	ZeroMemory ( &PacketBuffer, sizeof(INTERMEDIATE_BUFFER) );
	Request.EthPacket.Buffer = &PacketBuffer;
	Request.hAdapterHandle = handle;

	gPacketQueue.RemoveAll();
	PacketInfo* pPak = NULL;
	PacketInfo packet;

	//
	// Go into the endless loop (this is just a sample application)
	//
	ether_header_ptr	pEthHeader = NULL;
	iphdr_ptr			pIpHeader = NULL;
	tcphdr_ptr			pTcpHeader = NULL;
	BOOL				bDrop = FALSE;
	unsigned long start = GetTickCount();
	while (GetTickCount() - start < gRunTime)
	{
		//
		// Wait before any of the interfaces is ready to indicate the packet
		//
		if (WaitForSingleObject ( hEvent, 1000 ) != WAIT_OBJECT_0) {
			continue;
		}
		
		//
		// Read packet from the interface until there are any
		//
		while(api.ReadPacket(&Request))
		{
			//
			// Get Ethernet header
			//
			pEthHeader = (ether_header_ptr)PacketBuffer.m_IBuffer;
			
			//
			// Check if Ethernet frame contains IP packet
			//
			if(ntohs(pEthHeader->h_proto) == ETH_P_IP)
			{
				bDrop = FALSE;
				//
				// Get IP header
				//
				pIpHeader = (iphdr_ptr)(pEthHeader + 1);

				//
				// Check if IP packet contains TCP packet
				//
				if (pIpHeader->ip_p == IPPROTO_TCP)
				{
					//
					// Get TCP header pointer
					//
					pTcpHeader = (tcphdr_ptr)((PUCHAR)pIpHeader + pIpHeader->ip_hl*4);

					if(pTcpHeader->th_flags == TH_SYN && PacketBuffer.m_dwDeviceFlags == PACKET_FLAG_ON_SEND){
						pPak = new PacketInfo();
						pPak->remote_ip = pIpHeader->ip_dst.S_un.S_addr;
 						pPak->remote_port = ntohs (pTcpHeader->th_dport);
 						pPak->local_port = ntohs (pTcpHeader->th_sport);
						pPak->tm = GetTickCount();
						//printf("Register node: %s:%d\n", inet_ntoa(*(struct in_addr*)&pPak->remote_ip), pPak->remote_port);
						gPacketQueue.AddTail(pPak);
					}

					else if((pTcpHeader->th_flags == (TH_SYN | TH_ACK)) &&
						(PacketBuffer.m_dwDeviceFlags == PACKET_FLAG_ON_RECEIVE)){
 						packet.remote_ip = pIpHeader->ip_src.S_un.S_addr;
 						packet.remote_port = ntohs (pTcpHeader->th_sport);
 						packet.local_port = ntohs (pTcpHeader->th_dport);
						packet.tm = GetTickCount();
						bDrop = PackCmpAndRemoveFromList(&packet);
					}
				}
			}

			if(!bDrop) {
				if (PacketBuffer.m_dwDeviceFlags == PACKET_FLAG_ON_SEND) {
					// Place packet on the network interface
					api.SendPacketToAdapter(&Request);
				} else {
					// Indicate packet to MSTCP
					api.SendPacketToMstcp(&Request);
				}
			}
		}

		//
		// Reset signalled event
		//
		ResetEvent(hEvent);
	}

	WSACleanup();

	//release interface
	ReleaseInterface();

	return 0;
}

