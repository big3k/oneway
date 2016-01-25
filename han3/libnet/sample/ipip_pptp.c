/*
 *  $Id: ipip_pptp.c,v 1.1 2009/11/14 03:59:12 root Exp root $
 *  Program to capture PPTP traffic and wrap it with ipip 
 *  Usage:  
 *  ./ipip_pptp -i inf -d remote_end_of_tunnel (server IP)  
 *  Example:
 *  ./ipip_pptp -i eth0 -d 12.34.56.78
 *
 *   gcc -o ipip_pptp ipip_pptp.c -lpcap 
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
#define MAXTEXT 1400
#define __USE_BSD 1
#define __BYTE_ORDER __LITTLE_ENDIAN

// global variables 

pcap_t* descr;
int rawsocket=0;  
struct in_addr svr_ip;   // server ip 

/* packet processing callback */ 
void my_callback(u_char *useless, const struct pcap_pkthdr* pkthdr,
                 const u_char* packet)
{
    struct ip *oiph; 
    struct sockaddr_in dstaddr;  
  
  memset(&dstaddr, 0, sizeof(dstaddr));   
  dstaddr.sin_family = AF_INET;     /* Address family: Internet protocols */
  dstaddr.sin_port = 0;       /* Leave it empty */
  dstaddr.sin_addr =  svr_ip;  /* Destination IP */
  //printf("Sending to  %s \n", inet_ntoa(dstaddr.sin_addr)); 

  int size_ethernet = 14;  // sizeof(struct ether_header);
  int i; 

  oiph = (struct ip *)(packet + size_ethernet);

  i = sendto(rawsocket, packet+size_ethernet, pkthdr->len - size_ethernet, 
           0, (struct sockaddr *) &dstaddr, sizeof (dstaddr)); 

  if ( i == -1 )  {
       fprintf(stderr, "sending to %s failed!\n", inet_ntoa(dstaddr.sin_addr));
  } else  {
       printf("%u bytes sent to  %s \n", i, inet_ntoa(dstaddr.sin_addr)); 
       /**--- for debugging only  
       fprintf(stderr, "Src IP: %s ==>", inet_ntoa(oiph->ip_src));
       fprintf(stderr, "Dst IP: %s\n", inet_ntoa(oiph->ip_dst));
       fflush(stderr);
       */
  }
   
}  // end of callback


int main(int argc, char *argv[])
{
    int n; 
    int on=0; /* IP_HDRINCL on or off. 0: let kernel make header */ 

    /* pcap variables */
    char perrbuf[PCAP_ERRBUF_SIZE];
    char dev[MAXTEXT];  /* default if */
    char filter[] = "ip proto 47 or dst port 1723"; 
    struct bpf_program fp;      /* hold compiled program     */
    bpf_u_int32 maskp;          /* subnet mask               */
    bpf_u_int32 netp;           /* ip                        */
    void usage(char *);

    while ((n = getopt(argc, argv, "i:d:")) != EOF)
    {
        switch (n)
        {
         case 'i':
           strncpy(dev, optarg, MAXTEXT);  /* device */
           break;
         case 'd':
           inet_aton(optarg, &svr_ip);  /* server ip  */
           break;
         default:
                usage(argv[0]);
                exit(-1);
        }
    }

    if ( (rawsocket = socket(AF_INET, SOCK_RAW, IPPROTO_IPIP)) < 0){
        perror("main:socket()"); 
        exit(-1);
    }

    //* Let kernel build the ip header 
    if( setsockopt(rawsocket, IPPROTO_IP, IP_HDRINCL, &on, sizeof(on)) < 0){
        perror("main:setsockopt()"); 
        exit(-1);
   }
 
    pcap_lookupnet(dev,&netp,&maskp,perrbuf);  

    /* open device for reading in promiscuous mode */ 
    descr = pcap_open_live(dev,BUFSIZ,1,-1,perrbuf);
    if(descr == NULL)
    { printf("pcap_open_live(): %s\n",perrbuf); exit(1); }

    /* printf("The filter is: %s\n", filter);  */
    if(pcap_compile(descr,&fp,filter,0,netp) == -1)
    { fprintf(stderr,"Error calling pcap_compile\n"); exit(1); }

    /* set the compiled program as the filter */
    if(pcap_setfilter(descr,&fp) == -1)
    { fprintf(stderr,"Error setting filter\n"); exit(1); }

    //pcap_loop(descr,-1, my_callback, &svr_ip);
    pcap_loop(descr,-1, my_callback, NULL); 
    pcap_close(descr);
    close(rawsocket); 

    return (0);
}

void
usage(char *name)
{
    printf("usage %s -i interface -d server_ip\nExample:\n", name);
    printf("%s -i eth0 -d 12.34.56.78\n", name);
}

