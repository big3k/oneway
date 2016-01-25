/*
 *  $Id$
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

u_long cip, sip, pip, qip; /* client, server, ping, pong */ 
u_long pqseq0, scseq0;    /* seq/ack numbers */
u_long pqseq, scseq;    /* seq/ack numbers */
u_long pqack, scack;    /* seq/ack numbers */
u_short cport = 12768;   /* client port */
u_short sport = 80;      /* server port */
u_short pport = 12768;   /* client port */
u_short qport = 12768;      /* server port */
u_short nhosts, pay_0 = 0;
u_char cttl = 12, sttl = 18;  /* TTL of IP */
char pqmsg[MAXTEXT];   /* pointer to the commnad struct */ 
char *dms = NULL;  /* server payload */
char *dmc = NULL;  /* client payload */
char *ptmp = NULL;  /* client payload */
int c; 

struct cmdpayload {   // the command payload structure
  u_short nhosts;     // number of hosts to span across
  u_short ttl;          // ttl with 2bytes for alignment
  u_long pip;     // ping ip 
  u_long cip;     // client ip 
} cmd0;


int main(int argc, char *argv[])
{
    int n, i; 
    int cmdsize = sizeof(struct cmdpayload); 
    char errbuf[LIBNET_ERRBUF_SIZE];
    /* pcap variables */
    char perrbuf[PCAP_ERRBUF_SIZE];
    //unicode strings (winnt)
    char *dev ="eth0";   /* default if */
    char filter[MAXTEXT]; 
    pcap_t* descr;
    struct bpf_program fp;      /* hold compiled program     */
    bpf_u_int32 maskp;          /* subnet mask               */
    bpf_u_int32 netp;           /* ip                        */
    int devnum=0, Open; 
    void usage(char *);

    FILE *fclt; /* file handles for server and client content */ 
    int pay_s = 0, pay_c = 0;
    char *dmfc = NULL, *dmfs = NULL;  /* file names */
    u_char bt;
    char linebuf[MAXTEXT];

    while ((n = getopt(argc, argv, "s:c:p:q:i:n:C:S:")) != EOF)
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
    while ((i < MAXTEXT - cmdsize ) && ( (c=fgetc(fclt)) != EOF ) )
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

    /* construct ping payload */
    cmd0.nhosts = nhosts; 
    cmd0.pip = pip; 
    cmd0.cip = cip; 
    cmd0.ttl = cttl; 
    ptmp = (char *)&cmd0;  /* get the pointer */
        printf("ptmp read: %d\n", cmdsize);
         for (i=0; i < cmdsize; i++) {
           pqmsg[i] = *(ptmp + i); 
         }
         for (i=0; i <= strlen(dmc); i++) {
           pqmsg[i+cmdsize] = *(dmc + i); 
         }

        printf("Content read:\n");
         for (i=0; i <= strlen(dmc) + cmdsize; i++) {
            printf(" %02X", pqmsg[i]);
         }
         printf("\n");

    return (EXIT_SUCCESS);
bad:
    libnet_destroy(l);
    return (EXIT_FAILURE);
}

void
usage(char *name)
{
    printf("usage %s -s svr_IP -c clt_IP -p ping_IP -q pong_IP \n", name);
    printf("-i if -n nhosts -E data_file \n");  
}


