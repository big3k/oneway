/*
 *  $Id: mytestudp.c,v 1.2 2002/11/01 04:39:27 yudong Exp yudong $
 *
 *  Build a DNS type=any query packet
 *
 */

#include <libnet.h>
#define MAXLINES 10000

int main(int argc, char *argv[])
{
    int c, i, build_ip;
    u_char *cp;
    libnet_t *l;
    libnet_ptag_t ip;
    libnet_ptag_t udp;
    libnet_ptag_t dns;
    char *payload;
    u_short payload_s;
    struct libnet_stats ls;
    u_long src_ip, dst_ip;
    u_short src_prt, dst_prt;
    u_char opt[20];
    char errbuf[LIBNET_ERRBUF_SIZE];
    FILE *dmf, *nsf;
    char *dms[MAXLINES], *nss[MAXLINES];
    int pay_s[MAXLINES];
    char *dmfn = "dm.txt";
    char *nsfn = "ns.txt";
    char *paybuf, *pp;
    int jd = 0;
    int jn = 0;
    int kd, kn;
    u_char bt;
    char lb[80];
    char linebuf[80];
    char payld[100], tpayld[100];

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
          
         dms[jd] = (char *)malloc(strlen(payld)+1);  /* alloc mem */
         strcpy(dms[jd], payld); 
         pay_s[jd] = strlen(dms[jd]) + 1; 
         *(dms[jd] + pay_s[jd]) = '\x00'; 
         *(dms[jd] + pay_s[jd]+1) = '\xff';  /* query type=any */
         *(dms[jd] + pay_s[jd]+2) = '\x00'; 
         *(dms[jd] + pay_s[jd]+3) = '\x01'; 
         pay_s[jd] += 4; 
         for (kn=0; kn < pay_s[jd]; kn++) { 
            bt = *(dms[jd] + kn); 
            printf(" %02X", bt); 
         } 
         printf("\n");
     
         jd++;
       } /* end if  */
    } /* end while  */
 
    printf("Total domains: %d\n", jd); 
    fclose(dmf);
   
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
         printf("%s-\n", nss[jn-1]);
       } /* end if */   
    } /* end while  */
   
    printf("Total DNS IPs: %d\n", jn); 
    fclose(nsf);
    

    for(kn=0; kn < jn; kn++) {  /* loop over all DNS IPs */ 
      for(kd=0; kd < jd; kd++) {  /* loop over all DNS IPs */ 


    char *destIP = "192.168.1.100";
    char *qryDM = "www.nasa.gov"; 
    char *testpayload = "\x03\x77\x77\x77\x06\x79\x61\x68\x6f\x6f\x6f\x03"
        "\x63\x6f\x6d\x00\x00\xff\x00\x01";  /* www.yahooo.com */
    
    printf("Querying %s for domain %s ...\n", destIP, qryDM); 

    /*
     *  Initialize the library.  Root priviledges are required.
     */
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

    src_ip = libnet_get_prand(LIBNET_PRu32);     /* Random src ip */     
    src_prt = libnet_get_prand(LIBNET_PRu16);	 /* Random source port */
    dst_prt = 53;			         /* DNS */		 
    payload = testpayload; 
    /*  payload_s = strlen(payload); */
    payload_s = 20;
    udp = 0;

    if ((dst_ip = libnet_name2addr4(l, destIP, LIBNET_RESOLVE)) == -1)
                {
                    fprintf(stderr, "Bad destination IP address: %s\n", destIP);
                    exit(EXIT_FAILURE);
                }

/* Building DNS query */
  dns = libnet_build_dnsv4(
    libnet_get_prand(LIBNET_PRu16),         /* ID */
    0x0100,            /* flags: recursive query */
    0x0001,            /* number of questions */
    0x0000,            /* number of answer resource records */
    0x0000,            /* number of authority resource records */
    0x0000,            /* number of additional resource records */
    payload,           /* pointer to packet data (or NULL) */
    payload_s,             /* payload length */
    l,         /* libnet context pointer */
    0       /* packet id */
    );

        if (dns == -1)
        {
            fprintf(stderr, "Can't build DNS header: %s\n", libnet_geterror(l));
            goto bad;
        }


/* Building UDP */
        udp = libnet_build_udp(
            src_prt,                                /* source port */
            dst_prt,                            /* destination port */
            LIBNET_UDP_H + payload_s + LIBNET_DNSV4_H,   /* packet length */
            0,                                      /* checksum */
            NULL,                                /* payload */
            0,                              /* payload size */
            l,                                      /* libnet handle */
            udp);                                   /* libnet id */
        if (udp == -1)
        {
            fprintf(stderr, "Can't build UDP header: %s\n", libnet_geterror(l));
            goto bad;
        }

/* Building IP */

            ip = libnet_build_ipv4(
                LIBNET_IPV4_H + payload_s + LIBNET_UDP_H + LIBNET_DNSV4_H, /* length */
                0x10,                                          /* TOS:DNS */
                libnet_get_prand(LIBNET_PRu16),             /* IP ID */
                0,                                          /* IP Frag */
                64,                                         /* TTL */
                IPPROTO_UDP,                                /* protocol */
                0,                                          /* checksum */
                src_ip,
                dst_ip,
                NULL,                                       /* payload */
                0,                                          /* payload size */
                l,                                          /* libnet handle */
                0);                                         /* libnet id */
            if (ip == -1)
            {
             fprintf(stderr, "Can't build IP header: %s\n", libnet_geterror(l));
             goto bad;
            }

/*  Write it to the wire. */
        c = libnet_write(l);
        if (c == -1) {
            fprintf(stderr, "Write error: %s\n", libnet_geterror(l));
            goto bad;
        } else {
            printf("Wrote %d byte UDP packet; check the wire.\n", c);
        }

    libnet_stats(l, &ls);
    fprintf(stderr, "Packets sent:  %ld\n"
                    "Packet errors: %ld\n"
                    "Bytes written: %ld\n",
                    ls.packets_sent, ls.packet_errors, ls.bytes_written);
    libnet_destroy(l);

    return (EXIT_SUCCESS);
bad:
    libnet_destroy(l);
    return (EXIT_FAILURE);
}

