/*************************************************************************/
/*				Copyright (c) 2000-2009 NT Kernel Resources.		     */
/*                           All Rights Reserved.                        */
/*                          http://www.ntkernel.com                      */
/*                           ndisrd@ntkernel.com                         */
/*                                                                       */
/* Module Name:  filter.cpp                                            */
/*                                                                       */
/* Abstract: Defines the entry point for the console application         */
/*                                                                       */
/*************************************************************************/

#include "stdafx.h"

#pragma warning(disable: 4786) // this is a known bug that 4786 cannot be disabled.

TCP_AdapterList		AdList;
DWORD				iIndex;
CNdisApi			api;
ETH_REQUEST			Request;
INTERMEDIATE_BUFFER PacketBuffer;
HANDLE				hEvent;
PSTATIC_FILTER_TABLE pFilters = NULL;

unsigned char opt[] = "\x02\x04\x04\xec\x01\x01\x04\x02"; 

// Contains node IP addresses represented by u_long/in_addr.S_un.S_addr
// Maybe we need to consider synchronization if nodes are updated
// by different threads.
set<u_long>			nodes;
// Contains ports which have been used to send out SYN packets but
// these ports have not received SYN/ACK.
// Do we need to timeout ports for performance concern??? Maybe not.
set<u_short>		portsWithSynOnly;

list<syn_request>	synRequests;
// If the interval between a syn-ack and its syn is less than this value, we will drop the
// syn-ack packet. We need set a reasonable value for this variable, 300???
USHORT				minSynAndSynAckInterval = 300;

#ifndef WINSOCK2
USHORT htons( USHORT hostshort )
{
	PUCHAR	pBuffer;
	USHORT	nResult;

	nResult = 0;
	pBuffer = (PUCHAR )&hostshort;

	nResult = ( (pBuffer[ 0 ] << 8) & 0xFF00 )
		| ( pBuffer[ 1 ] & 0x00FF );

	return( nResult );
}

ULONG htonl( ULONG hostlong )
{
	ULONG    nResult = hostlong >> 16;
	USHORT	upper = (USHORT) nResult & 0x0000FFFF;
	USHORT	lower = (USHORT) hostlong & 0x0000FFFF;

	upper = htons( upper );
	lower = htons( lower );

    nResult = 0x10000 * lower + upper;
	return( nResult );
}

#define ntohs(X) htons(X)
#define ntohl(X) htonl(X)
#endif

//
// Function recalculates IP checksum
//
VOID RecalculateIPChecksum (iphdr_ptr pIpHeader)
{
	unsigned short word16;
	unsigned int sum = 0;
	unsigned int i = 0;
	PUCHAR buff;

	// Initialize checksum to zero
	pIpHeader->ip_sum = 0;
	buff = (PUCHAR)pIpHeader;

	// Calculate IP header checksum
	for (i = 0; i < pIpHeader->ip_hl*sizeof(DWORD); i=i+2)
	{
		word16 = ((buff[i]<<8)&0xFF00)+(buff[i+1]&0xFF);
		sum = sum+word16; 
	}

	// keep only the last 16 bits of the 32 bit calculated sum and add the carries
    while (sum>>16)
		sum = (sum & 0xFFFF)+(sum >> 16);

	// Take the one's complement of sum
	sum = ~sum;

	pIpHeader->ip_sum = htons((unsigned short) sum);
}

USHORT GetChecksum(void* buf, int size)
//Ref:http://hi.bccn.net/space-112902-do-blog-id-12121.html
{
	USHORT* buffer = (USHORT*)buf;
	unsigned long cksum = 0;
	while (size>1) {
		cksum += *buffer++;
		size -= sizeof(USHORT);
	}
	if (size) {
		cksum += *(UCHAR *)buffer;
	}
	while (cksum >> 16) {
		cksum = (cksum >> 16) + (cksum & 0xffff);
	}
	return (USHORT)(~cksum);
}

USHORT GetTcpChecksum(iphdr_ptr ip, tcphdr_ptr tcp)
{
	struct pseudo_header *header;
	USHORT tcp_length;
	USHORT result;
	unsigned char* buffer;
	

	tcp->th_sum = 0;
	tcp_length = ntohs(ip->ip_len) - sizeof(struct iphdr);

	buffer = (unsigned char*)new char[sizeof(struct pseudo_header) + tcp_length];

	header = (struct pseudo_header *)buffer;
	header->source_address = ip->ip_src;
	header->dest_address = ip->ip_dst;
	header->placeholder = 0;
	header->protocol = 0x06;	//TCP
	header->tcp_length = htons(tcp_length);

	memcpy(buffer + sizeof(struct pseudo_header), tcp, tcp_length);
	result = GetChecksum(buffer, sizeof(struct pseudo_header) + tcp_length);
	
	delete[] buffer;
	return result;
}

void ReleaseInterface()
{
	// This function releases packets in the adapter queue and stops listening the interface
	ADAPTER_MODE Mode;

	Mode.dwFlags = 0;
	Mode.hAdapterHandle = (HANDLE)AdList.m_nAdapterHandle[iIndex];

	// Set NULL event to release previously set event object
	api.SetPacketEvent(AdList.m_nAdapterHandle[iIndex], NULL);

	// Close Event
	if (hEvent)
		CloseHandle ( hEvent );

	// Set default adapter mode
	api.SetAdapterMode(&Mode);

	// Empty adapter packets queue
	api.FlushAdapterPacketQueue (AdList.m_nAdapterHandle[iIndex]);

	if (pFilters)
		free (pFilters);
}

void setupFilter() {
	// Allocate table filters for 10 filters
	DWORD dwTableSize = sizeof(STATIC_FILTER_TABLE) + sizeof(STATIC_FILTER)*9;
	pFilters = (PSTATIC_FILTER_TABLE)malloc(dwTableSize);
	memset (pFilters, 0, dwTableSize);

	{
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

		// Transport layer filter 
		pFilters->m_StaticFilters[0].m_TransportFilter.m_dwUnionSelector = TCPUDP;
		pFilters->m_StaticFilters[0].m_TransportFilter.m_TcpUdp.m_ValidFields = TCPUDP_DEST_PORT;
		pFilters->m_StaticFilters[0].m_TransportFilter.m_TcpUdp.m_DestPort.m_StartRange = 1; 
		pFilters->m_StaticFilters[0].m_TransportFilter.m_TcpUdp.m_DestPort.m_EndRange = 65535;

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

		// Transport layer filter 
		pFilters->m_StaticFilters[1].m_TransportFilter.m_dwUnionSelector = TCPUDP;
		pFilters->m_StaticFilters[1].m_TransportFilter.m_TcpUdp.m_ValidFields = TCPUDP_SRC_PORT;
		pFilters->m_StaticFilters[1].m_TransportFilter.m_TcpUdp.m_SourcePort.m_StartRange = 1; 
		pFilters->m_StaticFilters[1].m_TransportFilter.m_TcpUdp.m_SourcePort.m_EndRange = 65535;

		//***************************************************************************************
		// 3. Pass all packets (skipped by previous filters) without processing in user mode
		// Common values
		pFilters->m_StaticFilters[2].m_Adapter.QuadPart = 0; // applied to all adapters
		pFilters->m_StaticFilters[2].m_ValidFields = 0;
		pFilters->m_StaticFilters[2].m_FilterAction = FILTER_PACKET_PASS;
		pFilters->m_StaticFilters[2].m_dwDirectionFlags = PACKET_FLAG_ON_RECEIVE | PACKET_FLAG_ON_SEND;
	}

	api.SetPacketFilterTable(pFilters);
}

// pass in a const char*.
DWORD WINAPI makeTcpConnection(LPVOID lpIpAddr)
{
	struct sockaddr_in server;

	SOCKET conn;
	conn=socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
	if(conn==INVALID_SOCKET) {
		printf ("Invalid socket %d.\n", WSAGetLastError());
		return -1;
	}
	server.sin_addr.s_addr = inet_addr((char*)lpIpAddr);
	server.sin_family=AF_INET;
	server.sin_port=htons(80);

	//should timeout be concerned???
	if(connect(conn,(struct sockaddr*)&server,sizeof(server)))	{
		printf ("Failed to create connection %s.\n", (char*)lpIpAddr);
		closesocket(conn);
		return -1;	
	}
	printf ("Succeeded to create connection %s.\n", (char*)lpIpAddr);
	closesocket(conn);
	return 0;
}

BOOL isActiveAdaptor() 
{
	ether_header*		pEtherHdr = NULL;
	iphdr_ptr			pIpHdr		= NULL;
	tcphdr_ptr			pTcpHdr		= NULL;
	udphdr_ptr			pUdpHdr		= NULL;

	ADAPTER_MODE Mode;

	Mode.dwFlags = MSTCP_FLAG_SENT_TUNNEL|MSTCP_FLAG_RECV_TUNNEL;
	Mode.hAdapterHandle = (HANDLE)AdList.m_nAdapterHandle[iIndex];

	// Create notification event
	hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

	// Set event for helper driver
	if ((!hEvent)||(!api.SetPacketEvent((HANDLE)AdList.m_nAdapterHandle[iIndex], hEvent)))
	{
		printf ("Failed to create notification event or set it for driver.\n");
		return FALSE;
	}

	setupFilter();

	// Initialize Request
	ZeroMemory ( &Request, sizeof(ETH_REQUEST) );
	ZeroMemory ( &PacketBuffer, sizeof(INTERMEDIATE_BUFFER) );
	Request.EthPacket.Buffer = &PacketBuffer;
	Request.hAdapterHandle = (HANDLE)AdList.m_nAdapterHandle[iIndex];
		
	api.SetAdapterMode(&Mode);

	BOOL gotPackets = FALSE;
	UINT maxRuntime = 300; //300 milliseconds
	DWORD started = GetTickCount();

	DWORD dwThreadId;
	HANDLE hThread = CreateThread( 
		NULL,                   // default security attributes
        0,                      // use default stack size  
        makeTcpConnection,      // thread function name
        "202.202.202.202",      // argument to thread function 
        0,                      // use default creation flags 
        &dwThreadId);   // returns the thread identifier

	//detect if any tcp packet gets caught
	while (!gotPackets)
	{
		WaitForSingleObject ( hEvent, 100 ); //wait at most 100 milliseconds
				
		while(!gotPackets && api.ReadPacket(&Request))
		{
			pEtherHdr = (ether_header_ptr)PacketBuffer.m_IBuffer;

			if (ntohs(pEtherHdr->h_proto) == ETH_P_IP)
			{
				pIpHdr = (iphdr*)(PacketBuffer.m_IBuffer + sizeof(ether_header));

				if(pIpHdr->ip_p == IPPROTO_TCP)
				{
					pTcpHdr = (tcphdr_ptr)(((PUCHAR)pIpHdr) + sizeof(DWORD)*pIpHdr->ip_hl);
					if (pIpHdr->ip_dst.S_un.S_un_b.s_b1 == 202 && pIpHdr->ip_dst.S_un.S_un_b.s_b2 == 202 
						&& pIpHdr->ip_dst.S_un.S_un_b.s_b3 == 202 && pIpHdr->ip_dst.S_un.S_un_b.s_b4 == 202) {
						gotPackets = TRUE;
					}
				}
				else
				{
					pTcpHdr = NULL;
				}
			}
			
			if (PacketBuffer.m_dwDeviceFlags == PACKET_FLAG_ON_SEND)
			{
				// Place packet on the network interface
				api.SendPacketToAdapter(&Request);
			}
			else
			{
				// Indicate packet to MSTCP
				api.SendPacketToMstcp(&Request);
			}
			if (GetTickCount() - started > maxRuntime) {
				break;
			}
		}
		ResetEvent(hEvent);
		if (GetTickCount() - started > maxRuntime) {
			break;
		}
	}

	api.FlushAdapterPacketQueue((HANDLE)AdList.m_nAdapterHandle[iIndex]);
	ReleaseInterface();
	return gotPackets;
}

/**
Detect which adaptor is active. The result will be in iIndex. If not found, iIndex will be -1.
*/
void detectActiveAdaptor()
{
	iIndex = -1;
	for (UINT i = 0; i < AdList.m_nAdapterCount; ++i)
	{
		iIndex = i;
		if (isActiveAdaptor())
		{
			printf ("* Internal Name:\t %s\n", AdList.m_szAdapterNameList[i]);
			break;
		} else {
			printf ("  Internal Name:\t %s\n", AdList.m_szAdapterNameList[i]);
			iIndex = -1;
		}
	}
}

void printPacket(iphdr_ptr pIpHdr, tcphdr_ptr pTcpHdr, USHORT oriFlags, USHORT newFlags) {
	printf("%d.%d.%d.%d:%d ---> %d.%d.%d.%d:%d ori_flags %d new_flags %d", 
		pIpHdr->ip_src.S_un.S_un_b.s_b1, pIpHdr->ip_src.S_un.S_un_b.s_b2, 
		pIpHdr->ip_src.S_un.S_un_b.s_b3, pIpHdr->ip_src.S_un.S_un_b.s_b4, 
		ntohs(pTcpHdr->th_sport),
		pIpHdr->ip_dst.S_un.S_un_b.s_b1, pIpHdr->ip_dst.S_un.S_un_b.s_b2, 
		pIpHdr->ip_dst.S_un.S_un_b.s_b3, pIpHdr->ip_dst.S_un.S_un_b.s_b4, 
		ntohs(pTcpHdr->th_dport),
		oriFlags,
		newFlags);
}

inline BOOL isNodeIp(u_long ip) {
	return nodes.count(ip) > 0;
}

void justPassPackets(iphdr_ptr pIpHdr, tcphdr_ptr pTcpHdr) 
{
			
	if (PacketBuffer.m_dwDeviceFlags == PACKET_FLAG_ON_SEND)
	{
		// Place packet on the network interface
		api.SendPacketToAdapter(&Request);
	}
	else
	{
		// Indicate packet to MSTCP
		api.SendPacketToMstcp(&Request);
	}
}

void outSynModifyTtl(iphdr_ptr pIpHdr, tcphdr_ptr pTcpHdr, u_char ttl) {
	if (PacketBuffer.m_dwDeviceFlags == PACKET_FLAG_ON_SEND) {
		if (pTcpHdr != NULL && pTcpHdr->th_flags == TH_SYN && isNodeIp(pIpHdr->ip_dst.S_un.S_addr)) {
			pIpHdr->ip_ttl = ttl;
			RecalculateIPChecksum(pIpHdr);
		}
		// Place packet on the network interface
		api.SendPacketToAdapter(&Request);
	} else {
		// Indicate packet to MSTCP
		api.SendPacketToMstcp(&Request);
	}
}

void outSynModifyTcpFlags(iphdr_ptr pIpHdr, tcphdr_ptr pTcpHdr, u_char flags) {
	if (PacketBuffer.m_dwDeviceFlags == PACKET_FLAG_ON_SEND) {
		if (pTcpHdr != NULL && pTcpHdr->th_flags == TH_SYN && isNodeIp(pIpHdr->ip_dst.S_un.S_addr)) {
			pTcpHdr->th_flags = flags;
			pTcpHdr->th_sum = GetTcpChecksum(pIpHdr, pTcpHdr);
		}
		// Place packet on the network interface
		api.SendPacketToAdapter(&Request);
	} else {
		// Indicate packet to MSTCP
		api.SendPacketToMstcp(&Request);
	}
}

//32 16 8 4 2 1
// U  A P R S F
// R  C S S Y I
// G  K H T N N
// It's better to handle both in and out packets in just one function.
// How can we write unit tests for these functions???
void outSyn2Ack_InPushAck2SynAck(iphdr_ptr pIpHdr, tcphdr_ptr pTcpHdr) 
{
	if (PacketBuffer.m_dwDeviceFlags == PACKET_FLAG_ON_SEND)
	{
		if (pTcpHdr != NULL) {
			if (pTcpHdr->th_flags == TH_SYN) { //SYN
				if (isNodeIp(pIpHdr->ip_dst.S_un.S_addr)) { // send to node
					// keep a record of this port (using network value)
					portsWithSynOnly.insert(pTcpHdr->th_sport);
					//1. SYN ---> ACK
					pTcpHdr->th_flags = TH_ACK; // convert SYN to ACK
					//2. random ACK
					pTcpHdr->th_ack = (rand() << 16) + rand();
					//3. options to null. Need to change three places
					int sizeDelta = pTcpHdr->th_off - 5;
					pTcpHdr->th_off = 5;
					pIpHdr->ip_len = htons(ntohs(pIpHdr->ip_len) - sizeDelta * 4);
					Request.EthPacket.Buffer->m_Length -= (sizeDelta * 4);
					//4. recalculate checksums
					RecalculateIPChecksum(pIpHdr);
					pTcpHdr->th_sum = GetTcpChecksum(pIpHdr, pTcpHdr);

					printf("out\t%d\t", GetTickCount());
					printPacket(pIpHdr, pTcpHdr, TH_SYN, TH_ACK);
					printf("\n");
				}
			}
		}
		// Place packet on the network interface
		api.SendPacketToAdapter(&Request);
	}
	else
	{
		if (pTcpHdr != NULL) {
			if (pTcpHdr->th_flags == (TH_PSH | TH_ACK)) { //PSH/ACK
				if (isNodeIp(pIpHdr->ip_src.S_un.S_addr)) { // from node
					// Check if this is the first PSH/ACK packet for this port
					if (portsWithSynOnly.count(pTcpHdr->th_dport) > 0) { 
						// remove port from the monitoring set so that later PSH/ACK
						// packets will not be converted to SYN/ACK any more.
						portsWithSynOnly.erase(pTcpHdr->th_dport);
						//1. PSH/ACK ---> SYN/ACK
						pTcpHdr->th_flags = (TH_SYN | TH_ACK); // convert to SYN/ACK
						//2. trancate random data and restore options
						pTcpHdr->th_off = 7; // change from 5 to 7 to accomadate 8 bytes of options
						pIpHdr->ip_len = htons(48); 
						Request.EthPacket.Buffer->m_Length = 64;
						Request.EthPacket.Buffer->m_IBuffer[63] = 0;
						Request.EthPacket.Buffer->m_IBuffer[62] = 0;
						memcpy(Request.EthPacket.Buffer->m_IBuffer + 54, opt, 8);
						//3. recalculate checksums
						RecalculateIPChecksum(pIpHdr);
						pTcpHdr->th_sum = GetTcpChecksum(pIpHdr, pTcpHdr);

						printf("out\t%d\t", GetTickCount());
						printPacket(pIpHdr, pTcpHdr, TH_PSH | TH_ACK, TH_SYN | TH_ACK);
						printf("\n");

						//for (int j = 0; j < 60; j++) {
						//	printf("%02x ", Request.EthPacket.Buffer->m_IBuffer[j]);
						//	if (j % 16 == 15) {
						//		printf("\n");
						//	}
						//}
					}
				}
			}
		}
		// Indicate packet to MSTCP
		api.SendPacketToMstcp(&Request);
	}
}

void inDropSynAck(iphdr_ptr pIpHdr, tcphdr_ptr pTcpHdr) 
{
	if (PacketBuffer.m_dwDeviceFlags == PACKET_FLAG_ON_SEND)
	{
		// Place packet on the network interface
		api.SendPacketToAdapter(&Request);
		if (pTcpHdr != NULL) {
			if (pTcpHdr->th_flags == 2) { //SYN
				//we will monitor the response for this syn packet
				syn_request synRequest;
				syn_simple synSimple;
				synSimple.dst = pIpHdr->ip_dst;
				synSimple.src = pIpHdr->ip_src;
				synSimple.dst_port = ntohs(pTcpHdr->th_dport);
				synSimple.src_port = ntohs(pTcpHdr->th_sport);
				synSimple.start_time = GetTickCount();
				synRequest.synSimple = synSimple;
				synRequests.push_back(synRequest);
				printf("out\t%d\t", GetTickCount());
				printPacket(pIpHdr, pTcpHdr, TH_SYN | TH_ACK, 0);
				printf("\n");
			}
		}
	}
	else
	{
		BOOL drop = false;
		if (pTcpHdr != NULL) {
			if (pTcpHdr->th_flags == 18) { //SYN/ACK

				int size = synRequests.size();
				syn_request synRequest;
				//check all syn packets
				for (int i = 0; i < size; i++) {
					synRequest = synRequests.front();
					synRequests.pop_front();

					//if this syn matches the syn-ack
					if (!drop && synRequest.synSimple.dst.S_un.S_addr == pIpHdr->ip_src.S_un.S_addr &&
						synRequest.synSimple.dst_port == ntohs(pTcpHdr->th_sport) &&
						synRequest.synSimple.src.S_un.S_addr == pIpHdr->ip_dst.S_un.S_addr &&
						synRequest.synSimple.src_port == ntohs(pTcpHdr->th_dport)) {
						if (GetTickCount() - synRequest.synSimple.start_time < minSynAndSynAckInterval) {
							drop = true;
						} else {
						}
					} 

					// if already hold the syn long enough or not
					if (GetTickCount() - synRequest.synSimple.start_time < minSynAndSynAckInterval) {
						// keep holding. hold at least for certain period of time
						synRequests.push_back(synRequest);
						printf("\t---> keep holding the syn\n" );
					} else {
						printf("\t---> release syn\n" );
					}
				}
				if (drop) {
					printf("in\t%d\t", (GetTickCount() - synRequest.synSimple.start_time));
					printPacket(pIpHdr, pTcpHdr, TH_SYN | TH_ACK, 0);
					printf(" dropped\n");

				} else {
				}
			} 
		} 
		if (!drop) {
			// Indicate packet to MSTCP
			api.SendPacketToMstcp(&Request);
		}
	}

}

int main(int argc, char* argv[])
{
	if (argc < 2) {
		cout << "Please specify the IP addresses to operate on.\nIP addresses are separated by white space.";
		exit(0);
	}
	for (int i = 1; i < argc; i++) {
		nodes.insert(inet_addr(argv[i]));
		cout << argv[i] << " " << inet_addr(argv[i]) << "\n";
	}
	
	ether_header*		pEtherHdr = NULL;
	iphdr_ptr			pIpHdr		= NULL;
	tcphdr_ptr			pTcpHdr		= NULL;
	udphdr_ptr			pUdpHdr		= NULL;

	//initialize winsock2
	WSADATA wsaData;
	WSAStartup(0x101,&wsaData);

	if(!api.IsDriverLoaded())
	{
		printf ("Driver not installed on this system of failed to load.\n");
		return 0;
	}
	
	api.GetTcpipBoundAdaptersInfo ( &AdList );

	detectActiveAdaptor();

	if (iIndex == -1) {
		printf("No active adaptor detected!");
		return 0;
	} else {
		printf("Working on packets ...... \n");
	}
	
	ADAPTER_MODE Mode;

	Mode.dwFlags = MSTCP_FLAG_SENT_TUNNEL|MSTCP_FLAG_RECV_TUNNEL;
	Mode.hAdapterHandle = (HANDLE)AdList.m_nAdapterHandle[iIndex];

	// Create notification event
	hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

	// Set event for helper driver
	if ((!hEvent)||(!api.SetPacketEvent((HANDLE)AdList.m_nAdapterHandle[iIndex], hEvent)))
	{
		printf ("Failed to create notification event or set it for driver.\n");
		return 0;
	}

	atexit (ReleaseInterface);

	setupFilter();

	// Initialize Request
	ZeroMemory ( &Request, sizeof(ETH_REQUEST) );
	ZeroMemory ( &PacketBuffer, sizeof(INTERMEDIATE_BUFFER) );
	Request.EthPacket.Buffer = &PacketBuffer;
	Request.hAdapterHandle = (HANDLE)AdList.m_nAdapterHandle[iIndex];
		
	api.SetAdapterMode(&Mode);


	while (TRUE)
	{
		WaitForSingleObject ( hEvent, INFINITE );
				
		while(api.ReadPacket(&Request))
		{
			pEtherHdr = (ether_header_ptr)PacketBuffer.m_IBuffer;

			if (ntohs(pEtherHdr->h_proto) == ETH_P_IP)
			{
				pIpHdr = (iphdr*)(PacketBuffer.m_IBuffer + sizeof(ether_header));

				if(pIpHdr->ip_p == IPPROTO_TCP)
				{
					pTcpHdr = (tcphdr_ptr)(((PUCHAR)pIpHdr) + sizeof(DWORD)*pIpHdr->ip_hl);
				}
				else
				{
					pTcpHdr = NULL;
				}
			}
			
			// here we process packets.
			// Do we want a switch statement here???
			//outSyn2Ack_InPushAck2SynAck(pIpHdr, pTcpHdr);
			//outSynModifyTtl(pIpHdr, pTcpHdr, 23);
			//outSynModifyTcpFlags(pIpHdr, pTcpHdr, TH_FIN | TH_ACK);
			outSynModifyTcpFlags(pIpHdr, pTcpHdr, TH_RST | TH_ACK);
		}

		ResetEvent(hEvent);
	
	}

	return 0;
}
