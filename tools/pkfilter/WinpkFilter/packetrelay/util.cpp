#include "stdafx.h"

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

// pass in a const char* of IP address.
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
