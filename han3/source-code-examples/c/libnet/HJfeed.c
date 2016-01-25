/*
 *  $Id: HJfeed.c,v 1.1 2003/02/15 15:39:25 lis Exp lis $
 *
 *  Build DNS type=A query packet and send to dst DNS servers 
 *  with delay specified so for 3000 resolver IPs, the bandwidth
 *  is used for less than 30kbps.
 *  Radom src IP if no -s option is given
 *  Usage: HJfeed -d delay
 */

#include <arpa/inet.h>
#include <libnet.h>
#define MAXLINES 10000

int main(int argc, char *argv[])
{
    int c, i; 
    u_char *cp;
    libnet_t *l;
    libnet_ptag_t ip;
    libnet_ptag_t udp;
    libnet_ptag_t dns;
    struct libnet_stats ls;
    u_long nround; 
    u_long src_ip, dst_ip;
    u_short src_prt, dst_prt;
    u_char opt[20];
    char errbuf[LIBNET_ERRBUF_SIZE];
    FILE *dmf, *nsf, *srcf;
    char *dms[MAXLINES], *nss[MAXLINES], *srcs[MAXLINES];
    int pay_s[MAXLINES];
    char *dmfn = "dm.txt";
    char *nsfn = "ns.txt";
    char *srcfn = "src.txt";
    char *paybuf, *pp;
    int jd = 0;
    int jn = 0;
    int js = 0;
    int kd, kn, ks;
    u_long delay = 20; /* microsec */
    u_char bt;
    char lb[80];
    char linebuf[80];
    char payld[100], tpayld[100];

/* get the specified delay */
   if ( getopt(argc, argv, "d:") == 'd')
     delay = atoi(optarg);   
       
/* Read the domains and DNS IPs, and construct DNS palyload */ 

    if( (dmf = fopen(dmfn, "r")) == NULL) {
      fprintf(stderr, "Error opening file %s \n", dmfn); 
      exit(1);
    } /* end if */ 
 
    if( (nsf = fopen(nsfn, "r")) == NULL) {
      fprintf(stderr, "Error opening file %s \n", nsfn); 
      exit(1);
    } /* end if */ 
 
    printf("Reading domain names ...\n");
    while ((jd < MAXLINES) && (fgets(lb, 80, dmf)!=0) )  {
       kd = 0; 
       kn = 0;
       while (lb[kd] != '\0') {        /* get rid of space and CR */
        if ((lb[kd] != ' ') && (lb[kd] != '\n')) {
           linebuf[kn] = lb[kd];
           kn++;
        }  /* end if */
        kd++; 
       } /* end while */
       linebuf[kn] = '\0'; 
       paybuf = linebuf;
       if (kn != 0) { /* not empty line */ 
         /* convert domain name into DNS query message payload */
         *payld = '\0'; 
         while ( (pp = strchr(paybuf, '.') )!=NULL ) { 
            *pp++ = '\0';
            sprintf(tpayld, "%c%s", strlen(paybuf), paybuf); 
            strcat(payld, tpayld); 
            paybuf = pp; 
         }  /* end while */ 
            sprintf(tpayld, "%c%s", strlen(paybuf), paybuf); 
            strcat(payld, tpayld); 

         *payld = '\x3f';  /* new payload **/
         *(payld + 1) = '\x00'; 
          
         dms[jd] = (char *)malloc(strlen(payld)+10);  /* alloc mem */
         strcpy(dms[jd], payld); 
         pay_s[jd] = strlen(dms[jd]) + 1; 
         *(dms[jd] + pay_s[jd]) = '\x00'; 
         *(dms[jd] + pay_s[jd]+1) = '\x01';  /* query type=A */
         *(dms[jd] + pay_s[jd]+2) = '\x00'; 
         *(dms[jd] + pay_s[jd]+3) = '\x01'; 
         *(dms[jd] + pay_s[jd]+4) = '\x00'; 
         pay_s[jd] += 4; 
         /* ===== for message body debugging 
         for (kn=0; kn < pay_s[jd]; kn++) { 
            bt = *(dms[jd] + kn); 
            printf(" %02X", bt); 
         }  
         printf("\n");
         === */
     
         jd++;
       } /* end if  */
    } /* end while  */
 
    printf("Total domains: %d\n", jd); 
    fclose(dmf);
   
    jn = 0; 
    printf("Reading resolver IPs ... \n");
    while ((jn < MAXLINES) && (fgets(lb, 80, nsf)!=0) )  {
       kd = 0;
       kn = 0;
       while (lb[kd] != '\0') {        /* get rid of space and CR */
        if ((lb[kd] != ' ') && (lb[kd] != '\n')) {
           linebuf[kn] = lb[kd];
           kn++;
        }  /* end if */
        kd++;
       } /* end while */
       linebuf[kn] = '\0';
       if (kn != 0) { /* not empty line */
         nss[jn] = (char *)malloc(strlen(linebuf)+1);
         strcpy(nss[jn++], linebuf);
         /*  printf("%s\n", nss[jn-1]); */
       } /* end if */  
    } /* end while  */
  
    printf("Total DNS IPs: %d\n", jn);
    fclose(nsf);

nround = 0; 
while(1) {   /* non-stop sending ... */
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
    udp = 0; 
    dns = 0; 
    ip = 0;

 for(kd=0; kd < jd; kd++) {  /* loop over all domains */ 
   for(kn=0; kn < jn; kn++) {  /* loop over all DNS IPs */ 
     dst_ip = inet_addr(nss[kn]);
     for(ks=0; ks < 1000; ks++) {  /* send 1000 queries out */ 
      src_ip = libnet_get_prand(LIBNET_PRu32);    /* Random source ip */
      src_prt = libnet_get_prand(LIBNET_PRu16);    /* Random source port */
      dst_prt = 53;			         /* DNS */		 

/* Building DNS query */
  dns = libnet_build_dnsv4(
    libnet_get_prand(LIBNET_PRu16),         /* ID */
    0x0100,            /* flags: recursive query */
    0x0001,            /* number of questions */
    0x0000,            /* number of answer resource records */
    0x0000,            /* number of authority resource records */
    0x0000,            /* number of additional resource records */
    dms[kd],           /* pointer to packet data (or NULL) */
    pay_s[kd],             /* payload length */
    l,         /* libnet context pointer */
    dns       /* packet id */
    );

        if (dns == -1) {
            fprintf(stderr, "Can't build DNS header: %s\n", libnet_geterror(l));
            goto bad;
        }

/* Building UDP */
        udp = libnet_build_udp(
            src_prt,                                /* source port */
            dst_prt,                            /* destination port */
            LIBNET_UDP_H + pay_s[kd] + LIBNET_DNSV4_H,   /* packet length */
            0,                                      /* checksum */
            NULL,                                /* payload */
            0,                              /* payload size */
            l,                                      /* libnet handle */
            udp);                                   /* libnet id */

        if (udp == -1) {
            fprintf(stderr, "Can't build UDP header: %s\n", libnet_geterror(l));
            goto bad;
        }

/* Building IP */
            
            ip = libnet_build_ipv4(
                LIBNET_IPV4_H + pay_s[kd] + LIBNET_UDP_H + LIBNET_DNSV4_H, /* length */
                0x00,                                          /* TOS */
                0,                                            /* IP ID */
                0x0000,                                          /* IP Frag */
                64,                                         /* TTL */
                IPPROTO_UDP,                                /* protocol */
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
            /* printf("Wrote %d byte UDP packet; check the wire.\n", c); */
        }
 
   /* printf("Delaying %d microsec between each packet ... \n", delay); */
     } /* end for (ks=0; ks < 1000; ks++)  */ 
     sleep(delay);  /* slow down to conserve bandwidth */
   } /* end for, kn */ 
}  /* end for kd */

    libnet_stats(l, &ls);
    fprintf(stderr, "Packets sent:  %ld\n"
                    "Packet errors: %ld\n"
                    "Bytes written: %ld\n",
                    ls.packets_sent, ls.packet_errors, ls.bytes_written);
    libnet_destroy(l);

 nround++;
 printf("====== Round %ld sent ================\n", nround); 
}  /* end of while loop */
    return (EXIT_SUCCESS);
bad:
    libnet_destroy(l);
    return (EXIT_FAILURE);
}

