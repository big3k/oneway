/*
 *  $Id: libnet_write.c,v 1.1.1.1 2002/08/05 22:26:03 route Exp $
 *
 *  libnet
 *  libnet_write_ip.c - writes a prebuilt IP packet to the network
 *
 *  Copyright (c) 1998 - 2002 Mike D. Schiffman <mike@infonexus.com>
 *  All rights reserved.
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
#include "../include/libnet.h"

int
libnet_write(libnet_t *l)
{
    int c;
    u_long len;
    u_char *packet;

    if (l == NULL)
    { 
        return (-1);
    }

    packet = NULL;
    c = libnet_pblock_coalesce(l, &packet, &len);
    if (c == - 1)
    {
        /* err msg set in libnet_pblock_coalesce() */
        return (-1);
    }

    /* assume error */
    c = -1;
    switch (l->injection_type)
    {
        case LIBNET_RAW4:
        case LIBNET_RAW4_ADV:
            if (len > LIBNET_MAX_PACKET)
            {
                snprintf(l->err_buf, LIBNET_ERRBUF_SIZE,
                        "libnet_write(): packet is too large %ld\n", len);
                goto done;
            }
            c = libnet_write_raw_ipv4(l, packet, len);
            break;
        case LIBNET_RAW6:
        case LIBNET_RAW6_ADV:
            c = libnet_write_raw_ipv6(l, packet, len);
            break;
        case LIBNET_LINK:
        case LIBNET_LINK_ADV:
            c = libnet_write_link(l, packet, len);
            break;
        default:
            sprintf(l->err_buf, "libnet_write: unsuported injection type\n");
            goto done;
    }

    /* do statistics */
    if (c == len)
    {
        l->stats.packets_sent++;
        l->stats.bytes_written += c;
    }
    else
    {
        l->stats.packet_errors++;
        /*
         *  XXX - we probably should have a way to retrieve the number of
         *  bytes actually written (since we might have written something).
         */
        if (c > 0)
        {
            l->stats.bytes_written += c;
        }
    }
done:
    /*
     *  Restore original pointer address so free won't complain about a
     *  modified chunk pointer.
     */
    if (l->aligner > 0)
    {
        packet = packet - l->aligner;
    }
    free(packet);
    return (c);
}


int
libnet_write_raw_ipv4(libnet_t *l, u_char *packet, u_long size)
{
    int c;
    struct sockaddr_in sin;
    struct libnet_ipv4_hdr *ip_hdr;

    if (l == NULL)
    { 
        return (-1);
    } 

    ip_hdr = (struct libnet_ipv4_hdr *)packet;

#if (LIBNET_BSD_BYTE_SWAP)
    /*
     *  For link access, we don't need to worry about the inconsistencies of
     *  certain BSD kernels.  However, raw socket nuances abound.  Certain
     *  BSD implmentations require the ip_len and ip_off fields to be in host
     *  byte order.
     */
    ip_hdr->ip_len = FIX(ip_hdr->ip_len);
    ip_hdr->ip_off = FIX(ip_hdr->ip_off);
#endif

    memset(&sin, 0, sizeof(struct sockaddr_in));
    sin.sin_family  = AF_INET;
    sin.sin_addr.s_addr = ip_hdr->ip_dst.s_addr;
#if (__WIN32__)
    /* set port for TCP */
    /*
     *  XXX - should first check to see if there's a pblock for a TCP
     *  header, if not we can use a dummy value for the port.
     */
    if (ip_hdr->ip_p == 6)
    {
        struct libnet_tcp_hdr *tcph_p =
                (struct libnet_tcp_hdr *)(packet + (ip_hdr->ip_hl << 2));
        sin.sin_port = tcph_p->th_dport;
    }
    /* set port for UDP */
    /*
     *  XXX - should first check to see if there's a pblock for a UDP
     *  header, if not we can use a dummy value for the port.
     */
    else if (ip_hdr->ip_p == 17)
    {
        struct libnet_udp_hdr *udph_p =
                (struct libnet_udp_hdr *)(packet + (ip_hdr->ip_hl << 2));
       sin.sin_port = udph_p->uh_dport;
    }
#endif

    c = sendto(l->fd, packet, size, 0, (struct sockaddr *)&sin,
        sizeof(struct sockaddr));

#if (LIBNET_BSD_BYTE_SWAP)
    ip_hdr->ip_len = UNFIX(ip_hdr->ip_len);
    ip_hdr->ip_off = UNFIX(ip_hdr->ip_off);
#endif
             
    if (c != size)
    {
#if !(__WIN32__)
        snprintf(l->err_buf, LIBNET_ERRBUF_SIZE,
                "libnet_write_ipv4: %d bytes written (%s)\n", c,
                strerror(errno));
#else
        snprintf(l->err_buf, LIBNET_ERRBUF_SIZE,
                "libnet_write_ipv4: %d bytes written (%d)\n", c,
                WSAGetLastError());
#endif
    }
    return (c);
}


int
libnet_write_raw_ipv6(libnet_t *l, u_char *packet, u_long size)
{
    int c;
    struct sockaddr_in6 sin;
    struct libnet_ipv6_hdr *ip_hdr;

    if (l == NULL)
    { 
        return (-1);
    } 

    ip_hdr = (struct libnet_ipv6_hdr *)packet;

    memset(&sin, 0, sizeof(struct sockaddr_in));
    sin.sin6_family  = AF_INET6;
    memcpy(sin.sin6_addr.s6_addr, ip_hdr->ip_dst.libnet_s6_addr,
            sizeof(ip_hdr->ip_dst.libnet_s6_addr));
       
    c = sendto(l->fd, packet, size, 0, (struct sockaddr *)&sin, sizeof(sin));
    if (c != size)
    {
#if !(__WIN32__)
        snprintf(l->err_buf, LIBNET_ERRBUF_SIZE,
                "libnet_write_ipv6: %d bytes written (%s)\n", c,
                strerror(errno));
#else
        snprintf(l->err_buf, LIBNET_ERRBUF_SIZE,
                "libnet_write_ipv6: %d bytes written (%d)\n", c,
                WSAGetLastError());
#endif
    }
    return (c);
}

/* EOF */
