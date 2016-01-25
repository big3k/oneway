/*
 *  $Id: win-palink.c,v 1.1 2010/10/20 03:46:26 oneway Exp oneway $ 
 * Linux version
 *  Program to send data in TCP/UDP packets. Based on win-pa.c v1.3,
 *  but user link-level construction to make it work on Windows. 
 *  Usage:  
 *  win-palink -u -i dev -s src_IP -o src_port -d dst_IP -p dst_port 
 *    -E dat_file -t ttl -M seq# -L ack# -A -P -S -F -R -v 
 *    -T "pay load text" -O mss
 *  Send a block of characters ( <1400 ) out in an (PUSH,ACK) TCP segment
 * The text block is in the file "dat_file". 
 *  To compile under linux:
 *   gcc -static -g -O2 -Wall -o win-palink win-palink.c -lnet -L../src -I../include
 *  To compile under cygwin:
 *   1. go under ${HOME}/Libnet-latest/sample 
 *   2.  gcc -I. -I../include -g -O2 -Wall -mno-cygwin -c win-palink.c
 *   3.  gcc -g -O2 -Wall -mno-cygwin -o win-palink win-palink.o  
 *            ../src/libnet.a -lwpcap -lpacket -lws2_32
 */

#include <stdlib.h>
#include <stdio.h>
#include <libnet.h>

#define MAXTEXT 1400

void usage(char *); 

int main(int argc, char *argv[])
{
    int c, i; 
    u_char *cp;
    libnet_t *l;
    libnet_ptag_t eth, ip;
    libnet_ptag_t tcp, udp, t_op;
    struct libnet_stats ls;
    u_long src_ip, dst_ip;
    u_long seq, ack; 
    u_short src_prt, dst_prt, win, mss;
    u_short len; 
    u_char flags = 0x00;      /* tcp flag */ 
    u_char opt[] = "\x02\x04\x00\x00\x01\x01\x04\x02";
    u_char ttl='\xf0';  /* TTL of IP */
    char errbuf[LIBNET_ERRBUF_SIZE];
    char *dev = NULL;   /* default if */ 
    char *smac = "00:E0:81:30:D0:36"; 
    char *dmac = "00:23:EB:13:93:C2"; 
    struct in_addr addr;        

    FILE *dmf; 
    char *dms = NULL; 
    int pay_s = 0;
    char *dmfn = NULL; 
    char *paybuf; 
    u_char bt;
    char linebuf[MAXTEXT];
    int vbflag = 0;  /* verbose flag */ 
    int synflag = 0;  /* SYN flag.  */ 
    int udpflag = 0;  /* UDP flag.  */ 

    libnet_seed_prand(l);
    tcp = 0; 
    udp = 0; 
    ip = 0;
    t_op = 0; 

    dst_ip = libnet_get_prand(LIBNET_PRu32);
    src_ip = libnet_get_prand(LIBNET_PRu32);
    src_prt = libnet_get_prand(LIBNET_PRu16);
    dst_prt = 80;			         /* http */		 
    win = libnet_get_prand(LIBNET_PRu16); 
    seq = libnet_get_prand(LIBNET_PRu32);
    ack = libnet_get_prand(LIBNET_PRu32);

    while ((c = getopt(argc, argv, "i:d:o:s:p:T:E:t:M:L:w:O:APRSFvuh")) != EOF)
    {
        switch (c)
        {
         case 'i':
           dev = optarg; 
           break;   
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
           mss = htons( (u_short)atoi(optarg) ); 
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
                exit(0);  
         default:
                usage(argv[0]);
                exit(EXIT_FAILURE);
        }
    }

    fprintf(stderr, "here 1 \n"); 

    if ( dev == NULL ) {  /* tell user to choose an interface */ 
      fprintf(stderr, "Need to specify -i interface. \n"); 
      exit(EXIT_FAILURE);
    } 

    fprintf(stderr, "here 2 \n"); 

    l = libnet_init(
            LIBNET_LINK,                            /* injection type */
            dev,                                 /* network interface */
            errbuf);                                /* errbuf */

    if (l == NULL)
    {
        fprintf(stderr, "libnet_init() failed: %s", errbuf);
        exit(EXIT_FAILURE);
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
  
  if(udpflag) {  /* Building UDP packet */
    len =  LIBNET_UDP_H + pay_s;            /* packet length */

        udp = libnet_build_udp(
            src_prt,                                /* source port */
            dst_prt,                            /* destination port */
            len,   /* packet length */
            0, 					/* checksum */
            dms,                                      /* payload */ 
            pay_s,                                /* payload size */
            l,                                      /* libnet handle */
            udp);                                   /* libnet id */

        if (udp == -1) {
            fprintf(stderr, "Can't build UDP header: %s\n", libnet_geterror(l));
            goto bad;
        }

/* Building IP */

            ip = libnet_build_ipv4(
                LIBNET_IPV4_H + len,            /* length */
                0x00,                                          /* TOS */
                0,                                            /* IP ID */
                0x4000,                                          /* IP Frag */
                ttl,                                         /* TTL */
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

  } else { /* by default, building tcp */

  
    len =  LIBNET_TCP_H + pay_s;            /* packet length */
    /* Building TCP options for SYN */
        if(synflag && mss) { 
            ack =0; 
            len =  LIBNET_TCP_H + 8 + pay_s;    /* 8 more for syn */
            strncpy(opt+2, &mss, 2);  /* stuff mss in opt */ 
            t_op = libnet_build_tcp_options(
              opt, 
              8,
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
                LIBNET_IPV4_H + len, /* length */
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

  } /* end if (udpflag) */

/* Build ethernet frame */ 
    eth = libnet_build_ethernet(
        dmac,                                      /* ethernet destination */
        smac,                                      /* ethernet source */
        ETHERTYPE_IP,                               /* protocol type */
        NULL,                                       /* payload */
        0,                                          /* payload size */
        l,                                          /* libnet handle */
        0);                                         /* libnet id */
    if (eth == -1)
    {
       fprintf(stderr, "Can't build ethernet header: %s\n", libnet_geterror(l));
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
    fprintf(stderr, "usage %s -u -i dev -s sIP -o sport -d dIP -p dport -t ttl -w win -T \"payload text\" -E data_file -M seq -L ack -A|P|R|S|F -O mss -v \n",
                name);
    fprintf(stderr, "If both -E and -T specified, -E overrides -T\n"); 
    fprintf(stderr, "-O mss specifies MSS in TCP option. If not given, no TCP option will be present in SYN\n"); 
    fprintf(stderr, "-u: create UDP packet instead of default TCP packet.\n"); 

}
