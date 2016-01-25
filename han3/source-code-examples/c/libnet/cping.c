/*
 *  $Id: cping.c,v 1.10 2003/03/31 19:31:21 yudong Exp yudong $
 *  Program to send data to pong
 *  ping: commander; pong: responder
 *  cip -> sip: clients ==> servers 
 *  Usage:  
 *  cping -s svr_IP -c clt_IP -p ping_IP -q pong_IP -n nhosts -i if
 *    -C client_dat_file  -S server_dat_file
 *  To compile:
 *   gcc -static -o cping cping.c -lnet -lpcap 
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

u_long cip, sip, pip, qip; /* client, server, ping, pong */ 
u_long pqseq0, scseq0;    /* seq/ack numbers */
u_long pqseq, scseq;    /* seq/ack numbers */
u_long pqack, scack;    /* seq/ack numbers */
u_short cport = 12768;   /* client port */
u_short sport = 80;      /* server port */
u_short pport = 12768;   /* client port */
u_short nhosts, pay_0 = 0;
u_short nch = 1;       /* command channel: ping listens on (12768 - nch) */
u_char cttl = 12, sttl = 18;  /* TTL of IP */
char pqmsg[MAXTEXT];   /* pointer to the commnad struct */ 
char *dms = NULL;  /* server payload */
char *dmc = NULL;  /* client payload */
u_long pay_s = 0; 
int c, delay; 

struct cmdpayload {   // the command payload structure
  u_short nhosts;     // number of hosts to span across
  u_short ttl;          // ttl with 2bytes for alignment
  u_long pip;     // ping ip 
  u_long cip;     // client ip 
} cmd0;

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
    u_short cmdsize = sizeof (struct cmdpayload); 
    u_char ttl;  /* TTL of IP */
    u_char iflags, flags = 0x00;      /* tcp flag */
    u_char pqflags = 0x00, scflags = 0x00; 
    u_char opt[20];   /* tcp options, maybe used later */
    char *msg, *dms0;  /* generic packet message */ 
    u_long pqmsglen, scmsglen;   /* strlen */
    u_short pqout, scout;   /* send or not send?  */
    struct in_addr addr;        

  //  for extracting data from packets
  const struct libnet_ipv4_hdr *iph; /* The IP header */
  const struct libnet_tcp_hdr *tcph; /* The TCP header */

 /* For readability, make variables for the sizes of each of the structures */
  int size_ethernet = 14;  // sizeof(struct ether_header);
  int size_ip = 20;  // sizeof(struct iphdr);
  int size_tcp = 20; // sizeof(struct tcphdr);
  int i; 

  usleep(delay);  /* waiting for pong to finish sending */
  iph = (struct libnet_ipv4_hdr*)(packet + size_ethernet);
  tcph = (struct libnet_tcp_hdr*)(packet + size_ethernet + size_ip);

  win = tcph->th_win;            //keep the same win
  iflags = tcph->th_flags;        // received flags 
  switch (iflags) { 
    case TH_SYN:     /* ACK: q->q; (SYN, ACK): s->c */
                pqflags = TH_ACK; 
                scflags = TH_SYN | TH_ACK; 
                pqseq = pqseq0 + 1; 
                pqack = scseq0 + 1; 
                scseq = scseq0;
                scack = pqseq0 + 1; 
                pqmsglen = cmdsize; 
                scmsglen = 0; 
                dms0 = NULL;
                pqout = 1; 
                scout = 1;
                printf("Q===>P: SYN\n"); 
                break; 
    case TH_ACK:     /* (PSH,ACK): p->q; nothing: s->c */
                pqflags = TH_ACK | TH_PUSH;  
                scflags = 0x00;   /* server does not send to clnt */
                pqseq = pqseq0 + 1; 
                pqack = scseq0 + 1; 
                pqmsglen = cmdsize + strlen(dmc) + 1; /* keep \0 at end */
                scmsglen = 0; 
                pqout = 1; 
                scout = 0;
                printf("Q===>P: ACK\n"); 
                break; 
    case (TH_ACK | TH_PUSH): /* nothing: p->q; ACK: s->c */
                pqflags = TH_SYN; 
                scflags = TH_ACK; 
                pqseq = pqseq0; 
                pqack = 0; 
                scseq = scseq0 + 1;
                scack = pqseq0 + 1 + strlen(dmc); 
                scmsglen = strlen(dms); 
                dms0 = dms; 
                pqout = 0; 
                scout = 1;
                printf("Q===>P: (PSH, ACK)\n"); 
                break; 
    default: 
                break; 
  }  // tcp flags logic

  /* constructing and sending packets */
  for (i=0; i <= nhosts; i++)  {  
   if (i==0) {  // send command packet first
      src_prt = pport; 
      dst_prt = libnet_get_prand(LIBNET_PRu16);  /* dst port */ 
      seq = pqseq;
      ack = pqack; 
      src_ip = sip; 
      dst_ip = qip;   // to pinger 
      flags = pqflags; 
      ttl = 0x80; 
      msg = pqmsg;   //command payload
      pay_s = pqmsglen; 
   }  else {
      src_prt = sport;   // server port does not change
      dst_prt = cport + i;
      seq = scseq;
      ack = scack; 
      src_ip = sip + htonl(i); 
      dst_ip = cip + htonl(i); 
      flags = scflags; 
      ttl = sttl;                // get the ttl 
      msg = dms0; 
      pay_s = scmsglen; 
   }  // end of preparing headers 

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
            msg,                                /* payload */
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


        if( ( !i && pqout) || (i && scout))
          c = libnet_write(l);     /*  Write it to the wire. */
        //if ( i % 1000 == 0) sleep(1);  take a break every 1000 packets

        /**--- for debugging only
        fprintf(stderr, "i=: %d  pay_s = %d \n", i, pay_s);
        addr.s_addr = src_ip; 
        fprintf(stderr, "Src IP: %s ==>", inet_ntoa(addr));
        addr.s_addr = dst_ip; 
        fprintf(stderr, "Dst IP: %s\n", inet_ntoa(addr));
        fprintf(stderr, "seq:  %lu   ack: %lu\n", ntohl(seq), ntohl(ack) ); 
        fflush(stderr);
        --- **/
    //printf("Reply %d: RST = %d Seq = %lu TTL = %d Win = %u \n", count,
       // tcp->th_flags, ntohl(tcp->th_seq), ip->ip_ttl, ntohs(tcp->th_win));
  } // end for
 
   if(!pqout && scout) {  /* dialog finished. exit */
     pcap_close(descr);
     libnet_destroy(l);
     exit(0); 
   }
  
}  // end of callback


int main(int argc, char *argv[])
{
    int n, i; 
    u_long cmdsize = sizeof(struct cmdpayload);
    char errbuf[LIBNET_ERRBUF_SIZE];
    /* pcap variables */
    char perrbuf[PCAP_ERRBUF_SIZE];
    //unicode strings (winnt)
    char *dev ="eth0";   /* default if */
    char filter[MAXTEXT]; 
    struct bpf_program fp;      /* hold compiled program     */
    bpf_u_int32 maskp;          /* subnet mask               */
    bpf_u_int32 netp;           /* ip                        */
    void usage(char *);

    FILE *fclt; /* file handles for server and client content */ 
    char *dmfc = NULL, *dmfs = NULL;  /* file names */
    char *ptmp; 
    char linebuf[MAXTEXT];

    printf("cping v1.10 Mar 31, 2003. timeout: 20 sec. -x is in.\n"); 
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

    while ((n = getopt(argc, argv, "s:c:p:q:i:n:d:C:S:x:")) != EOF)
    {
        switch (n)
        {
         case 's':
           if ((sip = libnet_name2addr4(l, optarg, LIBNET_RESOLVE)) == -1)
             {
                fprintf(stderr, "Bad server IP address: %s\n", optarg);
                exit(EXIT_FAILURE);
             }
           break;
         case 'c':
           if ((cip = libnet_name2addr4(l, optarg, LIBNET_RESOLVE)) == -1)
             {
               fprintf(stderr, "Bad client IP address: %s\n", optarg);
               exit(EXIT_FAILURE);
             }
           break;
         case 'p':
           if ((pip = libnet_name2addr4(l, optarg, LIBNET_RESOLVE)) == -1)
             {
               fprintf(stderr, "Bad ping IP address: %s\n", optarg);
               exit(EXIT_FAILURE);
             }
           break;
         case 'q':
           if ((qip = libnet_name2addr4(l, optarg, LIBNET_RESOLVE)) == -1)
             {
               fprintf(stderr, "Bad pong IP address: %s\n", optarg);
               exit(EXIT_FAILURE);
             }
           break;
         case 'n':
           nhosts = (u_short)atoi(optarg);
           break;
         case 'x':
           nch = (u_short)atoi(optarg);
           break;
         case 'd':
           delay = atoi(optarg);
           break;
         case 'C':
           dmfc = optarg;
           break;
         case 'S':
           dmfs = optarg;
           break;
         case 'i':
           dev = optarg;
           break;
         default:
                usage(argv[0]);
                exit(EXIT_FAILURE);
        }
    }

/* Read the client payload  */
   if (dmfc != NULL) { /* -- if -C option is given */
    if( (fclt = fopen(dmfc, "r")) == NULL) {
      fprintf(stderr, "Error opening the content file %s \n", dmfc);
      exit(1);
    } /* end if */
 
  /*  printf("Reading client -> server content ...\n"); */
    i = 0;
    while ((i < MAXTEXT) && ( (c=fgetc(fclt)) != EOF ) )
     {
       linebuf[i++] = c;
     }
    fclose(fclt);
    linebuf[i] = '\0';
    dmc = (char *)malloc(strlen(linebuf)+1);  /* alloc mem */
    strcpy(dmc, linebuf);
   } /* ---end if */

/* Read the server payload  */ 
   if (dmfs != NULL) { /* -- if -C option is given */
    if( (fclt = fopen(dmfs, "r")) == NULL) {
      fprintf(stderr, "Error opening the content file %s \n", dmfs);
      exit(1);
    } /* end if */
 
  /*  printf("Reading server content ...\n"); */
    i = 0;
    while ((i < MAXTEXT) && ( (c=fgetc(fclt)) != EOF ) )
     {
       linebuf[i++] = c;
     }
    fclose(fclt);
    linebuf[i] = '\0';
    dms = (char *)malloc(strlen(linebuf)+1);  /* alloc mem */
    strcpy(dms, linebuf);
   } /* ---end if */

    pcap_lookupnet(dev,&netp,&maskp,perrbuf);  

    /* open device for reading in promiscuous mode */ 
    descr = pcap_open_live(dev,BUFSIZ,1,-1,perrbuf);
    if(descr == NULL)
    { printf("pcap_open_live(): %s\n",perrbuf); exit(1); }

    /* Lets try and compile the program.. non-optimized */
    /* sprintf(filter, "src %s", inet_ntoa(addr) );  */
    sprintf(filter, "dst port %d", pport-nch);   // ping listening port 
    printf("The filter is: %s\n", filter); 
    if(pcap_compile(descr,&fp,filter,0,netp) == -1)
    { fprintf(stderr,"Error calling pcap_compile\n"); exit(1); }

    /* set the compiled program as the filter */
    if(pcap_setfilter(descr,&fp) == -1)
    { fprintf(stderr,"Error setting filter\n"); exit(1); }

    /* construct ping payload */
    cmd0.nhosts = nhosts; 
    cmd0.pip = pip; 
    cmd0.cip = cip; 
    cmd0.ttl = cttl | (nch << 8);   /* merge nch and ttl in u_long */
    ptmp = (char *)&cmd0;  /* get the pointer */
    for (i=0; i < cmdsize; i++) {
       pqmsg[i] = *(ptmp + i);        /* copy cmd */
    }
    for (i=0; i <= strlen(dmc); i++) {
       pqmsg[i+cmdsize] = *(dmc + i); /* append client content */
    }

    pqseq0 = libnet_get_prand(LIBNET_PRu32); 
    scseq0 = libnet_get_prand(LIBNET_PRu32); 
    /* send out the 1st command to pong */
        tcp = libnet_build_tcp(
            pport,                                /* source port */
            libnet_get_prand(LIBNET_PRu16),        /* dst port */ 
            pqseq0,   /* seq # */
            0,                                /* ack =0 */
            TH_SYN,          /* TCP flags */
            libnet_get_prand(LIBNET_PRu16),        /* win */ 
            0,                                      /* checksum */
            0,                                      /* urgent pointer */
            LIBNET_TCP_H + cmdsize,           /* packet length */
            pqmsg,                                /* payload */
            cmdsize,                              /* payload size */
            l,                                      /* libnet handle */
            tcp);                                   /* libnet id */

            
            ip = libnet_build_ipv4(        /* building ip */
                LIBNET_IPV4_H + cmdsize + LIBNET_TCP_H, /* length */
                0x00,                                          /* TOS */
                0,                                            /* IP ID */
                0x4000,                                          /* IP Frag */
                0x80,                                         /* TTL */
                IPPROTO_TCP,                                /* protocol */
                0,                                          /* checksum */
                sip,
                qip,
                NULL,                                       /* payload */
                0,                                          /* payload size */
                l,                                          /* libnet handle */
                ip);                                         /* libnet id */

        c = libnet_write(l);     /*  Write it to the wire. */

    /* start the sniffer */ 
    printf("First ping sent. Sniffer started\n"); 
    /* start the sniffer */
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
    printf("usage %s -s svr_IP -c clt_IP -p ping_IP -q pong_IP \n", name);
    printf("-i if -n nhosts -d delay -C client_file -S server_file -x channel\n");  
}


