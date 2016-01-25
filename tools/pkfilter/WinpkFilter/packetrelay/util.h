#ifndef __UTIL_H__
#define __UTIL_H__


#ifndef WINSOCK2
USHORT htons( USHORT hostshort );
ULONG htonl( ULONG hostlong );

#define ntohs(X) htons(X)
#define ntohl(X) htonl(X)
#endif

USHORT GetChecksum(void* buf, int size);

USHORT GetTcpChecksum(iphdr_ptr ip, tcphdr_ptr tcp);

VOID RecalculateIPChecksum (iphdr_ptr pIpHeader);

DWORD WINAPI makeTcpConnection(LPVOID lpIpAddr);

void printPacket(iphdr_ptr pIpHdr, tcphdr_ptr pTcpHdr, USHORT oriFlags, USHORT newFlags) ;

#endif //__UTIL_H__