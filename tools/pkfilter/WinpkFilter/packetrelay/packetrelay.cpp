/*************************************************************************/
/* Module Name:  packetrelay.cpp                                            */
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
UCHAR				*frameSrcAddr, *frameDstAddr;
BOOL				_server = FALSE;
// number of bytes to shift when converting tcp to udp
int					_bytesToShift = 29;

std::map<int, std::vector<ULONG> >	_tcpPortIpMap;
std::map<int, std::vector<ULONG> >	_udpPortIpMap;

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

void setupFilterForAdaptorDectection() {
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

void setupFilterForRealWork() {
	// Allocate table filters for 10 filters
	DWORD dwTableSize = sizeof(STATIC_FILTER_TABLE) + sizeof(STATIC_FILTER)*9;
	pFilters = (PSTATIC_FILTER_TABLE)malloc(dwTableSize);
	memset (pFilters, 0, dwTableSize);

	{
		pFilters->m_TableSize = 3;

		//**************************************************************************************
		// 1. Incoming UDP responses filter: REDIRECT IN UDP packets with source PORT [1025, 65536]
		// Common values
		pFilters->m_StaticFilters[0].m_Adapter.QuadPart = 0; // applied to all adapters
		pFilters->m_StaticFilters[0].m_ValidFields = NETWORK_LAYER_VALID | TRANSPORT_LAYER_VALID;
		pFilters->m_StaticFilters[0].m_FilterAction = FILTER_PACKET_REDIRECT;
		pFilters->m_StaticFilters[0].m_dwDirectionFlags = PACKET_FLAG_ON_RECEIVE;

		// Network layer filter
		pFilters->m_StaticFilters[0].m_NetworkFilter.m_dwUnionSelector = IPV4; 
		pFilters->m_StaticFilters[0].m_NetworkFilter.m_IPv4.m_ValidFields = IP_V4_FILTER_PROTOCOL;
		pFilters->m_StaticFilters[0].m_NetworkFilter.m_IPv4.m_Protocol = IPPROTO_UDP;

		// Transport layer filter 
		pFilters->m_StaticFilters[0].m_TransportFilter.m_dwUnionSelector = TCPUDP;
		pFilters->m_StaticFilters[0].m_TransportFilter.m_TcpUdp.m_ValidFields = TCPUDP_SRC_PORT;
		pFilters->m_StaticFilters[0].m_TransportFilter.m_TcpUdp.m_SourcePort.m_StartRange = 1025; 
		pFilters->m_StaticFilters[0].m_TransportFilter.m_TcpUdp.m_SourcePort.m_EndRange = 65535;


		//****************************************************************************************
		// 2. Incoming TCP responses filter: REDIRECT IN TCP packets with source PORT [1025, 65536]
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
		pFilters->m_StaticFilters[1].m_TransportFilter.m_TcpUdp.m_SourcePort.m_StartRange = 1025; 
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

	setupFilterForAdaptorDectection();

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
        "74.125.24.51",			// argument to thread function 
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
					if (pIpHdr->ip_dst.S_un.S_un_b.s_b1 == 74 && pIpHdr->ip_dst.S_un.S_un_b.s_b2 == 125 
						&& pIpHdr->ip_dst.S_un.S_un_b.s_b3 == 24 && pIpHdr->ip_dst.S_un.S_un_b.s_b4 == 51) {
						gotPackets = TRUE;
						frameSrcAddr = (UCHAR *)malloc(6);
						frameDstAddr = (UCHAR *)malloc(6);
						memcpy(frameSrcAddr, PacketBuffer.m_IBuffer, 6);
						memcpy(frameDstAddr, PacketBuffer.m_IBuffer + 6, 6);
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

//32 16 8 4 2 1
// U  A P R S F
// R  C S S Y I
// G  K H T N N
void convertInTcp2OutUdp(iphdr_ptr pIpHdr, tcphdr_ptr pTcpHdr) {
	ULONG srcIp = pIpHdr->ip_src.s_addr;
	int srcPort = ntohs(pTcpHdr->th_sport);
	int dstPort = ntohs(pTcpHdr->th_dport);

	// shift whole IP packet by 29 bytes to the right.
	// The first 20 bytes are for IP header, the next 8 bytes are for
	// UDP header, and the next one byte is for compress flag (even number
	// for not compressed, odd number for compressed).

	// Frame header's length is 14.
	int bytesToShift = PacketBuffer.m_Length - 14;
	int newLength = PacketBuffer.m_Length + _bytesToShift;
	int oldIpLen = ntohs(pIpHdr->ip_len);
	if (newLength > MAX_ETHER_FRAME) {
		//need to compress
	} else {
		for (int i = 0; i < bytesToShift; i++) { 
			PacketBuffer.m_IBuffer[newLength - 1 - i] = PacketBuffer.m_IBuffer[PacketBuffer.m_Length - 1 - i];
		}
	}
	//modify frame header
	memcpy(PacketBuffer.m_IBuffer, frameSrcAddr, 6);
	memcpy(PacketBuffer.m_IBuffer + 6, frameDstAddr, 6);

	//construct IP header, get src IP address
	for (int ii = 0x1a; ii < 0x1e; ii++) { 
		PacketBuffer.m_IBuffer[ii] ^= PacketBuffer.m_IBuffer[ii + 4];
		PacketBuffer.m_IBuffer[ii + 4] ^= PacketBuffer.m_IBuffer[ii];
		PacketBuffer.m_IBuffer[ii] ^= PacketBuffer.m_IBuffer[ii + 4];
	}
	// get dst IP address
	std::vector<ULONG> ips = _tcpPortIpMap[dstPort];
	int size = ips.size();
	if (size == 1) {
		memcpy(PacketBuffer.m_IBuffer + 0x1e, &ips.at(0), 4);
	} else {
		int index = srcIp % size;
		std::cout << "Source ip is " << srcIp << " index " << index << " new IP " << ips.at(index) << std::endl;
		memcpy(PacketBuffer.m_IBuffer + 0x1e, &ips.at(index), 4);
	}

	PacketBuffer.m_IBuffer[0x17] = IPPROTO_UDP;
	// ip length
	PacketBuffer.m_IBuffer[0x11] += _bytesToShift;

	//construct UDP header
	PacketBuffer.m_IBuffer[0x22] = 0;
	PacketBuffer.m_IBuffer[0x23] = 0x89;
	PacketBuffer.m_IBuffer[0x24] = 0;
	PacketBuffer.m_IBuffer[0x25] = 0x89;

	//udp length 
	PacketBuffer.m_IBuffer[0x26] = (oldIpLen + 8 + (_bytesToShift == 28 ? 0 : 1)) / 256;
	PacketBuffer.m_IBuffer[0x27] = (oldIpLen + 8 + (_bytesToShift == 28 ? 0 : 1)) % 256;
	PacketBuffer.m_IBuffer[0x28] = 0;
	PacketBuffer.m_IBuffer[0x29] = 0;


	PacketBuffer.m_dwDeviceFlags = PACKET_FLAG_ON_SEND;
	PacketBuffer.m_Length += _bytesToShift;

	pIpHdr->ip_sum = 0;
	pIpHdr->ip_sum = GetChecksum(pIpHdr, pIpHdr->ip_hl * sizeof(DWORD));
	//RecalculateIPChecksum(pIpHdr);
	//pTcpHdr->th_sum = GetTcpChecksum(pIpHdr, pTcpHdr);

}

void tcpInUdpOut(iphdr_ptr pIpHdr, tcphdr_ptr pTcpHdr) 
{
	convertInTcp2OutUdp(pIpHdr, pTcpHdr);
	// Place packet on the network interface
	api.SendPacketToAdapter(&Request);
}

void convertInUdp2OutUdp(iphdr_ptr pIpHdr, udphdr_ptr pUdpHdr) {
	ULONG srcIp = pIpHdr->ip_src.s_addr;
	int srcPort = ntohs(pUdpHdr->th_sport);
	int dstPort = ntohs(pUdpHdr->th_dport);

	// shift whole IP packet by 29 bytes to the right.
	// The first 20 bytes are for IP header, the next 8 bytes are for
	// UDP header, and the next one byte is for compress flag (even number
	// for not compressed, odd number for compressed).

	// Frame header's length is 14.
	int bytesToShift = PacketBuffer.m_Length - 14;
	int newLength = PacketBuffer.m_Length + _bytesToShift;
	int oldIpLen = ntohs(pIpHdr->ip_len);
	if (newLength > MAX_ETHER_FRAME) {
		//need to compress
	} else {
		for (int i = 0; i < bytesToShift; i++) { 
			PacketBuffer.m_IBuffer[newLength - 1 - i] = PacketBuffer.m_IBuffer[PacketBuffer.m_Length - 1 - i];
		}
	}
	//modify frame header
	memcpy(PacketBuffer.m_IBuffer, frameSrcAddr, 6);
	memcpy(PacketBuffer.m_IBuffer + 6, frameDstAddr, 6);

	//construct IP header, get src IP address
	for (int ii = 0x1a; ii < 0x1e; ii++) { 
		PacketBuffer.m_IBuffer[ii] ^= PacketBuffer.m_IBuffer[ii + 4];
		PacketBuffer.m_IBuffer[ii + 4] ^= PacketBuffer.m_IBuffer[ii];
		PacketBuffer.m_IBuffer[ii] ^= PacketBuffer.m_IBuffer[ii + 4];
	}
	// get dst IP address
	std::vector<ULONG> ips = _udpPortIpMap[dstPort];
	int size = ips.size();
	if (size == 1) {
		memcpy(PacketBuffer.m_IBuffer + 0x1e, &ips.at(0), 4);
	} else {
		int index = srcIp % size;
		std::cout << "Source ip is " << srcIp << " index " << index << " new IP " << ips.at(index) << std::endl;
		memcpy(PacketBuffer.m_IBuffer + 0x1e, &ips.at(index), 4);
	}

	PacketBuffer.m_IBuffer[0x17] = IPPROTO_UDP;
	// ip length
	PacketBuffer.m_IBuffer[0x11] += _bytesToShift;

	//construct UDP header
	PacketBuffer.m_IBuffer[0x22] = 0;
	PacketBuffer.m_IBuffer[0x23] = 0x89;
	PacketBuffer.m_IBuffer[0x24] = 0;
	PacketBuffer.m_IBuffer[0x25] = 0x89;

	//udp length 
	PacketBuffer.m_IBuffer[0x26] = (oldIpLen + 8 + (_bytesToShift == 28 ? 0 : 1)) / 256;
	PacketBuffer.m_IBuffer[0x27] = (oldIpLen + 8 + (_bytesToShift == 28 ? 0 : 1)) % 256;
	PacketBuffer.m_IBuffer[0x28] = 0;
	PacketBuffer.m_IBuffer[0x29] = 0;


	PacketBuffer.m_dwDeviceFlags = PACKET_FLAG_ON_SEND;
	PacketBuffer.m_Length += _bytesToShift;

	pIpHdr->ip_sum = 0;
	pIpHdr->ip_sum = GetChecksum(pIpHdr, pIpHdr->ip_hl * sizeof(DWORD));
	//RecalculateIPChecksum(pIpHdr);
	//pTcpHdr->th_sum = GetTcpChecksum(pIpHdr, pTcpHdr);

}

void udpInUdpOut(iphdr_ptr pIpHdr, udphdr_ptr pUdpHdr) 
{
	convertInUdp2OutUdp(pIpHdr, pUdpHdr);
	// Place packet on the network interface
	api.SendPacketToAdapter(&Request);
}

void convertInUdp2Tcp(iphdr_ptr pIpHdr, tcphdr_ptr pTcpHdr) {
	justPassPackets(pIpHdr, pTcpHdr); 
}

void modifyOutTcp(iphdr_ptr pIpHdr, tcphdr_ptr pTcpHdr) {
	justPassPackets(pIpHdr, pTcpHdr); 
}

//tcp,80->65.49.68.151
//tcp,443->65.49.68.152,65.49.68.153
//udp,500->65.49.2.150
void initPortIpMaps(const char* file) {

	std::ifstream inFile(file, std::ios::in);
	if (!inFile) {
		std::cout << "Cannot open file " << file << std::endl;
		exit(0);
	}
	char seq[256];

	std::cout << "Processing " << file << std::endl;

	while(inFile >> seq) {
		std::cout << seq << std::endl;
		std::string line(seq);
		if (line.find("//") == 0) {
			//comment line
		} else if (line.find("tcp,") == 0 || line.find("udp,") == 0) {
			std::string type = line.substr(0, 3);
			std::cout << type << "," ;
			int i = line.find(",", 4);
			int port = atoi(line.substr(4, i - 4).c_str());
			std::cout << port << "->";
			int s = -1, e = -1;
			s = line.find(">") + 1;
			std::vector<ULONG> ips;
			std::vector<ULONG>::iterator it;

			it = ips.begin();

			do {
				e = line.find(",", s + 1);
				ULONG ip;
				if (e > s) {
					ip = inet_addr(line.substr(s, e - s).c_str());
				} else {
					ip = inet_addr(line.substr(s).c_str());
				}
				it = ips.insert(it, ip);
				it++;
				s = e + 1;
				std::cout << ip;
				if (e > 0) {
					std::cout << ",";
				} else {
					std::cout << std::endl;
				}
			} while (e > 0);
			int found = type.find("udp", 0);
			if (found >= 0) {
				_udpPortIpMap[port] = ips;
				std::cout << std::endl << "UDP port " << port << " size is " << _udpPortIpMap[port].size() << std::endl;
			} else {
				_tcpPortIpMap[port] = ips;
				std::cout << std::endl << "TCP port " << port << " size is " << _tcpPortIpMap[port].size() << std::endl;
			}
		} else {
			std::cout << std::endl << "Invalid line: " << line.c_str << std::endl;
			exit(0);
		}
	}

	inFile.close();
}


void printUsage() {
	std::cout << std::endl << "Usage: client client_conf_file" << std::endl;
	std::cout << "       server" << std::endl;
}

void parseArgs(int argc, char* argv[]) {
	if (argc == 2 && !strcmp(argv[1], "server")) {
		_server = TRUE;
	} else if (argc == 3 && !strcmp(argv[1], "client")) {
		_server = FALSE;
	} else {
		printUsage();
		exit(0);
	}
}

int main(int argc, char* argv[])
{
	parseArgs(argc, argv);
	if (!_server) {
		initPortIpMaps(argv[2]);
	}

	std::cout << std::endl << "Port 80 size is " << _tcpPortIpMap[80].size() << std::endl;
	std::cout << std::endl << "Port 443 size is " << _tcpPortIpMap[443].size() << std::endl;
	std::cout << std::endl << "Port 500 size is " << _udpPortIpMap[500].size() << std::endl;

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

	setupFilterForRealWork();

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

				pTcpHdr = NULL;
				pUdpHdr = NULL;
				if(pIpHdr->ip_p == IPPROTO_TCP)
				{
					pTcpHdr = (tcphdr_ptr)(((PUCHAR)pIpHdr) + sizeof(DWORD)*pIpHdr->ip_hl);
				}
				else if(pIpHdr->ip_p == IPPROTO_UDP)
				{
					pUdpHdr = (udphdr_ptr)(((PUCHAR)pIpHdr) + sizeof(DWORD)*pIpHdr->ip_hl);
				}
			}
			if (_server) {
				if (PacketBuffer.m_dwDeviceFlags == PACKET_FLAG_ON_SEND && pTcpHdr != NULL) {
					modifyOutTcp(pIpHdr, pTcpHdr); 
				} else if (PacketBuffer.m_dwDeviceFlags == PACKET_FLAG_ON_RECEIVE && pUdpHdr != NULL ) {
					convertInUdp2Tcp(pIpHdr, pTcpHdr); 
				} else {
					justPassPackets(pIpHdr, pTcpHdr); 
				}
			} else {
				// here we process packets.
				if (PacketBuffer.m_dwDeviceFlags == PACKET_FLAG_ON_RECEIVE && pTcpHdr != NULL 
					&& ntohs(pTcpHdr->th_sport) > 1024 && _tcpPortIpMap[ntohs(pTcpHdr->th_dport)].size() > 0) {
					tcpInUdpOut(pIpHdr, pTcpHdr);
				} else if (PacketBuffer.m_dwDeviceFlags == PACKET_FLAG_ON_RECEIVE && pUdpHdr != NULL 
					&& ntohs(pUdpHdr->th_sport) > 1024 && _udpPortIpMap[ntohs(pUdpHdr->th_dport)].size() > 0) {
					udpInUdpOut(pIpHdr, pUdpHdr);
				} else {
					justPassPackets(pIpHdr, pTcpHdr); 
				}
			}
		}

		ResetEvent(hEvent);
	
	}

	return 0;
}

