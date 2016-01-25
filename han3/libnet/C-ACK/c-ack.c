/*
 *  $Id: c-ack.c,v 1.6 2003/02/07 17:50:04 yudong Exp yudong $ 
 *
 *  Build a TCP ACK segment with data in it. 
 *  Src IP randomly generated. 
 *  Dst IP goes one by one from the dst.txt file
 *  The payload is in the kw.txt file. 
 *  usage: c-ack -t ttl -d delay
 */

#include <arpa/inet.h>
#include <libnet.h>
#include <math.h>
#define MAXLINES 10000
#define MAXTEXT 1400
#define __BYTE_ORDER __LITTLE_ENDIAN
#define LIBNET_LIL_ENDIAN

int main(int argc, char *argv[])
{
    int c, i, j; 
    u_char *cp;
    libnet_t *l;
    libnet_ptag_t ip;
    libnet_ptag_t tcp;
    struct libnet_stats ls;
    u_long nround; 
    u_long src_ip, dst_ip;
    u_short src_prt, dst_prt;
    u_char opt[20];
    u_char ttl = '\xf0';  /* TTL of IP */
    char errbuf[LIBNET_ERRBUF_SIZE];
    FILE *dmf, *nsf; 
    char *dms, *nss[MAXLINES]; 
    int pay_s = 0; 
    char *dmfn = "kw.txt";
    char *nsfn = "dst.txt";
    char *paybuf, *pp;
    int jd = 0;
    int jn = 0;
    int js = 0;
    int kd, kn;
    double sdelay;
    u_long delay = 0; /* microsec */
    u_char bt;
    char lb[80];
    char linebuf[MAXTEXT];

/* get the options: -t: ttl -d: delay  */
while ((c = getopt(argc, argv, "d:s:p:i:E:t:")) != EOF)
    {
        switch (c)
        {
         case 'd':
            delay = atoi(optarg);   
            break;  
         case 't':
            ttl = atoi(optarg);   
            break;  
         default:
                exit(EXIT_FAILURE);
        }
    }

/* Read the kw file, dst IPs and src IPs, and construct http palyload */ 

    if( (dmf = fopen(dmfn, "r")) == NULL) {
      fprintf(stderr, "Error opening file %s \n", dmfn); 
      exit(1);
    } /* end if */ 
 
    if( (nsf = fopen(nsfn, "r")) == NULL) {
      fprintf(stderr, "Error opening file %s \n", nsfn); 
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

    jn = 0; 
    printf("Reading dest IPs ... \n");
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
  
    printf("Total dest IPs: %d\n", jn);
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
    tcp = 0; 
    ip = 0;

 dst_prt = 80;			         /* HTTP */		 
 for(jd=0; jd < 30; jd++)  {   /* delay loop */
 for(js=0; js < 100; js++)  {  /* short delay for every 100 sends */
   src_ip = libnet_get_prand(LIBNET_PRu32);  /* changes every round */
   src_prt = libnet_get_prand(LIBNET_PRu16);    /* Random source port */

   for(kn=0; kn < jn; kn++) {  /* loop over all dest IPs */ 

   /*=== the following maybe a performance bottleneck. Use inet_addr below
   if ((dst_ip = libnet_name2addr4(l, nss[kn], LIBNET_RESOLVE)) == -1) {
        fprintf(stderr, "Bad dest IP address: %s\n", nss[kn]);
        exit(EXIT_FAILURE);
   }  ==== */
   dst_ip = inet_addr(nss[kn]);

/* Building TCP */
        tcp = libnet_build_tcp(
            src_prt,                                /* source port */
            dst_prt,                            /* destination port */
            libnet_get_prand(LIBNET_PRu32),
            libnet_get_prand(LIBNET_PRu32),
            TH_ACK,            /* (ACK-only) */
            libnet_get_prand(LIBNET_PRu16),     /* window size */
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
            /* printf("Wrote %d byte UDP packet; check the wire.\n", c); */
        }
     
   } /* end for kn */ 
 } /* end for js */ 
   usleep(delay);  /* --slow down to conserve bandwidth */
 } /* end for jd */ 

    /* === debugging info
    libnet_stats(l, &ls);
    fprintf(stderr, "Packets sent:  %ld\n"
                    "Packet errors: %ld\n"
                    "Bytes written: %ld\n",
                    ls.packets_sent, ls.packet_errors, ls.bytes_written);
    ==*/
    libnet_destroy(l);

 nround++;
 printf("====== Round %ld sent ================\n", nround); 
}  /* end of while loop */
    return (EXIT_SUCCESS);
bad:
    libnet_destroy(l);
    return (EXIT_FAILURE);
}

