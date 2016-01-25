/*
 *  $Id: cpong.c,v 1.8 2003/04/11 02:35:28 xor Exp xor $
 *  Program to reflect ping's data 
 *  ping: commander; pong: responder
 *  cip -> sip: clients ==> servers 
 *  Usage:  
 *  c-ping -s src_IP -d dst_IP -p ping_IP -q pong_IP -n nhosts 
 *    -E "dat_file". 
 *  To compile:
 *   gcc -static -o cping cping.c -lnet -lpcap 
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include "../include/config.h"
#include "../include/pcap.h"
#include "./libnet_test.h"

#define MAXTEXT 1400
#define Max_Num_Adapter 10

// global variables 

libnet_t *l;     /* libnet stuff */
libnet_ptag_t ip;
libnet_ptag_t tcp;
u_short cport = 12768;   /* client port */
u_short sport = 80;      /* server port */
u_short pport = 12768;   /* ping port */

char     devlist[Max_Num_Adapter][1024];
struct cmdpayload {   // the command payload structure
  u_short nhosts;     // number of hosts to span across
  u_short ttl;          // ttl & channel
  u_long pip;     // ping ip 
  u_long cip;     // client ip 
};


/* packet processing callback */ 
void my_callback(u_char *useless, const struct pcap_pkthdr* pkthdr,
                 const u_char* packet)
{
    // variables for constructing packets
    u_long cip, sip, pip, qip; /* client, server, ping, pong */ 
    u_long src_ip, dst_ip; 
    u_short src_prt, dst_prt, win; 
    u_long seq, ack;    /* taken from ping-pong packet */
    u_char ttl, ctmp;  /* TTL of IP */
    u_char flags = 0x00;      /* tcp flag */
    u_char opt[20];   /* tcp options, maybe used later */
    u_char *dms0=NULL, *dms = NULL; /* payload */ 
    u_long nhosts, pay_s = 0;
    struct in_addr addr;        

  //  for extracting data from packets
  const struct libnet_ipv4_hdr *iph; /* The IP header */
  const struct libnet_tcp_hdr *tcph; /* The TCP header */
  struct cmdpayload *cmd; 

 /* For readability, make variables for the sizes of each of the structures */
  int size_ethernet = 14;  // sizeof(struct ether_header);
  int size_ip = 20;  // sizeof(struct iphdr);
  int size_tcp = 20; // sizeof(struct tcphdr);
  int i, c, count = 1;

  iph = (struct libnet_ipv4_hdr*)(packet + size_ethernet);
  tcph = (struct libnet_tcp_hdr*)(packet + size_ethernet + size_ip);
  cmd = (struct cmdpayload*)(packet + size_ethernet + size_ip + size_tcp);

  dms0 = (char*)cmd; 
  /** -- for debugging
  for (i=0; i< sizeof(struct cmdpayload); i++) {
    ctmp =  *(dms0 + i); 
    printf(" %02X", ctmp); 
  }
  ---**/
  nhosts = cmd->nhosts;    // number of targets 
  sip = iph->ip_src.s_addr;   // server ip 
  cip = cmd->cip;           // ip of server , which is not the dst ip 
  pip = cmd->pip;    // ip of pinger
  qip = cip;    // ip of ponger spoofed as client 
  win = tcph->th_win;            //use swapped win
  flags = tcph->th_flags;        // received flags 
  switch(flags) { 
   case TH_SYN: 
        printf("P===>Q: SYN\n"); fflush(stdout); break; 
   case TH_ACK: 
        printf("P===>Q: ACK\n"); fflush(stdout); break;
   case TH_ACK | TH_PUSH: 
        printf("P===>Q: (PSH, ACK)\n"); fflush(stdout);  break;
  }
  if (flags == (TH_ACK | TH_PUSH) ) {
     dms0 += sizeof(struct cmdpayload); //move point to string 
     for (i=0; i< strlen(dms0); i++) {
       ctmp =  *(dms0 + i); /* printf(" %02X", ctmp); */ }
  } else {
     dms0 = NULL; 
  }  // (ACK, PUSH) from ping contains payload

  seq = ntohl(tcph->th_seq); 
  ack = ntohl(tcph->th_ack);         //copy seq and ack # 
  /* constructing and sending packets */
  for (i=0; i <= nhosts; i++)  {  
   if (i==0) {  // send command packet first
      src_prt = libnet_get_prand(LIBNET_PRu16); 
      dst_prt = pport - (cmd->ttl >> 8);   /* command channel */ 
      src_ip = qip; 
      dst_ip = pip;   // to pinger 
      ttl = 0x80; 
      dms = NULL;   //empty payload
      pay_s = 0;    
   }  else {
      src_prt = cport + i;   // client port increases 
      dst_prt = sport;       // server port does not change
      src_ip = cip + htonl(i); 
      dst_ip = sip + htonl(i); 
      ttl = (u_char)(cmd->ttl & 0x00ff);               // get the ttl 
      dms = dms0; 
      pay_s = 0;
      if (dms) pay_s = strlen(dms); // content packets payload size
   }  // end of preparing headers 

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
        //if ( i % 1000 == 0) sleep(1);  //take a break every 1000 packets

        /**--- for debuggin 
        addr.s_addr = src_ip; 
        fprintf(stderr, "Src IP: %s\n", inet_ntoa(addr));
        addr.s_addr = dst_ip; 
        fprintf(stderr, "Dst IP: %s\n", inet_ntoa(addr));
        fprintf(stderr, "seq:  %lu   ack: %lu\n", ntohl(seq), ntohl(ack) ); 
        fflush(stderr);
        printf("here 2!"); 
        ---**/
    //printf("Reply %d: RST = %d Seq = %lu TTL = %d Win = %u \n", count,
       // tcp->th_flags, ntohl(tcp->th_seq), ip->ip_ttl, ntohs(tcp->th_win));
  } // end for
  
}  // end of callback


int main(int argc, char *argv[])
{
    int i; 
    char errbuf[LIBNET_ERRBUF_SIZE];
    /* pcap variables */
    char perrbuf[PCAP_ERRBUF_SIZE];
    //unicode strings (winnt)
    char      *devs;  //  a list of network adapters
    WCHAR           *temp,*temp1;
    char filter[MAXTEXT]; 
    pcap_t* descr;
    struct bpf_program fp;      /* hold compiled program     */
    bpf_u_int32 maskp;          /* subnet mask               */
    bpf_u_int32 netp;           /* ip                        */
    int devnum=0, Open; 
    void usage(char *);


    printf("cpong 1.8, Apr 11, 2003. -x is in. argv[1] is in\n"); 
    fflush(stdout);
    /* Initialize the library.  Root priviledges are required.  */
    l = libnet_init(
            LIBNET_RAW4,                            /* injection type */
            NULL,                                   /* network interface */
            errbuf);                                /* errbuf */

    if (l == NULL)
    {
        fprintf(stderr, "libnet_init() failed: %s\n", errbuf);
        fflush(stderr);
        exit(EXIT_FAILURE);
    }

    libnet_seed_prand(l);
    tcp = LIBNET_PTAG_INITIALIZER ; 
    ip = LIBNET_PTAG_INITIALIZER ;

    printf("Choose your adapters:\n"); fflush(stdout);
    i=0;
    /* Initialize pcap stuff */
    /* ask pcap for the network address and mask of the device */

    if ( (devs = pcap_lookupdev(perrbuf)) == NULL) {   // get the list of devs 
       printf("Unable to get the adapter list! \n");  fflush(stdout);
       return -1; 
    }
    temp = (WCHAR*)devs;
    temp1 = (WCHAR*)devs; 
  while ((*temp!='\0')||(*(temp-1)!='\0'))
  {
          if (*temp=='\0')
          {
                  memcpy(devlist[i],temp1,(temp-temp1)*2);
                  temp1=temp+1;
                  i++;
  }

  temp++;
  }

  devnum=i;
  for (i=0;i<devnum;i++)
       wprintf(L"\n%d- %s\n",i+1,devlist[i]);
  printf("\n"); fflush(stdout); 
  

   do
     {
      printf("Select the number of the adapter to open : ");
      fflush(stdout);
      if(argc==2) {
        Open=atoi(argv[1]);  /* get dev number from command line */
        argc=1; /* reset */
      } else 
        scanf("%d",&Open);
      if (Open>devnum) printf("\nThe number must be smaller than %d\n",devnum);
        fflush(stdout);
      if (Open==0) exit(0); 
     } while (Open>devnum);

   printf("Adpater chosen: %d\n", Open); fflush(stdout);

    pcap_lookupnet(devlist[Open-1],&netp,&maskp,perrbuf);  

    /* open device for reading in promiscuous mode */ 
    descr = pcap_open_live(devlist[Open-1],BUFSIZ,1,-1,perrbuf);
    if(descr == NULL)
    { printf("pcap_open_live(): %s\n",perrbuf); 
      fflush(stdout);
      exit(1); }

    /* Lets try and compile the program.. non-optimized */
    /* sprintf(filter, "src %s", inet_ntoa(addr) );  */
    sprintf(filter, "src port %d", pport);   // pinger port 
    printf("The filter is: %s\n", filter); fflush(stdout);
    if(pcap_compile(descr,&fp,filter,0,netp) == -1)
    { fprintf(stderr,"Error calling pcap_compile\n"); exit(1); }

    /* set the compiled program as the filter */
    if(pcap_setfilter(descr,&fp) == -1)
    { fprintf(stderr,"Error setting filter\n"); exit(1); }

    /* start the sniffer */ 
    printf("Sniffer started\n"); fflush(stdout);
    pcap_loop(descr,-1,my_callback,NULL);

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
    fprintf(stderr, "usage %s -s sIP -o sport -d dIP -p dport -t ttl -w win -E data_file -M seq -L ack -A|P|R|S|F \n",
                name);
}


