/*
 *  $Id: rst2synack-raw.c,v 1.1 2010/06/06 14:04:02 oneway Exp oneway $
 *  Similar to rst2synack.c,, but use raw socket instead of libnet to 
 *  craft packet. 
 *  Sends a RST when seeing a SYN-ACK. 
 *  Usage:  
 *  rst2synack-raw -p remote_port -i interface
 *  to compile:   
    gcc -o rst2synack-raw rst2synack-raw.c -lpcap

 *
 */

#include <unistd.h>
#include <pcap.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/if_ether.h>
#include <net/ethernet.h>
#include <netinet/ip.h>
#define __FAVOR_BSD
#include <netinet/tcp.h>

#define MAXTEXT 1400
#define TTL  127    //TTL in IP header
#define __USE_BSD 1
#define __BYTE_ORDER __LITTLE_ENDIAN

// global variables 
pcap_t* descr;
unsigned long pay_s = 0; 
int c, rawsock; 

void send_tcp(int rawsock,
              unsigned long ip_src, unsigned short sport,
              unsigned long ip_dst, unsigned short dport,
              unsigned long seq,    unsigned long  ack,
              unsigned short flag ) {

  int i;
  char dbuf[MAXTEXT];  // data buffer for constructing  tcp segment

  struct ip *iph = (struct ip *) dbuf;
  struct tcphdr *tcph = (struct tcphdr *) ( dbuf + sizeof(struct ip) );
  struct sockaddr_in dsin;  // dst addr for sendto()

  memset(dbuf, 0, MAXTEXT);
  dsin.sin_family = AF_INET;
  dsin.sin_port = dport; 
  dsin.sin_addr.s_addr = ip_dst; 

 // Constructing IP and TCP headers
  iph->ip_hl = 5;
  iph->ip_v = 4;
  iph->ip_tos = 0;
  iph->ip_len = sizeof (struct ip) + sizeof (struct tcphdr);    /* no payload */
  iph->ip_id = htons (random());
  iph->ip_off = 0;
  iph->ip_ttl = TTL;
  iph->ip_p = 6;  // payload is TCP
  iph->ip_sum = 0; // let kernel recalculate it
  iph->ip_src.s_addr = ip_src; 
  iph->ip_dst.s_addr = dsin.sin_addr.s_addr;
  tcph->th_sport = htons(sport); 
  tcph->th_dport = htons(dport); 
  tcph->th_seq = htonl(seq); 
  tcph->th_ack = htonl(ack); 
  tcph->th_x2 = 0;
  tcph->th_off = 5;
  tcph->th_flags = flag;
  tcph->th_win = htons (32768); /* max window size */
  tcph->th_sum = 0;
  tcph->th_urp = 0;

  i = sendto(rawsock, dbuf, iph->ip_len,
             0, (struct sockaddr *) &dsin, sizeof (dsin));

  if ( i == -1 )  {
       fprintf(stderr, "sending to %s failed!\n", inet_ntoa(dsin.sin_addr));
  } else  {
       printf("Outgoing packet: \n"); 
       printf("%u bytes sent to  %s \n", i, inet_ntoa(dsin.sin_addr));
       /**--- for debugging only **/
       fprintf(stderr, "Src IP: %s ==>", inet_ntoa(iph->ip_src));
       fprintf(stderr, "Dst IP: %s\n", inet_ntoa(iph->ip_dst));
       fprintf(stderr, "Src port: %d ==>", sport);
       fprintf(stderr, "Dst port: %d\n\n", dport);
       fflush(stderr);
       /* */
  }

}  // end of function


/* packet processing callback */ 
void my_callback(unsigned char *useless, const struct pcap_pkthdr* pkthdr,
                 const unsigned char* packet)
{
    // variables for constructing packets
    unsigned long seq, ack;    /* seq/ack numbers */
    unsigned long src_ip, dst_ip; 
    unsigned short src_prt, dst_prt, win; 
    unsigned short ttl;  /* TTL of IP */
    unsigned short flags = 0x00;      /* tcp flag */
    struct in_addr addr, saddr, daddr;        

  //  for extracting data from packets
  const struct ip *iph; /* The IP header */
  const struct tcphdr *tcph; /* The TCP header */

 /* For readability, make variables for the sizes of each of the structures */
  int size_ethernet = 14;  // sizeof(struct ether_header);
  int size_ip = 20;  // sizeof(struct iphdr);
  int size_tcp = 20; // sizeof(struct tcphdr);

  iph = (struct ip*)(packet + size_ethernet);
  tcph = (struct tcphdr*)(packet + size_ethernet + size_ip);

  /* constructing and sending RST packet */
      saddr = iph->ip_src; 
      daddr = iph->ip_dst; 
      src_ip = saddr.s_addr; 
      dst_ip = daddr.s_addr; 
      src_prt = ntohs(tcph->th_sport); 
      dst_prt = ntohs(tcph->th_dport);  /* dst port */ 
      seq = ntohl(tcph->th_seq) + 1; 
      ack = ntohl(tcph->th_ack); 
      win = ntohs(tcph->th_win);            //keep the same win
      flags = TH_RST; 
      ttl = 0x81; 
      pay_s = 0; 

       printf("================================\n"); 
       printf("Incoming packet: \n"); 
       /**--- for debugging only **/
       fprintf(stderr, "Src IP: %s ==>", inet_ntoa(saddr)); 
       fprintf(stderr, "Dst IP: %s\n", inet_ntoa(daddr)); 
       fprintf(stderr, "Src port: %d\n", src_prt);
       fprintf(stderr, "Dst port: %d\n\n", dst_prt);
       fflush(stderr);

      send_tcp(rawsock,
         src_ip,  src_prt,    // src IP and port
         dst_ip,  dst_prt,    // dst IP and port
         seq, ack, flags);
  
}  // end of callback


int main(int argc, char *argv[])
{
    int n; 
    int on=1; /* IP_HDRINCL on or off. 0: let kernel make header */

    /* pcap variables */
    char perrbuf[PCAP_ERRBUF_SIZE];
    char *dev ="eth0";   /* default if */
    char filter[MAXTEXT]; 
    unsigned short sport;      /* remote port from which syn-ack is coming */
    struct bpf_program fp;      /* hold compiled program     */
    bpf_u_int32 maskp;          /* subnet mask               */
    bpf_u_int32 netp;           /* ip                        */
    void usage(char *);

    if ( (rawsock = socket(AF_INET, SOCK_RAW, IPPROTO_IPIP)) < 0){
        perror("main:socket()");
        exit(-1);
    }

    //* we build the ip header
    if( setsockopt(rawsock, IPPROTO_IP, IP_HDRINCL, &on, sizeof(on)) < 0){
        perror("main:setsockopt()");
        exit(-1);
   }

    if (argc != 5) {
                usage(argv[0]);
                exit(-1);
    }

    while ((n = getopt(argc, argv, "p:i:")) != EOF)
    {
        switch (n)
        {
         case 'p':
           sport = (unsigned short) atoi(optarg);  /* local port */
           break;
         case 'i':
           dev = optarg;  /* device */
           break;
         default:
                usage(argv[0]);
                exit(-1); 
        }
    }

    pcap_lookupnet(dev,&netp,&maskp,perrbuf);  

    /* open device for reading in promiscuous mode */ 
    descr = pcap_open_live(dev,BUFSIZ,1,-1,perrbuf);
    if(descr == NULL)
    { printf("pcap_open_live(): %s\n",perrbuf); exit(1); }

    /* Lets try and compile the program.. non-optimized */
    //sprintf(filter, "src port %d and tcp[13] == 18 and host 173.64.113.167", sport); /* only SYN-ACK */
    sprintf(filter, "src port %d and tcp[13] == 18", sport); /* only SYN-ACK */
    /* printf("The filter is: %s\n", filter);  */
    if(pcap_compile(descr,&fp,filter,0,netp) == -1)
    { fprintf(stderr,"Error calling pcap_compile\n"); exit(1); }

    /* set the compiled program as the filter */
    if(pcap_setfilter(descr,&fp) == -1)
    { fprintf(stderr,"Error setting filter\n"); exit(1); }

    pcap_loop(descr,-1,my_callback,NULL);
    pcap_close(descr);

    close(rawsock); 

    return (0);
}

void
usage(char *name)
{
    printf("usage %s -p remoteport -i interface\n", name);
}


