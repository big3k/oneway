/*
 *  $Id$
 *  Program to see what IPs UDP traffic is going to. 
 *  Will be useful for monitoring voice traffic, such as TS
 *  Usage:  
 *  udp-check -p local_port -t sniffing_time -i dev
 *  To compile:
 *   gcc -static -g -O2 -Wall -o udp-check udp-check.c -lpcap 
 *
 */

#include <pcap.h>
#include <libnet.h>
#include <getopt.h>
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
   struct in_addr daddr;
  const struct iphdr *ip; /* The IP header */
 /* For readability, make variables for the sizes of each of the structures */
  int size_ethernet = sizeof(struct ether_header);

  ip = (struct iphdr*)(packet + size_ethernet);
  daddr.s_addr = ip->daddr; 
  printf("%s\n", inet_ntoa(daddr));
}


int main(int argc, char *argv[])
{
    int c; 
    int snifft = 10; /** time to sniff, seconds */
    u_short src_prt = 80; 
    /* pcap variables */
    char perrbuf[PCAP_ERRBUF_SIZE];
    char *dev ="eth0";   /* default if */ 
    char filter[MAXTEXT]; 
    pcap_t* descr;
    struct bpf_program fp;      /* hold compiled program     */
    bpf_u_int32 maskp;          /* subnet mask               */
    bpf_u_int32 netp;           /* ip                        */
    void usage(char *);

    while ((c = getopt(argc, argv, "p:t:i:")) != EOF)
    {
        switch (c)
        {
         case 'p':
           src_prt = (u_short)atoi(optarg);
           break;
         case 't':
           snifft = (u_char)atoi(optarg);
           break;
         case 'i':
           dev = optarg;
           break;
         default:
                usage(argv[0]);
                exit(EXIT_FAILURE);
        }
    }

    /* Initialize pcap stuff */
    /* ask pcap for the network address and mask of the device */
    pcap_lookupnet(dev,&netp,&maskp,perrbuf);

    /* open device for reading in promiscuous mode */ 
    descr = pcap_open_live(dev,BUFSIZ,1,0,perrbuf);
    if(descr == NULL)
    { printf("pcap_open_live(): %s\n",perrbuf); exit(1); }

    /* Lets try and compile the program.. non-optimized */
    sprintf(filter, "src port %d", src_prt);  
    /*  printf("The filter is: %s\n", filter);  */
    if(pcap_compile(descr,&fp,filter,0,netp) == -1)
    { fprintf(stderr,"Error calling pcap_compile\n"); exit(1); }

    /* set the compiled program as the filter */
    if(pcap_setfilter(descr,&fp) == -1)
    { fprintf(stderr,"Error setting filter\n"); exit(1); }

    /* start the sniffer */ 
    signal(SIGALRM,  &time_out_exit);
    alarm(snifft);
    pcap_loop(descr,-1,my_callback,NULL);
    alarm(0); 
    pcap_close(descr);

    return (EXIT_SUCCESS);
bad:
    return (EXIT_FAILURE);
}

void
usage(char *name)
{
    fprintf(stderr, "usage %s -p sport -t snifftime -i dev \n",
                name);
}


