/*
 *  $Id: sd_content.c,v 1.1 2003/01/01 02:37:10 yudong Exp yudong $
 *  Usage: sd_content -s src_IP -d dst_IP -p dst_port -f in_file
 *  Send a block of characters ( <1400 ) out in a TCP segment
 * and sniff the response
 * The block is in the file "in_file", default is "kw.txt". 
 *
 */

#include <libnet.h>
#define MAXLINES 10000
#define MAXTEXT 1400

int main(int argc, char *argv[])
{
    int c, i; 
    u_char *cp;
    libnet_t *l;
    libnet_ptag_t ip;
    libnet_ptag_t tcp;
    struct libnet_stats ls;
    u_long src_ip, dst_ip;
    u_short src_prt, dst_prt;
    u_char opt[20];
    char errbuf[LIBNET_ERRBUF_SIZE];
    FILE *dmf; 
    char *dms; 
    int pay_s;
    char *dmfn = "kw.txt";
    char *paybuf, *pp;
    u_char bt;
    char linebuf[MAXTEXT];

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

    dst_ip = libnet_get_prand(LIBNET_PRu32);
    src_ip = libnet_get_prand(LIBNET_PRu32);
    src_prt = libnet_get_prand(LIBNET_PRu16);
    dst_prt = 80;			         /* http */		 

    while ((c = getopt(argc, argv, "d:s:p:f:")) != EOF)
    {
        switch (c)
        {
         case 'd':
           if ((dst_ip = libnet_name2addr4(l, optarg, LIBNET_RESOLVE)) == -1)
             {
                fprintf(stderr, "Bad destination IP address: %s\n", optarg);
                exit(EXIT_FAILURE);
             }
           break;
         case 's':
           if ((src_ip = libnet_name2addr4(l, optarg, LIBNET_RESOLVE)) == -1)
             {
               fprintf(stderr, "Bad source IP address: %s\n", optarg);
               exit(EXIT_FAILURE);
             }
           break;
         case 'p':
           dst_prt = (u_short)atoi(optarg);
           printf("Dest port: %d\n", dst_prt); 
           break;
         case 'f':
           dmfn = optarg; 
           printf("File name: %s\n", dmfn); 
           break;
         default:
                exit(EXIT_FAILURE);
        }
    }



/* Read the keywords, dest IPs, src IPs , and construct http payload */ 

    if( (dmf = fopen(dmfn, "r")) == NULL) {
      fprintf(stderr, "Error opening the content file %s \n", dmfn); 
      exit(1);
    } /* end if */ 
 
    printf("Reading content ...\n");
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
         /* ===== for message body debugging  */
         printf("Content read:\n");
         for (i=0; i < pay_s; i++) { 
            bt = *(dms + i); 
            printf(" %02X", bt); 
         }  
         printf("\n");
         /*=== */
     
   


/* Building TCP */
        tcp = libnet_build_tcp(
            src_prt,                                /* source port */
            dst_prt,                            /* destination port */
            libnet_get_prand(LIBNET_PRu32),
            libnet_get_prand(LIBNET_PRu32),
            0x18,            /* (PSH, ACK) */
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

   /*  usleep(20); slow down to conserve bandwidth */

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

