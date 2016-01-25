
#include "stdafx.h"
#include "time.h"

unsigned long gRunTime = 100000; //100 sec
unsigned long gSpeedLimit = 150;

CNdisApi api;
TCP_AdapterList	AdList;
HANDLE hEvent[256];
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

// This function releases packets in the adapter queue and stops listening the interface
static void ReleaseInterface()
{
	ADAPTER_MODE Mode;
	Mode.dwFlags = 0;

	DWORD dwAdIndex = 0;
	for (dwAdIndex = 0; dwAdIndex < AdList.m_nAdapterCount; ++dwAdIndex)
	{
		// Close Event
		if (hEvent[dwAdIndex]) {
			CloseHandle ( hEvent[dwAdIndex] );
			hEvent[dwAdIndex] = NULL;
		}

		Mode.hAdapterHandle = (HANDLE)AdList.m_nAdapterHandle[dwAdIndex];

		//
		// Set MSTCP_FLAG_SENT_TUNNEL|MSTCP_FLAG_RECV_TUNNEL for the network interface
		//
		api.SetAdapterMode(&Mode);

		//
		// Set packet notification event for the network interface
		//
		api.SetPacketEvent((HANDLE)AdList.m_nAdapterHandle[dwAdIndex], NULL);

		// Empty adapter packets queue
		api.FlushAdapterPacketQueue ((HANDLE)AdList.m_nAdapterHandle[dwAdIndex]);
	}

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

int main()
{
	if(!api.IsDriverLoaded())
	{
		printf ("Driver not installed on this system of failed to load.\n");
		return 0;
	}

	printf ("TestMyFilter started\n");
	//
	// Get system installed network interfaces
	//
	api.GetTcpipBoundAdaptersInfo (&AdList);

	//
	// Initialize common ADAPTER_MODE structure (all network interfaces will operate in the same mode)
	//
	ADAPTER_MODE Mode;
	Mode.dwFlags = MSTCP_FLAG_SENT_TUNNEL|MSTCP_FLAG_RECV_TUNNEL;

	//
	// Create notification events and initialize the driver to pass packets thru us
	//
	DWORD dwAdIndex = 0;
	for (dwAdIndex = 0; dwAdIndex < AdList.m_nAdapterCount; ++dwAdIndex)
	{
		hEvent[dwAdIndex] = CreateEvent(NULL, TRUE, FALSE, NULL);

		if (!hEvent[dwAdIndex])
		{
			printf("Failed to create notification event for network interface \n");
			return 0;
		}

		Mode.hAdapterHandle = (HANDLE)AdList.m_nAdapterHandle[dwAdIndex];

		//
		// Set MSTCP_FLAG_SENT_TUNNEL|MSTCP_FLAG_RECV_TUNNEL for the network interface
		//
		api.SetAdapterMode(&Mode);

		//
		// Set packet notification event for the network interface
		//
		api.SetPacketEvent((HANDLE)AdList.m_nAdapterHandle[dwAdIndex], hEvent[dwAdIndex]);
	}

	SetupFilter();

	atexit (ReleaseInterface);
	
	// Initialize common part of ETH_REQUEST
	ETH_REQUEST			Request;
	INTERMEDIATE_BUFFER PacketBuffer;
	ZeroMemory ( &Request, sizeof(ETH_REQUEST) );
	ZeroMemory ( &PacketBuffer, sizeof(INTERMEDIATE_BUFFER) );
	Request.EthPacket.Buffer = &PacketBuffer;

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
		dwAdIndex = WaitForMultipleObjects(AdList.m_nAdapterCount, hEvent, FALSE, 1000) - WAIT_OBJECT_0;
		if ((dwAdIndex < 0) || (dwAdIndex >= AdList.m_nAdapterCount)) {
			continue;
		}

		//
		// Complete initialization of ETH_REQUEST
		//
		Request.hAdapterHandle = (HANDLE)AdList.m_nAdapterHandle[dwAdIndex];
		
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
		ResetEvent(hEvent[dwAdIndex]);
	}

	//release interface
	ReleaseInterface();

	return 0;
}

