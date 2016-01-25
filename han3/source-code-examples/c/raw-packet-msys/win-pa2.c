
/* v1.0  Sun Oct 31 01:13:33 EDT 2010
   v1.1  5/6/2011: added "-r" option to support raw IP. 

To compile on MinGW/msys: 
gcc  -I/usr/include -I../WpdPack/Include -g -O2 -Wall -mno-cygwin -c win-pa2.c 
gcc -static -g -O2 -L../WpdPack/Lib -mno-cygwin -o win-pa2.exe win-pa2.o -lwpcap -lpacket -lws2_32 -liphlpapi
strip win-pa2.exe

*/

#include <unistd.h>
#include <stdlib.h>
#include <string.h> 
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <iphlpapi.h>
#include <pcap.h>
#include <remote-ext.h>
#include "raw.h" 

#define MAXTEXT 2048

/* Make a raw IP packet, including eithernet header. Take the parameters 
   and return the length of the packet */
int make_rawip(char *dbuf, 
              u_char *smac,  u_char *dmac,
              u_long src_ip, u_long dst_ip, 
              u_char ip_p,   u_char ttl,            // ip protocol & ttl
              char *payload, int paylen) 
{

  int ip_len;

  ETHER_HDR *eth = (ETHER_HDR *) dbuf; 
  struct ip *iph = (struct ip *) ( dbuf + sizeof(ETHER_HDR) );

 // Constructing ethernet header 
  memset(dbuf, 0, MAXTEXT);
  memcpy(eth->source, smac, 6); 
  memcpy(eth->dest, dmac, 6); 
  eth->type  = htons(0x0800);   //IP frame

 // Constructing IP and TCP headers
  iph->ip_hl = 5;
  iph->ip_v = 4;
  iph->ip_tos = 0;
  ip_len = sizeof (struct ip);    /* no payload yet*/
  iph->ip_id = htons (rand());
  iph->ip_off = 0;
  iph->ip_ttl = ttl; 
  iph->ip_p = ip_p;  // user specified ip protocol 
  iph->ip_sum = 0; 
  iph->ip_src.s_addr = src_ip; 
  iph->ip_dst.s_addr = dst_ip; 

  if ( paylen )  { /*  load user data */ 
    memcpy(dbuf+sizeof(ETHER_HDR)+ip_len, payload, paylen);  
    ip_len += paylen;  
  }
  iph->ip_len = htons(ip_len); 
  iph->ip_sum = in_checksum((unsigned short*)iph, sizeof(struct ip));

  /**--- for debugging only **/
  fprintf(stderr, "Src IP: %s ==>", inet_ntoa(iph->ip_src));
  fprintf(stderr, "Dst IP: %s\n", inet_ntoa(iph->ip_dst));
  fprintf(stderr, "Total bytes sent: %d\n", ip_len+sizeof(ETHER_HDR) );
  fflush(stderr);
  /* */
  return( ip_len + sizeof(ETHER_HDR) );  

}  // end of function


/* Make a TCP segment, including IP and eithernet header. Take the parameters 
   and return the length of the segment */
int make_tcp(char *dbuf, 
              u_char *smac,  u_char *dmac,
              u_long src_ip, u_long dst_ip, 
              u_long seq,    u_long ack,
              u_short src_prt, u_short dst_prt, 
              u_short win, u_short mss, u_char ttl, 
              u_short flags, char *payload, int paylen) 
{

  int i, ip_len;
  u_char opt[] = "\x02\x04\x00\x00\x01\x01\x04\x02";   /* tcp option  */

  ETHER_HDR *eth = (ETHER_HDR *) dbuf; 
  struct ip *iph = (struct ip *) ( dbuf + sizeof(ETHER_HDR) );
  struct tcphdr *tcph = (struct tcphdr *) ( dbuf + sizeof(ETHER_HDR) + sizeof(struct ip) );
  u_short nmss; 
  unsigned char seudo[MAXTEXT];
  P_HDR pseudo_header;

  memset(dbuf, 0, MAXTEXT);
  memcpy(eth->source, smac, 6); 
  memcpy(eth->dest, dmac, 6); 
  eth->type  = htons(0x0800);   //IP frame

 // Constructing ethernet header 
 // Constructing IP and TCP headers
  iph->ip_hl = 5;
  iph->ip_v = 4;
  iph->ip_tos = 0;
  ip_len = sizeof (struct ip) + sizeof (struct tcphdr);    /* no payload */
  iph->ip_id = htons (rand());
  iph->ip_off = 0;
  iph->ip_ttl = ttl; 
  iph->ip_p = 6;  // payload is TCP
  iph->ip_sum = 0; 
  iph->ip_src.s_addr = src_ip; 
  iph->ip_dst.s_addr = dst_ip; 
  tcph->th_sport = htons (src_prt);
  tcph->th_dport = htons (dst_prt);
  tcph->th_seq = htonl(seq); 
  tcph->th_ack = htonl(ack); 
  tcph->th_x2 = 0;
  tcph->th_off = 0x5;
  tcph->th_flags = flags;
  tcph->th_win = htons (win); /* max window size */
  tcph->th_sum = 0;
  tcph->th_urp = 0;

  if ( flags & 0x02  && mss ) { /* SYN is set and MSS > 0 */
    nmss = htons(mss); 
    tcph->th_ack = htonl(0); 
    strncpy(opt+2, &nmss, 2);  /* stuff mss in opt */
    strncpy(dbuf+sizeof(ETHER_HDR)+sizeof(struct ip)+sizeof(struct tcphdr), opt, 8);  /* Add option */ 
    ip_len += 8;   /* TCP option */ 
    tcph->th_off = 0x7; 

    printf("len=%d  Inside TCP option:", ip_len); 
    for (i=0; i<8; i++) printf("%02x ", (u_char)opt[i] ); 
    printf("\n\n");

  }

  if ( paylen )  { /*  load user data */ 
    strncpy(dbuf+sizeof(ETHER_HDR)+ip_len, payload, paylen);  
    ip_len += paylen;  
  }
  iph->ip_len = htons(ip_len); 
  iph->ip_sum = in_checksum((unsigned short*)iph, sizeof(struct ip));

  // tcp checksum 
 pseudo_header.source_address = src_ip; 
 pseudo_header.dest_address = dst_ip; 
 pseudo_header.placeholder = 0;
 pseudo_header.protocol = IPPROTO_TCP;
 pseudo_header.tcp_length = htons( ip_len - sizeof(struct ip) );
 memcpy(&pseudo_header.tcp, (u_char *)tcph, sizeof(struct tcphdr) ); 
 memcpy(seudo, &pseudo_header, sizeof(P_HDR) );
 memcpy(seudo + sizeof(P_HDR), (u_char *)tcph+sizeof(struct tcphdr), 
      ip_len-sizeof(struct ip) - sizeof(struct tcphdr) ); 
	
 tcph->th_sum = in_checksum((unsigned short*)seudo, 
       sizeof(P_HDR) + ip_len - sizeof(struct ip) - sizeof(struct tcphdr) );   
    
  /**--- for debugging only **/
  fprintf(stderr, "Src IP: %s ==>", inet_ntoa(iph->ip_src));
  fprintf(stderr, "Dst IP: %s\n", inet_ntoa(iph->ip_dst));
  fprintf(stderr, "Total bytes sent: %d\n", ip_len+sizeof(ETHER_HDR) );
  fflush(stderr);
  /* */
  return( ip_len + sizeof(ETHER_HDR) );  

}  // end of function


void
usage(char *name)
{
    fprintf(stderr, "\nUsage:\n win-pa2.exe -i dev_index -r ip_prot -s sIP -o sport -d dIP -p dport -t ttl -w win -T \"payload text\" -E data_file -M seq -L ack -A|P|R|S|F -O mss -v \n"); 
    fprintf(stderr, "If both -E and -T specified, -E overrides -T\n");
    fprintf(stderr, "-r ip_prot: produce a raw IP packet instead of TCP, with IP protocol ip_prot > 0\n"); 
    fprintf(stderr, "-O mss specifies MSS in TCP option. If not given, no TCP option will be present in SYN\n");
   // fprintf(stderr, "-u: create UDP packet instead of default TCP packet.\n");

}

void GetMacAddress(unsigned char *mac, IPAddr destip) {
    DWORD ret;
    IPAddr srcip = 0;
    ULONG MacAddr[2];
    ULONG PhyAddrLen = 6;  
    int i; 

    //Now print the Mac address also
    ret = SendARP(destip , srcip , MacAddr , &PhyAddrLen);
    if(PhyAddrLen) {
        BYTE *bMacAddr = (BYTE *) & MacAddr;
        for (i = 0; i < (int) PhyAddrLen; i++)
            mac[i] = (char)bMacAddr[i];
    }
}


void GetGateway(struct in_addr ip , char *sgatewayip , u_long *gatewayip) {
    char pAdapterInfo[5000];
    PIP_ADAPTER_INFO  AdapterInfo;
    ULONG OutBufLen = sizeof(pAdapterInfo) ;

    GetAdaptersInfo((PIP_ADAPTER_INFO) pAdapterInfo, &OutBufLen);
    for(AdapterInfo = (PIP_ADAPTER_INFO)pAdapterInfo; AdapterInfo ; AdapterInfo = AdapterInfo->Next) {
        if(ip.s_addr == inet_addr(AdapterInfo->IpAddressList.IpAddress.String))
     strcpy(sgatewayip , AdapterInfo->GatewayList.IpAddress.String);
    }
    *gatewayip = inet_addr(sgatewayip);
}


int main(int argc, char *argv[])
{

   /* for interfaces */
    pcap_if_t *alldevs;
    pcap_if_t *d;
    pcap_t *fp;
    pcap_addr_t *a; 
    char errbuf[PCAP_ERRBUF_SIZE];
    u_char s_mac[6],d_mac[6];
    IPAddr srcip, extip; 
    u_long gwip; 
    struct in_addr sip;
    char sgwip[16]; 
    int dev_idx = 0; 
    //DWORD ifidx; 
   /* for ip headers */
    u_long src_ip, dst_ip;
    u_long seq, ack;
    u_short src_prt, dst_prt, win, mss;
    u_short len;
    u_char flags = 0x00;      /* tcp flag */
    u_char ip_p = 0;  /* protocol of raw IP */
    u_char ttl;  /* TTL of IP */
    FILE *dmf;
    char *dms = NULL;
    int pay_s = 0;
    char *dmfn = NULL;
    char *paybuf;
    char linebuf[MAXTEXT], dbuf[MAXTEXT];
    int vbflag = 0;  /* verbose flag */
    int synflag = 0;  /* SYN flag.  */
    int udpflag = 0;  /* UDP flag.  */

    int i=0, c;
    
    srand((unsigned)time(NULL)); 
    /* default values */
    ttl='\xf0';  /* TTL of IP */
    dst_ip = rand() * rand(); 
    src_ip = rand() * rand(); 
    src_prt = rand(); 
    dst_prt = 80;                                /* http */              
    win = rand(); 
    seq = rand() * rand(); 
    ack = rand() * rand(); 
    mss = 0; 

    /* Command line option processing */ 

    while ((c = getopt(argc, argv, "i:r:d:o:s:p:T:E:t:M:L:w:O:APRSFvuh")) != EOF)
    {
        switch (c)
        {
         case 'i':
           dev_idx = atoi(optarg);
           break;
         case 'r':
           ip_p = (u_char)atoi(optarg); 
           break;
         case 'd':
           dst_ip = inet_addr(optarg); 
           break;
         case 's':
           src_ip = inet_addr(optarg); 
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
         case 'T':
           paybuf = optarg;
           dms = (char *)malloc(strlen(paybuf)+1);  /* alloc mem */
           strcpy(dms, paybuf);
           pay_s = strlen(dms);
           break;
         case 'E':
           dmfn = optarg;
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
         case 'O':
           mss = (u_short)atoi(optarg);
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
           synflag = 1;
           break;
         case 'u':
           udpflag = 1;
           break;
         case 'F':
           flags = flags | 0x01;  /* FIN */
           break;
         case 'v':
           vbflag = 1;  /* Verbose flag */
           break;
         case 'h':
           usage(argv[0]);
           break;
         default:
           usage(argv[0]);
        }
    }

/* Read the payload from file  */ 
   if (dmfn != NULL) { /* -- if -E option is given */
    if( (dmf = fopen(dmfn, "r")) == NULL) {
      fprintf(stderr, "Error opening the content file %s \n", dmfn);
      exit(1);
    } /* end if */

    i=fread(linebuf, sizeof(char), MAXTEXT, dmf);
    fclose(dmf);
    dms = (char *)malloc(i);  /* alloc mem */
    memcpy(dms, linebuf, i); 
    pay_s = i; 
    printf("Payload read from file, size=%d\n", i); 
   } /* ---end if */


    /* Retrieve the device list from the local machine */
    if (pcap_findalldevs(&alldevs, errbuf) == -1)
    {
        fprintf(stderr,"Error in pcap_findalldevs: %s\n", errbuf);
        exit(1);
    }
    extip = inet_addr("72.14.204.103"); 
    /* Not reliable 
    GetBestInterface(extip, &ifidx);  
    printf("My best guess of interface index: %d\n", (int)ifidx); 
    */
    
    if ( dev_idx == 0 ) { 
     printf("\nYou have the following IPv4 interface(s). Select one and enter the dev_index number in the -i option\n"); 
     printf("--------------------------------------------------------------------------------------------\n"); 
     printf("dev_index\t\t Attributes\n"); 
     printf("--------------------------------------------------------------------------------------------\n"); 
    }
    /* Print the list */
    i=0;
    for(d= alldevs; d != NULL; d= d->next) {
      i++;
      for (a=d->addresses;a;a=a->next) {
        if (a->addr->sa_family == AF_INET ) { 
           sip = ((struct sockaddr_in *)a->addr)->sin_addr; 
           srcip =  sip.s_addr; 
           GetMacAddress(s_mac, srcip); 
           GetGateway(sip, sgwip, &gwip); 
           GetMacAddress(d_mac, gwip); 
           if (dev_idx == i) break; 
           printf("  %d\t %s\n", i, d->name);
           if (d->description)  printf("\t %s\n",d->description);
           printf("\t IP: %s\t", inet_ntoa(sip) ); 
           printf(" MAC: %.2X-%.2X-%.2X-%.2X-%.2X-%.2X\n",
                       s_mac[0],s_mac[1],s_mac[2],s_mac[3],s_mac[4],s_mac[5]);
           printf("\t GW: %s\t", sgwip);  
           printf(" MAC: %.2X-%.2X-%.2X-%.2X-%.2X-%.2X\n",
                       d_mac[0],d_mac[1],d_mac[2],d_mac[3],d_mac[4],d_mac[5]);
        } //if
      }  // for a
      if (dev_idx == i) break; 
    }   // for d
    
    if (i == 0)
    {
        printf("\nNo interfaces found! Make sure WinPcap is installed.\n");
        return (-1);
    }

   if (dev_idx > i || dev_idx <=0 ) {
     printf("\nError: selected dev_index is over the range. Max=%d\n", i);
     usage(argv[0]); 
     return(-1); 
   }

   if (dev_idx != 0 ) {
     printf("You selected:  %d\t %s\n", i, d->name);
     printf("\t \t %s\n", d->description);
   }

   if (ip_p) { 
     printf("Making raw IP, protocol = %d\n", ip_p); 
     len = make_rawip(dbuf,
              s_mac,  d_mac,
              src_ip, dst_ip,
              ip_p, ttl,  
              dms, pay_s);
   } else { 
     len = make_tcp(dbuf,
              s_mac,  d_mac,
              src_ip, dst_ip,
              seq,    ack,
              src_prt, dst_prt,
              win, mss, ttl,
              flags, dms, pay_s);

    printf("len=%d  TCP option:", len); 
    for (i=0; i<8; i++) printf("%02x ", (u_char)dbuf[54+i]); 
    printf("\n\n");
  }

    /* Open the output device */
    if ( (fp= pcap_open(d->name,            // name of the device
                        100,                // portion of the packet to capture (only the first 100 bytes)
                        PCAP_OPENFLAG_PROMISCUOUS,  // promiscuous mode
                        1000,               // read timeout
                        NULL,               // authentication on the remote machine
                        errbuf              // error buffer
                        ) ) == NULL)
    {
        fprintf(stderr,"\nUnable to open the adapter. %s is not supported by WinPcap\n", d->name);
        return(-1);
    }

    /* We don't need any more the device list. Free it */
    pcap_freealldevs(alldevs);

    /* Send down the packet */
    for (i=0; i<len; i++) {
      printf("%02x ", (u_char)dbuf[i]);
    }
    if (pcap_sendpacket(fp, dbuf, len) != 0)
    {
        fprintf(stderr,"\nError sending the packet: %s\n", pcap_geterr(fp));
        return(-1);
    }

    return(0); 
}


//General Networking Functions
unsigned short in_checksum(unsigned short *ptr,int nbytes) {
	register long sum;
	unsigned short oddbyte;
	register short answer;

	sum=0;
	while(nbytes>1) {
		sum+=*ptr++;
		nbytes-=2;
	}
	if(nbytes==1) {
		oddbyte=0;
		*((u_char*)&oddbyte)=*(u_char*)ptr;
		sum+=oddbyte;
	}

	sum = (sum>>16)+(sum & 0xffff);
	sum = sum + (sum>>16);
	answer=(SHORT)~sum;
	
	return(answer);
}


