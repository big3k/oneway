/*
 *  $Id: lpa.c,v 1.1 2003/03/24 21:04:17 yudong Exp yudong $
 *  Built upon win-pa.c,v 1.6, designed to use with a sniffer 
 *  for fast port scan 
 *  Program to send data in TCP segs. 
 *  Usage:  
 *  lpa -s src_IP -o src_port -d dst_IP -p dst_port -n nhosts -t delay
 *      -M seq# -L ack# -A -P -S -F -R -v -T "pay load text"
 *  Send a block of characters 
 * The text block is in the file "dat_file". 
 *  To compile under cygwin:
 *   1. go under ${HOME}/Libnet-latest/sample 
 *   2.  gcc -I. -I../include -g -O2 -Wall -mno-cygwin -c win-pa.c
 *   3.  gcc -g -O2 -Wall -mno-cygwin -o win-pa win-pa.o  
 *            ../src/libnet.a -lwpcap -lpacket -lws2_32
 * Customization for L.Q. 
 *> 1. -n can take any number;
 *> 2. a new parameter to control how many hosts to scan between each delay;
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <libnet.h> 

#define MAXTEXT 1400

void usage(char *);

int main(int argc, char *argv[])
{
    int c, i, j, m=1, n, count=0; 
    u_long  k; 
    u_char *cp;
    libnet_t *l;
    libnet_ptag_t ip;
    libnet_ptag_t tcp, t_op;
    struct libnet_stats ls;
    u_long src_ip, dst_ip;
    u_long seq, ack; 
    u_short src_prt, dst_prt, win;
    u_short len; 
    u_char flags = 0x00;      /* tcp flag */ 
    u_char opt[20];
    u_char ttl='\xf0';  /* TTL of IP */
    char errbuf[LIBNET_ERRBUF_SIZE];
    char *dev ="eth0";   /* default if */ 
    struct in_addr addr;        

    FILE *dmf; 
    char *dms = NULL; 
    int pay_s = 0, delay = 0, nhosts = 1;
    char *dmfn = NULL; 
    char *paybuf; 
    u_char bt;
    char linebuf[MAXTEXT];
    int vbflag = 0;  /* verbose flag */ 
    int synflag = 0;  /* SYN flag.  */ 


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

    dst_ip = libnet_get_prand(LIBNET_PRu32);
    src_ip = libnet_get_prand(LIBNET_PRu32);
    src_prt = libnet_get_prand(LIBNET_PRu16);
    dst_prt = 80;			         /* http */		 
    win = libnet_get_prand(LIBNET_PRu16); 
    seq = libnet_get_prand(LIBNET_PRu32);
    ack = libnet_get_prand(LIBNET_PRu32);

    while ((c = getopt(argc, argv, "d:o:s:p:T:n:m:t:M:L:w:d:APRSFv")) != EOF)
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
         case 'o':
           src_prt = (u_short)atoi(optarg);
           break;
         case 'p':
           dst_prt = (u_short)atoi(optarg);
           break;
         case 't':
           delay = atoi(optarg);
           break;
         case 'n':
           nhosts = atoi(optarg);
           break;
         case 'm':
           m = atoi(optarg);
           break;
         case 'T':
           paybuf = optarg; 
           dms = (char *)malloc(strlen(paybuf)+1);  /* alloc mem */
           strcpy(dms, paybuf); 
           pay_s = strlen(dms); 
           break;
         case 'M':
           seq = (u_long)strtoul(optarg, NULL, 10); 
           break;
         case 'L':
           ack = (u_long)strtoul(optarg, NULL, 10); 
           break;
         case 'w':
           win = (u_short)atoi(optarg); 
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
         case 'F':
           flags = flags | 0x01;  /* FIN */
           break;
         case 'v':
           vbflag = 1;  /* Verbose flag */
           break;
         default:
                usage(argv[0]);
                exit(EXIT_FAILURE);
        }
    }

           addr.s_addr = dst_ip; 
      if (vbflag) { 
           printf("Dest IP: %s\n", inet_ntoa(addr)); 
           printf("Dest port: %d\n", dst_prt); 
               /**  printf("Content file name: %s\n", dmfn); 
               printf("Sniffing device: %s\n", dev); */
      } 


/* Read the keywords, dest IPs, src IPs , and construct http payload */ 
   if (dmfn != NULL) { /* -- if -E option is given */ 
    if( (dmf = fopen(dmfn, "r")) == NULL) {
      fprintf(stderr, "Error opening the content file %s \n", dmfn); 
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
   } /* ---end if */ 

         if(vbflag) { /* ===== for message body debugging */
           printf("Content read:\n");
            for (i=0; i < pay_s; i++) { 
             bt = *(dms + i); 
             printf(" %02X", bt); 
            }  
            printf("\n");
         }
  
    len =  LIBNET_TCP_H + pay_s;            /* packet length */
/*  no factorize nhosts */ 

    for (k=0; k< nhosts; k++) { 
       // printf("%ld\n", k); 
/* Building TCP options for SYN */
        if(synflag) { 
            ack =0; 
            len =  LIBNET_TCP_H + 12 + pay_s;    /* 12 more for syn */
            t_op = libnet_build_tcp_options(
              "\x02\x04\x05\xb4\x01\x01\x04\x02\x01\x00\x00\x00",
              12,
              l,
              t_op);
        }

/* Building TCP */
        tcp = libnet_build_tcp(
            src_prt,                                /* source port */
            dst_prt,                            /* destination port */
            seq,            /* seq libnet_get_prand(LIBNET_PRu32),*/
            ack,            /* ack libnet_get_prand(LIBNET_PRu32), */
            flags,            /* TCP flags */
            win,            /* win size libnet_get_prand(LIBNET_PRu16),  */
            0,                                      /* checksum */
            0,                                      /* urgent pointer */
            len,           /* packet length */
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
                dst_ip + htonl(k),
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
            /* goto bad; */
        } else {
            /* printf("Wrote %d byte TCP packet; check the wire.\n", c); */
        }

     count++;
     if(count % m == 0) usleep(delay); 

   }   /* end for k */


  /* === debugging info ====*/
    libnet_stats(l, &ls);
    if(vbflag) { 
      fprintf(stderr, "Packets sent:  %ld\n"
                    "Packet errors: %ld\n"
                    "Bytes written: %ld\n",
                    ls.packets_sent, ls.packet_errors, ls.bytes_written);
    } 
    libnet_destroy(l);

    return (EXIT_SUCCESS);
bad:
    libnet_destroy(l);
    return (EXIT_FAILURE);
}

void
usage(char *name)
{
    fprintf(stderr, "usage %s -s sIP -o sport -d starting_IP -n nhosts -m n_betweendelay -p dport -t delay\n", name);
    fprintf(stderr, " -w win -T \"payload text\" -M seq -L ack -A|P|R|S|F -v \n");
    fprintf(stderr, " nhosts can be any number; delay (us) is for every 256 packets \n");
}
