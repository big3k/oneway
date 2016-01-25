/*
 *  $Id: rip.c,v 1.1.1.1 2002/08/05 22:26:04 route Exp $
 *
 *  libnet 1.1
 *  Build a RIP packet
 *
 *  Copyright (c) 1998 - 2002 Mike D. Schiffman <mike@infonexus.com>
 *  All rights reserved.
 *
 *  Copyright (c) 2001 Tom Macdougall <macdougt71@hotmail.com>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

#if (HAVE_CONFIG_H)
#include "../include/config.h"
#endif
#include "./libnet_test.h"

#define MAX_PAYLOAD_SIZE    1024
#define MAX_NUM_ROUTES      25


/******************************************************************

Main function (mostly copied from earlier examples)

Question: why are the packets built from left to right
          instead of right to left?

*******************************************************************/


int
main(int argc, char *argv[])
{

    int c, i, j, l, rip_ips;
    libnet_t *l;
    char *device;
    libnet_ptag_t t;
    struct ether_addr *e;               /* structure to hold returned MAC */
    u_char local_enet_src[6];
    u_long src_ip, dst_ip;              /* source ip, dest ip */
    u_long reverse;
    u_char *rip_payload;
    u_char *rip_ip_route;
    u_long rip_payload_s
    char errbuf[LIBNET_ERRBUF_SIZE];
    struct in_addr rip_routes[MAX_NUM_ROUTES];


    printf("libnet 1.1 RIP packet shaping[link]\n");

    device = NULL;
    src_ip  = 0;
    dst_ip  = 0;
    while ((c = getopt(argc, argv, "d:i:s:")) != EOF)
    {
        switch (c)
        {
            case 'd':
                if (!(dst_ip = libnet_name2addr(optarg, LIBNET_RESOLVE)))
                {
                    fprintf(stderr, "Bad destination IP address: %s\n", optarg);
                    exit(EXIT_FAILURE); 
                }
                break;
            case 's':
                if (!(src_ip = libnet_name2addr(optarg, LIBNET_RESOLVE)))
                {
                    fprintf(stderr, "Bad source IP address: %s\n", optarg);
                    exit(EXIT_FAILURE); 
                }
                break;
            case 'i':
                device = optarg;
                break;
            default:
        }
    }

    if (!src_ip || !dst_ip)
    {
        usage(argv[0]);
        exit(EXIT_FAILURE);
    }

    /* Number of rip routes to advertise */
    rip_ips = argc - optind;
  
    if (rip_ips < 1)
    {
        usage(argv[0]);
        exit(EXIT_FAILURE);
    }
  
    /* RIP memory allocation */
    rip_payload_s = 4 + 20 * rip_ips * sizeof(u_char);
    rip_payload = malloc(rip_payload_s);

    l = libnet_init(
            LIBNET_LINK,                            /* injection type */
            device,                                 /* network interface */
            errbuf);                                /* errbuf */
    if (l == NULL)
    {
        fprintf(stderr, "libnet_init() failed: %s", errbuf);
        exit(EXIT_FAILURE);
    }

    /* Fetch the MAC address of the current device */
    e = libnet_get_hwaddr(l);
    for (i = 0; i < 6; i++)
    {
        local_enet_src[i] = e->ether_addr_octet[i];
    }

    /*
     *  Build the packet, remmebering that order IS important.  We must
     *  build the packet from lowest protocol type on up as it would
     *  appear on the wire.  So for our RIP packet:
     *
     *  ------------------------------------------------------/ /-----
     *  |   Ethernet   |      IP           |  UDP   |     RIP        |
     *  ------------------------------------------------------/ /-----
     *         ^                     ^          ^       ^
     *         |-----------------|   |          |       |
     *  libnet_build_ethernet()--|   |          |       |
     *                               |          |       |
     *  libnet_build_ipv4()----------|          |       |
     *                                          |       |
     *  libnet_build_udp()----------------------|       |
     *                                                  |
     *  libnet_build_rip()------------------------------|
     */
    t = libnet_build_ethernet(
        enet_dst,                                   /* ethernet destination */
        local_enet_src,                             /* ethernet source */
        ETHERTYPE_IP,                               /* protocol type */
        NULL,                                       /* payload */
        0,                                          /* payload size */
        l,                                          /* libnet handle */
        0);
    if (t == -1)
    {
        fprintf(stderr, "Can't build ethernet header: %s\n", libnet_geterror(l));
        goto bad;
    }

    t = libnet_build_ipv4(
        LIBNET_IPV4_H + LIBNET_UDP_H + rip_payload_s,/* length */
        0,                                          /* TOS */
        242,                                        /* IP ID */
        0,                                          /* IP Frag */
        64,                                         /* TTL */
        IPPROTO_UDP,                                /* protocol */
        0,                                          /* checksum */
        src_ip,                                     /* source IP */
        dst_ip,                                     /* destination IP */
        NULL,                                       /* payload */
        0,                                          /* payload size */
        l,                                          /* libnet handle */
        0);                                         /* libnet id */
    if (t == -1)
    {
        fprintf(stderr, "Can't build IP header: %s\n", libnet_geterror(l));
        goto bad;
    }

    t = libnet_build_udp(
        520,                                        /* source port */
        520,                                        /* NTP port */
        LIBNET_UDP_H + LIBNET_RIP_H + rip_payload_s,/* UDP packet length */
        0,                                          /* checksum */
        NULL,                                       /* payload */
        0,                                          /* payload size */
        l,                                          /* libnet handle */
        0);                                         /* libnet id */
    if (t == -1)
    {
        fprintf(stderr, "Can't build UDP header: %s\n", libnet_geterror(l));
        goto bad;
    }

    t = libnet_build_rip(
        RIPCMD_RESPONSE,                        /* command */
        RIPVER_1,                               /* version */
        0,                                      /* zero */
        0x0002,                                 /* address family */
        0,                                      /* zero */
        reverse,                                /* IP address */
        0,                                      /* zero */
        0,                                      /* zero */
        0x00000001,                             /* metric */
        NULL,                                   /* payload */
        0,                                      /* payload size */
        l,                                      /* libnet handle */
        0);                                     /* libnet id */
    if (t == -1)
    {
        fprintf(stderr, "Can't build RIP header: %s\n", libnet_geterror(l));
        goto bad;
    }

    for (j = optind; j < argc; j++)
    {
        l = j - 5;
        inet_aton(argv[j], &rip_routes[l]);
        reverse = ntohl(rip_routes[l].s_addr);
        rip_ip_route = malloc(24 * sizeof(u_char));

    
        /* First time you need the command and version */
        if (0 == l)
        {
            memcpy(rip_packet,rip_ip_route,24);      
            free(rip_ip_route);     
        }
        else
        {
            /* Skip the command and version */
            rip_ip_route+=4;
            memcpy(rip_packet+4+20*l,rip_ip_route,20);      
            free(rip_ip_route);           
        }    
    }
  

  
  
#ifdef DEBUG
  temp2 = packet + LIBNET_ETH_H + LIBNET_IP_H+LIBNET_UDP_H ;
  show_hex((u_char *)temp2,"rip_packet",40);
#endif
  
  
}


void
usage(char *name)
{
    fprintf(stderr, "usage: %s -s s_ip -d d_ip ip_route1 ... ip_routeN where 0 < N < 26\n", name);
    fprintf(stderr, "example: %s -s 10.12.2.15 -d 10.12.2.1 12.1.1.0 12.3.4.0 \n", name);
}

/* EOF */
