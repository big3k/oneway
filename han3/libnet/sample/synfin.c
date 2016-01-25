/*
 *  $Id$
 *  Program to test if OS will produce CLOSE_WAIT state. 
 *  First sends a SYN to remote host, and as seeing (SYN, ACK)
 *   coming back, send a FIN. 
 *  Usage:  
 *  synfin -s sip -d dip -p dport -i dev
 *   gcc -static -o synfin synfin.c -lnet -lpcap 
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
libnet_ptag_t tcp, t_op;
pcap_t* descr;

u_short dport = 80;      /* default remote port */
u_long pay_s = 0; 
u_long sip, dip; 
int c, delay; 

static void time_out_exit (int signo)  /* out of the loop */
{
  exit(0);
}

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
  int i; 

  iph = (struct libnet_ipv4_hdr*)(packet + size_ethernet);
  tcph = (struct libnet_tcp_hdr*)(packet + size_ethernet + size_ip);

  /* constructing and sending FIN-ACK packet */
      saddr = iph->ip_src; 
      daddr = iph->ip_dst; 
      src_ip = daddr.s_addr; 
      dst_ip = saddr.s_addr; 
      src_prt = ntohs(tcph->th_dport); 
      dst_prt = ntohs(tcph->th_sport);  /* dst port */ 
      seq = ntohl(tcph->th_ack); 
      ack = ntohl(tcph->th_seq) + 1; 
      win = ntohs(tcph->th_win);            //keep the same win
      flags = TH_FIN | TH_ACK;       // FIN
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

         /**--- for debugging only  
        fprintf(stderr, "i=: %d  pay_s = %d \n", i, pay_s);
        addr.s_addr = src_ip; 
        fprintf(stderr, "Src IP: %s ==>", inet_ntoa(addr));
        addr.s_addr = dst_ip; 
        fprintf(stderr, "Dst IP: %s\n", inet_ntoa(addr));
        fprintf(stderr, "Dst port: %d\n", dst_prt);
        fprintf(stderr, "Src port: %d\n", src_prt);
        fprintf(stderr, "seq:  %lu   ack: %lu\n", seq, ack ); 
        fflush(stderr);
         --- **/
    //printf("Reply %d: RST = %d Seq = %lu TTL = %d Win = %u \n", count,
       // tcp->th_flags, ntohl(tcp->th_seq), ip->ip_ttl, ntohs(tcp->th_win));
  
}  // end of callback


int main(int argc, char *argv[])
{
    int n, i; 
    char errbuf[LIBNET_ERRBUF_SIZE];
    /* pcap variables */
    char perrbuf[PCAP_ERRBUF_SIZE];
    char *dev ="eth0";   /* default if */
    char filter[MAXTEXT]; 
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
    t_op = 0; 

    while ((n = getopt(argc, argv, "s:d:p:i:")) != EOF)
    {
        switch (n)
        {
         case 's':
           if ((sip = libnet_name2addr4(l, optarg, LIBNET_RESOLVE)) == -1)
             {
                fprintf(stderr, "Bad source IP address: %s\n", optarg);
                exit(EXIT_FAILURE);
             }
           break;
         case 'd':
           if ((dip = libnet_name2addr4(l, optarg, LIBNET_RESOLVE)) == -1)
             {
               fprintf(stderr, "Bad dst IP address: %s\n", optarg);
               exit(EXIT_FAILURE);
             }
           break;
         case 'p':
           dport = (u_short) atoi(optarg);  /* dst port */
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
    sprintf(filter, "src port %d and tcp[13]&18=18", dport); /* SYN-ACK */
    /* printf("The filter is: %s\n", filter);  */
    if(pcap_compile(descr,&fp,filter,0,netp) == -1)
    { fprintf(stderr,"Error calling pcap_compile\n"); exit(1); }

    /* set the compiled program as the filter */
    if(pcap_setfilter(descr,&fp) == -1)
    { fprintf(stderr,"Error setting filter\n"); exit(1); }

    /* send SYN out */
        t_op = libnet_build_tcp_options(
              "\x02\x04\x05\xb4\x01\x01\x04\x02\x01\x00\x00\x00",
              12,
              l,
              t_op);

        tcp = libnet_build_tcp(
            libnet_get_prand(LIBNET_PRu16),        /* src port */
            dport,        /* dst port */
            libnet_get_prand(LIBNET_PRu32),   /* seq # */
            0,                                /* ack =0 */
            TH_SYN,          /* TCP flags */
            libnet_get_prand(LIBNET_PRu16),        /* win */
            0,                                      /* checksum */
            0,                                      /* urgent pointer */
            LIBNET_TCP_H + 12,           /* packet length */
            NULL,                                /* payload */
            0,                              /* payload size */
            l,                                      /* libnet handle */
            tcp);                                   /* libnet id */

            ip = libnet_build_ipv4(        /* building ip */
                LIBNET_IPV4_H + LIBNET_TCP_H + 12, /* length */
                0x00,                                          /* TOS */
                0,                                            /* IP ID */
                0x4000,                                          /* IP Frag */
                0x80,                                         /* TTL */
                IPPROTO_TCP,                                /* protocol */
                0,                                          /* checksum */
                sip,
                dip,
                NULL,                                       /* payload */
                0,                                          /* payload size */
                l,                                          /* libnet handle */
                ip);                                         /* libnet id */
 
        c = libnet_write(l);     /*  Write it to the wire. */

    /* waiting for response */ 
    signal(SIGALRM,  &time_out_exit);
    alarm(20);    /* time out after 20sec */
    pcap_loop(descr,-1,my_callback,NULL);
    alarm(0);

    pcap_close(descr);

    libnet_destroy(l);

    return (EXIT_SUCCESS);
bad:
    libnet_destroy(l);
    return (EXIT_FAILURE);
}

void
usage(char *name)
{
    printf("usage %s -s sip -d dip -p dport -i interface\n", name);
}


