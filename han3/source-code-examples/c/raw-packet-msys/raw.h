
/*

raw.h 
- Contains the structures for the IP TCP UDP version 4 and 6 headers for raw sockets
can be used with IP_HDRINCL option with setsockopt

Author : Prasshhant Pugalia
*/

// Set the packing to a 1 byte boundary
//#include "pshpack1.h"

//IPv6 Structures
#include "ws2tcpip.h"   //for IP_HDRINCL
#include "pcap.h"

//Ethernet Header

typedef struct ethernet_header
{
	UCHAR dest[6];
	UCHAR source[6];
	USHORT type;
}   ETHER_HDR , *PETHER_HDR , FAR * LPETHER_HDR , ETHERHeader;


/*
 Define the IPv4 header. Make the version and length field one
 character since we can't declare two 4 bit fields without
 the compiler aligning them on at least a 1 byte boundary.
*/

struct ip
  {
    unsigned int ip_hl:4;               /* header length */
    unsigned int ip_v:4;                /* version */
    u_int8_t ip_tos;                    /* type of service */
    u_short ip_len;                     /* total length */
    u_short ip_id;                      /* identification */
    u_short ip_off;                     /* fragment offset field */
#define IP_RF 0x8000                    /* reserved fragment flag */
#define IP_DF 0x4000                    /* dont fragment flag */
#define IP_MF 0x2000                    /* more fragments flag */
#define IP_OFFMASK 0x1fff               /* mask for fragmenting bits */
    u_int8_t ip_ttl;                    /* time to live */
    u_int8_t ip_p;                      /* protocol */
    u_short ip_sum;                     /* checksum */
    struct in_addr ip_src, ip_dst;      /* source and dest address */
  };


typedef u_int32_t tcp_seq;

struct tcphdr
  {
    u_int16_t th_sport;         /* source port */
    u_int16_t th_dport;         /* destination port */
    tcp_seq th_seq;             /* sequence number */
    tcp_seq th_ack;             /* acknowledgement number */
    u_int8_t th_x2:4;           /* (unused) */
    u_int8_t th_off:4;          /* data offset */
    u_int8_t th_flags;
#  define TH_FIN        0x01
#  define TH_SYN        0x02
#  define TH_RST        0x04
#  define TH_PUSH       0x08
#  define TH_ACK        0x10
#  define TH_URG        0x20
    u_int16_t th_win;           /* window */
    u_int16_t th_sum;           /* checksum */
    u_int16_t th_urp;           /* urgent pointer */
};



//
typedef struct udp_hdr
{
    unsigned short source_port;     // Source port no.
    unsigned short dest_port;       // Dest. port no.
    unsigned short udp_length;      // Udp packet length
    unsigned short udp_checksum;    // Udp checksum (optional)
}   UDP_HDR, *PUDP_HDR , FAR * LPUDP_HDR , UDPHeader , UDP_HEADER;


typedef struct pseudo_header    //needed for checksum calculation
{
	u_long source_address;
	u_long dest_address;
	unsigned char placeholder;
	unsigned char protocol;
	unsigned short tcp_length;
	//char tcp[28];
 	struct tcphdr tcp;
}   P_HDR , PSEUDO_HDR , PSEUDO_HEADER;

// Restore the byte boundary back to the previous value
//#include <poppack.h>


//General Networking Functions
USHORT in_checksum(unsigned short* , int);
