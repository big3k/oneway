/*
 *  $Id: rst2synack.c,v 1.1 2010/03/26 03:02:53 oneway Exp oneway $
 *  Sends a RST when seeing a SYN-ACK. 
 *  Usage:  
 *  rst2synack -p remote_port -i interface
 *  to compile:   
    gcc -I. -I../include -g -O2 -Wall -c rst2synack.c
    gcc -static -o rst2synack rst2synack.o ../src/libnet.a -lpcap

 *
 */

#include <pcap.h>
#include <libnet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/if_ether.h>
#include <net/ethernet.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#define MAXTEXT 1400
#define __USE_BSD 1
#define __BYTE_ORDER __LITTLE_ENDIAN

// global variables 

libnet_t *l;     /* libnet stuff */
libnet_ptag_t ip;
libnet_ptag_t tcp;
pcap_t* descr;

u_long pay_s = 0; 
int c; 

/* packet processing callback */ 
void my_callback(u_char *useless, const struct pcap_pkthdr* pkthdr,
                 const u_char* packet)
{
    // variables for constructing packets
    u_long seq, ack;    /* seq/ack numbers */
    u_long src_ip, dst_ip; 
    u_short src_prt, dst_prt, win; 
    u_char ttl;  /* TTL of IP */
    u_char flags = 0x00;      /* tcp flag */
    struct in_addr addr, saddr, daddr;        

  //  for extracting data from packets
  const struct libnet_ipv4_hdr *iph; /* The IP header */
  const struct libnet_tcp_hdr *tcph; /* The TCP header */

 /* For readability, make variables for the sizes of each of the structures */
  int size_ethernet = 14;  // sizeof(struct ether_header);
  int size_ip = 20;  // sizeof(struct iphdr);
  int size_tcp = 20; // sizeof(struct tcphdr);

  iph = (struct libnet_ipv4_hdr*)(packet + size_ethernet);
  tcph = (struct libnet_tcp_hdr*)(packet + size_ethernet + size_ip);

  /* constructing and sending RST packet */
      saddr = iph->ip_src; 
      daddr = iph->ip_dst; 
      src_ip = daddr.s_addr; 
      dst_ip = saddr.s_addr; 
      src_prt = ntohs(tcph->th_dport); 
      dst_prt = ntohs(tcph->th_sport);  /* dst port */ 
      seq = ntohl(tcph->th_ack); 
      ack = ntohl(tcph->th_seq) + 1; 
      win = ntohs(tcph->th_win);            //keep the same win
      flags = TH_RST; 
      ttl = 0x81; 
      pay_s = 0; 

     // fprintf(stderr, "here i=: %d  pay_s = %d \n", i, pay_s);
        tcp = libnet_build_tcp(
            src_prt,                                /* source port */
            dst_prt,                            /* destination port */
            seq,          /* seq  libnet_get_prand(LIBNET_PRu32), */
            ack,          /* ack  libnet_get_prand(LIBNET_PRu32), */
            flags,            /* TCP flags */
            win,         /* win libnet_get_prand(LIBNET_PRu16),  */ 
            0,                                      /* checksum */
            0,                                      /* urgent pointer */
            LIBNET_TCP_H + pay_s,           /* packet length */
            NULL,                                /* payload */
            pay_s,                              /* payload size */
            l,                                      /* libnet handle */
            tcp);                                   /* libnet id */

            
            ip = libnet_build_ipv4(        /* building ip */
                LIBNET_IPV4_H + pay_s + LIBNET_TCP_H, /* length */
                0x00,                                          /* TOS */
                0,                                            /* IP ID */
                0x4000,                                          /* IP Frag */
                ttl,                                         /* TTL */
                IPPROTO_TCP,                                /* protocol */
                0,                                          /* checksum */
                src_ip,
                dst_ip,
                NULL,                                       /* payload */
                0,                                          /* payload size */
                l,                                          /* libnet handle */
                ip);                                         /* libnet id */

          c = libnet_write(l);     /*  Write it to the wire. */
          /* printf("c= %ud\n", c); */

         /**--- for debugging only  */
        fprintf(stderr, "%d bytes sent\n", c); 
        addr.s_addr = src_ip; 
        fprintf(stderr, "Src IP: %s ==>", inet_ntoa(addr));
        addr.s_addr = dst_ip; 
        fprintf(stderr, "Dst IP: %s\n", inet_ntoa(addr));
        fprintf(stderr, "Dst port: %d\n", dst_prt);
        fprintf(stderr, "Src port: %d\n", src_prt);
        fprintf(stderr, "seq:  %lu   ack: %lu\n", seq, ack ); 
        fflush(stderr);
         /*--- **/
    //printf("Reply %d: RST = %d Seq = %lu TTL = %d Win = %u \n", count,
       // tcp->th_flags, ntohl(tcp->th_seq), ip->ip_ttl, ntohs(tcp->th_win));
  
}  // end of callback


int main(int argc, char *argv[])
{
    int n; 
    char errbuf[LIBNET_ERRBUF_SIZE];
    /* pcap variables */
    char perrbuf[PCAP_ERRBUF_SIZE];
    char *dev ="eth0";   /* default if */
    char filter[MAXTEXT]; 
    u_short sport;      /* remote port from which syn-ack is coming */
    struct bpf_program fp;      /* hold compiled program     */
    bpf_u_int32 maskp;          /* subnet mask               */
    bpf_u_int32 netp;           /* ip                        */
    void usage(char *);

    /* Initialize the library.  Root priviledges are required.  */
    l = libnet_init(
            LIBNET_RAW4,                            /* injection type */
            NULL,                                   /* network interface */
            errbuf);                                /* errbuf */

    if (l == NULL)
    {
        fprintf(stderr, "libnet_init() failed: %s\n", errbuf);
        exit(EXIT_FAILURE);
    }

    libnet_seed_prand(l);
    tcp = 0; 
    ip = 0;

    if (argc != 5) {
                usage(argv[0]);
                exit(EXIT_FAILURE);
    }

    while ((n = getopt(argc, argv, "p:i:")) != EOF)
    {
        switch (n)
        {
         case 'p':
           sport = (u_short) atoi(optarg);  /* local port */
           break;
         case 'i':
           dev = optarg;  /* device */
           break;
         default:
                usage(argv[0]);
                exit(EXIT_FAILURE);
        }
    }

    pcap_lookupnet(dev,&netp,&maskp,perrbuf);  

    /* open device for reading in promiscuous mode */ 
    descr = pcap_open_live(dev,BUFSIZ,1,-1,perrbuf);
    if(descr == NULL)
    { printf("pcap_open_live(): %s\n",perrbuf); exit(1); }

    /* Lets try and compile the program.. non-optimized */
    sprintf(filter, "src port %d and tcp[13] == 18", sport); /* only SYN-ACK */
    /* printf("The filter is: %s\n", filter);  */
    if(pcap_compile(descr,&fp,filter,0,netp) == -1)
    { fprintf(stderr,"Error calling pcap_compile\n"); exit(1); }

    /* set the compiled program as the filter */
    if(pcap_setfilter(descr,&fp) == -1)
    { fprintf(stderr,"Error setting filter\n"); exit(1); }

    pcap_loop(descr,-1,my_callback,NULL);
    pcap_close(descr);

    libnet_destroy(l);

    return (EXIT_SUCCESS);
}

void
usage(char *name)
{
    printf("usage %s -p remoteport -i interface\n", name);
}


