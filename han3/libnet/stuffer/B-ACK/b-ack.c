/*
 *  $Id: b-ack.c,v 1.6 2003/02/03 19:43:24 yudong Exp yudong $
 *  Program to send data and capture the response
 *  Usage:  
 *  b-back -s src_IP -o src_port -d dst_IP -p dst_port -E dat_file 
 *      -i sniff-dev -w win -M seq -L ack -t ttl -A|-P|-R|-S|-F
 *  Send a block of characters ( <1400 ) out in an ACK TCP segment
 * and sniff the response
 * The text block is in the file "dat_file". 
 *  To compile:
 *   gcc -static -o b-ack b-ack.c -lnet -lpcap 
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

static void time_out_exit (int signo)  /* out of the loop */
{
  exit(0); 
} 

/* packet processing callback */ 
void my_callback(u_char *useless,const struct pcap_pkthdr* pkthdr,
                 const u_char* packet)
{
  const struct iphdr *ip; /* The IP header */
  const struct tcphdr *tcp; /* The TCP header */
  const char *payload; /* Packet payload */
 /* For readability, make variables for the sizes of each of the structures */
  int size_ethernet = sizeof(struct ether_header);
  int size_ip = sizeof(struct iphdr);
  int size_tcp = sizeof(struct tcphdr);
  static int count = 1;

  ip = (struct iphdr*)(packet + size_ethernet);
  tcp = (struct tcphdr*)(packet + size_ethernet + size_ip);
  payload = (u_char *)(packet + size_ethernet + size_ip + size_tcp);
  printf("Reply %d: RST = %d Seq = %lu TTL = %d Win = %u \n", count, 
      tcp->rst, ntohl(tcp->seq), ip->ttl, ntohs(tcp->window)); 
    count++;
}


int main(int argc, char *argv[])
{
    int c, i; 
    u_char *cp;
    libnet_t *l;
    libnet_ptag_t ip;
    libnet_ptag_t tcp;
    struct libnet_stats ls;
    u_long src_ip, dst_ip;
    u_short src_prt, dst_prt, win;
    u_long seq, ack;
    u_char flags = 0x00;      /* tcp flag */
    u_char opt[20];
    u_char ttl;  /* TTL of IP */
    char errbuf[LIBNET_ERRBUF_SIZE];
    /* pcap variables */
    char perrbuf[PCAP_ERRBUF_SIZE];
    char *dev ="eth0";   /* default if */ 
    char filter[MAXTEXT]; 
    pcap_t* descr;
    const u_char *packet;
    struct pcap_pkthdr hdr;     /* pcap.h */
    struct ether_header *eptr;  /* net/ethernet.h */
    struct bpf_program fp;      /* hold compiled program     */
    bpf_u_int32 maskp;          /* subnet mask               */
    bpf_u_int32 netp;           /* ip                        */
    struct in_addr addr;        
    void usage(char *);

    FILE *dmf; 
    char *dms = NULL; 
    int pay_s = 0;
    char *dmfn = NULL; 
    char *paybuf; 
    u_char bt;
    char linebuf[MAXTEXT];


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

    dst_ip = libnet_get_prand(LIBNET_PRu32);
    src_ip = libnet_get_prand(LIBNET_PRu32);
    src_prt = libnet_get_prand(LIBNET_PRu16);
    dst_prt = 80;			         /* http */		 
    win = libnet_get_prand(LIBNET_PRu16);
    seq = libnet_get_prand(LIBNET_PRu32);
    ack = libnet_get_prand(LIBNET_PRu32);

    while ((c = getopt(argc, argv, "d:o:s:p:i:E:t:M:L:w:APRSF")) != EOF)
    {
        switch (c)
        {
         case 'd':
           if ((dst_ip = libnet_name2addr4(l, optarg, LIBNET_RESOLVE)) == -1)
             {
                fprintf(stderr, "Bad destination IP address: %s\n", optarg);
                exit(EXIT_FAILURE);
             }
           break;
         case 's':
           if ((src_ip = libnet_name2addr4(l, optarg, LIBNET_RESOLVE)) == -1)
             {
               fprintf(stderr, "Bad source IP address: %s\n", optarg);
               exit(EXIT_FAILURE);
             }
           break;
         case 'o':
           src_prt = (u_short)atoi(optarg);
           break;
         case 'p':
           dst_prt = (u_short)atoi(optarg);
           break;
         case 't':
           ttl = (u_char)atoi(optarg);
           break;
         case 'E':
           dmfn = optarg; 
           break;
         case 'i':
           dev = optarg; 
           break;
         case 'M':
           seq = (u_long)atoi(optarg);
           break;
         case 'L':
           ack = (u_long)atoi(optarg);
           break;
         case 'w':
           win = (u_short)atoi(optarg);
           break;
	 case 'A':
           flags = flags | 0x10;  /* ACK */
           break;
         case 'P':
           flags = flags | 0x08;  /* PSH */
           break;
         case 'R':
           flags = flags | 0x04;  /* RST */
           break;
         case 'S':
           flags = flags | 0x02;  /* SYN */
           break;
         case 'F':
           flags = flags | 0x01;  /* FIN */
           break;
         default:
                usage(argv[0]);
                exit(EXIT_FAILURE);
        }
    }

           addr.s_addr = dst_ip; 
/*  double checking parameters 
           printf("Dest IP: %s\n", inet_ntoa(addr)); 
           printf("Dest port: %d\n", dst_prt); 
           printf("Content file name: %s\n", dmfn); 
           printf("Sniffing device: %s\n", dev); 
*/

    /* Initialize pcap stuff */
    /* ask pcap for the network address and mask of the device */
    pcap_lookupnet(dev,&netp,&maskp,perrbuf);

    /* open device for reading in promiscuous mode */ 
    descr = pcap_open_live(dev,BUFSIZ,1,0,perrbuf);
    if(descr == NULL)
    { printf("pcap_open_live(): %s\n",perrbuf); exit(1); }

    /* Lets try and compile the program.. non-optimized */
    sprintf(filter, "src %s", inet_ntoa(addr) );  
/*    printf("The filter is: %s\n", filter); */
    if(pcap_compile(descr,&fp,filter,0,netp) == -1)
    { fprintf(stderr,"Error calling pcap_compile\n"); exit(1); }

    /* set the compiled program as the filter */
    if(pcap_setfilter(descr,&fp) == -1)
    { fprintf(stderr,"Error setting filter\n"); exit(1); }

/* Read the keywords, dest IPs, src IPs , and construct http payload */ 
   if (dmfn != NULL) { /* -- if -E option is given */ 
    if( (dmf = fopen(dmfn, "r")) == NULL) {
      fprintf(stderr, "Error opening the content file %s \n", dmfn); 
      exit(1);
    } /* end if */ 
 
  /*  printf("Reading content ...\n"); */
    i = 0; 
    while ((i < MAXTEXT) && ( (c=fgetc(dmf)) != EOF ) )
     { 
       linebuf[i++] = c;
     }
    fclose(dmf);
    linebuf[i] = '\0'; 
    paybuf = linebuf;
    dms = (char *)malloc(strlen(paybuf)+1);  /* alloc mem */
    strcpy(dms, paybuf); 
    pay_s = strlen(dms); 
         /* ===== for message body debugging 
         printf("Content read:\n");
         for (i=0; i < pay_s; i++) { 
            bt = *(dms + i); 
            printf(" %02X", bt); 
         }  
         printf("\n");
        === */
   } /* ---end if */ 
   


/* Building TCP */
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
            dms,                                /* payload */
            pay_s,                              /* payload size */
            l,                                      /* libnet handle */
            tcp);                                   /* libnet id */

        if (tcp == -1) {
            fprintf(stderr, "Can't build TCP header: %s\n", libnet_geterror(l));
            goto bad;
        }

/* Building IP */
            
            ip = libnet_build_ipv4(
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

            if (ip == -1) {
             fprintf(stderr, "Can't build IP header: %s\n", libnet_geterror(l));
             goto bad;
            }

/*  Write it to the wire. */
        c = libnet_write(l);
        if (c == -1) { 
            fprintf(stderr, "Write error: %s\n", libnet_geterror(l));
            goto bad;
        } else {
            /* printf("Wrote %d byte TCP packet; check the wire.\n", c); */
        }

    /* start the sniffer */ 
    signal(SIGALRM,  &time_out_exit);
    alarm(4);
    pcap_loop(descr,20,my_callback,NULL);
    alarm(0); 
    pcap_close(descr);

   /*  usleep(20); slow down to conserve bandwidth */

  /* === debugging info
    libnet_stats(l, &ls);
    fprintf(stderr, "Packets sent:  %ld\n"
                    "Packet errors: %ld\n"
                    "Bytes written: %ld\n",
                    ls.packets_sent, ls.packet_errors, ls.bytes_written);
  ====*/
    libnet_destroy(l);

    return (EXIT_SUCCESS);
bad:
    libnet_destroy(l);
    return (EXIT_FAILURE);
}

void
usage(char *name)
{
    fprintf(stderr, "usage %s -s sIP -o sport -d dIP -p dport -t ttl -w win -E data_file -M seq -L ack -A|P|R|S|F \n",
                name);
}


