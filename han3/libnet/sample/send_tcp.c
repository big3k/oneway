/*
 *  $Id: send_tcp.c,v 1.3 2010/03/25 05:39:26 oneway Exp oneway $ 
 *  Program to send an empty TCP segment, with specified flag. 
 *   gcc -o send_tcp send_tcp.c
 */

#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/ip.h>
#define __FAVOR_BSD
#include <netinet/tcp.h>
#include <stdio.h> 

#define MAXTEXT 2048
#define TTL  127    //TTL in IP header 
#define __BYTE_ORDER __LITTLE_ENDIAN

void send_tcp(int rawsock, 
              const char *sp_src, unsigned short sp_src_port,   
              const char *sp_dst, unsigned short sp_dst_port,
              unsigned short flag ) { 

  int i; 
  char dbuf[MAXTEXT];  // data buffer for constructing  tcp segment 

  struct ip *iph = (struct ip *) dbuf;
  struct tcphdr *tcph = (struct tcphdr *) ( dbuf + sizeof(struct ip) );
  struct sockaddr_in dsin;  // dst addr for sendto() 

  memset(dbuf, 0, MAXTEXT); 
  dsin.sin_family = AF_INET;
  dsin.sin_port = htons (sp_dst_port);
  dsin.sin_addr.s_addr = inet_addr (sp_dst);

 // Constructing IP and TCP headers 
  iph->ip_hl = 5;
  iph->ip_v = 4;
  iph->ip_tos = 0;
  iph->ip_len = sizeof (struct ip) + sizeof (struct tcphdr);	/* no payload */
  iph->ip_id = htons (random());	
  iph->ip_off = 0;
  iph->ip_ttl = TTL;
  iph->ip_p = 6;  // payload is TCP 
  iph->ip_sum = 0; // let kernel recalculate it 
  iph->ip_src.s_addr = inet_addr (sp_src);
  iph->ip_dst.s_addr = dsin.sin_addr.s_addr;
  tcph->th_sport = htons (sp_src_port);	
  tcph->th_dport = htons (sp_dst_port);
  tcph->th_seq = random ();
  tcph->th_ack = random (); 
  tcph->th_x2 = 0;
  tcph->th_off = 5; 
  tcph->th_flags = flag;	
  tcph->th_win = htons (32768);	/* max window size */
  tcph->th_sum = 0;
  tcph->th_urp = 0;
 
  i = sendto(rawsock, dbuf, iph->ip_len, 
             0, (struct sockaddr *) &dsin, sizeof (dsin));

  if ( i == -1 )  {
       fprintf(stderr, "sending to %s failed!\n", inet_ntoa(dsin.sin_addr));
  } else  {
       printf("%u bytes sent to  %s \n", i, inet_ntoa(dsin.sin_addr));
       /**--- for debugging only **/
       fprintf(stderr, "Src IP: %s ==>", inet_ntoa(iph->ip_src));
       fprintf(stderr, "Dst IP: %s\n", inet_ntoa(iph->ip_dst));
       fflush(stderr);
       /* */
  }

}  // end of function 


int main(int argc, char *argv[])
{
    int n; 
    int on=1; /* IP_HDRINCL on or off. 0: let kernel make header */ 
    int rawsock; 

    if ( (rawsock = socket(AF_INET, SOCK_RAW, IPPROTO_IPIP)) < 0){
        perror("main:socket()"); 
        exit(-1);
    }

    //* we build the ip header 
    if( setsockopt(rawsock, IPPROTO_IP, IP_HDRINCL, &on, sizeof(on)) < 0){
        perror("main:setsockopt()"); 
        exit(-1);
   }
 

  send_tcp(rawsock,
           "192.168.1.2", 1234, // src IP and port 
           "192.168.1.4", 4321, // dst IP and port 
           TH_FIN); 
/* Other flags: 
#  define TH_FIN        0x01
#  define TH_SYN        0x02
#  define TH_RST        0x04
#  define TH_PUSH       0x08
#  define TH_ACK        0x10
#  define TH_URG        0x20
*/

    close(rawsock); 

    return (0);
}

