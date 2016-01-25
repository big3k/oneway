/*
 *  $Id$ 
 *
 *  Send a Syn first, ACK second, then  
 *  send an http query segment with keywords in it  
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
    libnet_ptag_t tcp, t_op;
    struct libnet_stats ls;
    u_long nround, seqn; 
    u_long src_ip, dst_ip;
    u_short src_prt, dst_prt;
    u_char opt[20];
    char errbuf[LIBNET_ERRBUF_SIZE];
    FILE *dmf, *nsf, *srcf;
    char *dms[MAXLINES], *nss[MAXLINES], *srcs[MAXLINES];
    int pay_s[MAXLINES];
    char *dmfn = "kw.txt";
    char *nsfn = "dst.txt";
    char *srcfn = "src.txt";
    char *paybuf, *pp;
    int jd = 0;
    int jn = 0;
    int js = 0;
    int kd, kn;
    u_char bt;
    char lb[80];
    char linebuf[80];
    char payld[100], tpayld[100];

/* Read the keywords, dest IPs, src IPs , and construct http payload */ 

    if( (dmf = fopen(dmfn, "r")) == NULL) {
      fprintf(stderr, "Error opening key word file %s \n", dmfn); 
      exit(1);
    } /* end if */ 
 
    if( (nsf = fopen(nsfn, "r")) == NULL) {
      fprintf(stderr, "Error opening file %s \n", nsfn); 
      exit(1);
    } /* end if */ 
 
    if( (srcf = fopen(srcfn, "r")) == NULL) {
      fprintf(stderr, "Error opening file %s \n", srcfn); 
      exit(1);
    } /* end if */ 
 
    printf("Reading key words ...\n");
    while ((jd < MAXLINES) && (fgets(lb, 80, dmf)!=0) )  {
       kd = 0; 
       kn = 0;
       while (lb[kd] != '\0') {        /* get rid of space and CR */
           linebuf[kn++] = lb[kd++];
       } /* end while */
       linebuf[kn] = '\0'; 
       paybuf = linebuf;
       if (kn != 0) { /* not empty line */ 
         /* convert line into http query  payload */
         dms[jd] = (char *)malloc(strlen(paybuf)+1);  /* alloc mem */
         strcpy(dms[jd], paybuf); 
         pay_s[jd] = strlen(dms[jd]); 
         /* ===== for message body debugging  */
         for (kn=0; kn < pay_s[jd]; kn++) { 
            bt = *(dms[jd] + kn); 
            printf(" %02X", bt); 
         }  
         printf("\n");
         /*=== */
     
         jd++;
       } /* end if  */
    } /* end while  */
 
    printf("Total http requests: %d\n", jd); 
    fclose(dmf);
   
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
    printf("Reading dst IPs ... \n");
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
  
    printf("Total dst IPs: %d\n", jn);
    fclose(nsf);

nround = 0; 
/* while(1) {    non-stop sending ... */
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
    t_op = 0; 

 for(kd=0; kd < jd; kd++) {  /* loop over all http reqs */ 

   for(kn=0; kn < jn; kn++) {  /* loop over all dst IPs */ 

   if ((dst_ip = libnet_name2addr4(l, nss[kn], LIBNET_RESOLVE)) == -1) {
        fprintf(stderr, "Bad dest IP address: %s\n", nss[kn]);
        exit(EXIT_FAILURE);
   }  /* end if */

    /* printf("feeding %s for keyword %s ...\n", nss[kn], dms[kd]);  */
    i = (int)(libnet_get_prand(LIBNET_PRu16)/65535.0*(js-1));  /* Randsrc ip */
    src_ip = libnet_name2addr4(l, srcs[i], LIBNET_RESOLVE); /* src ip */ 
    printf("using source IP %s ...\n", srcs[i]); 
    src_prt = libnet_get_prand(LIBNET_PRu16);    /* Random source port */
    dst_prt = 80;			         /* http */		 

/* Building TCP SYN */
        seqn = libnet_get_prand(LIBNET_PRu32);   /* sequence number */ 
        t_op = libnet_build_tcp_options(
              "\x02\x04\x05\xb4\x01\x01\x04\x02\x01\x00\x00\x00", 
              12, 
              l,
              t_op); 
              
        tcp = libnet_build_tcp(
            src_prt,                                /* source port */
            dst_prt,                            /* destination port */
            seqn,                              /* seq num */
            0,                                  /* ack num */ 
            TH_SYN,            /* SYN */
            libnet_get_prand(LIBNET_PRu16),     /* window size */ 
            0,                                      /* checksum */
            0,                                      /* urgent pointer */
            LIBNET_TCP_H + 12,           /* packet length */
            NULL,                                /* payload */
            0,                              /* payload size */
            l,                                      /* libnet handle */
            tcp);                                   /* libnet id */

        if (tcp == -1) {
            fprintf(stderr, "Can't build TCP header: %s\n", libnet_geterror(l));
            goto bad;
        }

/* Building IP */
            
            ip = libnet_build_ipv4(
                LIBNET_IPV4_H + 12+ LIBNET_TCP_H, /* length */
                0x00,                                          /* TOS */
                0,                                            /* IP ID */
                0x4000,                                          /* IP Frag */
                64,                                         /* TTL */
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
            /* printf("Wrote %d byte TCP packet; check the wire.\n", c); */
        }

   sleep(2);  /* slow down to conserve bandwidth */

/* BUILD ACK */
        tcp = libnet_build_tcp(
            src_prt,                                /* source port */
            dst_prt,                            /* destination port */
            seqn + 1,                              /* seq num */
            0,                                  /* ack num */ 
            TH_ACK,            /* SYN */
            libnet_get_prand(LIBNET_PRu16),     /* window size */ 
            0,                                      /* checksum */
            0,                                      /* urgent pointer */
            LIBNET_TCP_H,           /* packet length */
            NULL,                                /* payload */
            0,                              /* payload size */
            l,                                      /* libnet handle */
            tcp);                                   /* libnet id */

        if (tcp == -1) {
            fprintf(stderr, "Can't build TCP header: %s\n", libnet_geterror(l));
            goto bad;
        }

/* Building IP */
            
            ip = libnet_build_ipv4(
                LIBNET_IPV4_H + LIBNET_TCP_H, /* length */
                0x00,                                          /* TOS */
                0,                                            /* IP ID */
                0x4000,                                          /* IP Frag */
                64,                                         /* TTL */
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
            /* printf("Wrote %d byte TCP packet; check the wire.\n", c); */
        }

   usleep(500);  /* slow down to conserve bandwidth */

/* Building TCP */
        tcp = libnet_build_tcp(
            src_prt,                                /* source port */
            dst_prt,                            /* destination port */
            seqn + 1, 
            libnet_get_prand(LIBNET_PRu32),
            0x18,            /* (PSH, ACK) */
            libnet_get_prand(LIBNET_PRu16),     /* window size */ 
            0,                                      /* checksum */
            0,                                      /* urgent pointer */
            LIBNET_TCP_H + pay_s[kd],           /* packet length */
            dms[kd],                                /* payload */
            pay_s[kd],                              /* payload size */
            l,                                      /* libnet handle */
            tcp);                                   /* libnet id */

        if (tcp == -1) {
            fprintf(stderr, "Can't build TCP header: %s\n", libnet_geterror(l));
            goto bad;
        }

/* Building IP */
            
            ip = libnet_build_ipv4(
                LIBNET_IPV4_H + pay_s[kd] + LIBNET_TCP_H, /* length */
                0x00,                                          /* TOS */
                0,                                            /* IP ID */
                0x4000,                                          /* IP Frag */
                64,                                         /* TTL */
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
            /* printf("Wrote %d byte TCP packet; check the wire.\n", c); */
        }

   usleep(20);  /* slow down to conserve bandwidth */
   } /* end for */ 
}  /* end for */

    libnet_stats(l, &ls);
    fprintf(stderr, "Packets sent:  %ld\n"
                    "Packet errors: %ld\n"
                    "Bytes written: %ld\n",
                    ls.packets_sent, ls.packet_errors, ls.bytes_written);
    libnet_destroy(l);

 nround++;
 printf("====== Round %ld sent ================\n", nround); 
/* }   end of while loop */
    return (EXIT_SUCCESS);
bad:
    libnet_destroy(l);
    return (EXIT_FAILURE);
}

