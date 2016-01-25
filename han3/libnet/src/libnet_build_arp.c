/*
 *  $Id: libnet_build_arp.c,v 1.1.1.1 2002/08/05 22:26:03 route Exp $
 *
 *  libnet
 *  libnet_build_arp.c - ARP packet assembler
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


libnet_ptag_t
libnet_build_arp(u_short hrd, u_short pro, u_char hln, u_char pln, u_short op,
            u_char *sha, u_char *spa, u_char *tha, u_char *tpa,
            u_char *payload, u_long payload_s, libnet_t *l, libnet_ptag_t ptag)
{
    u_long n;
    u_short h;
    libnet_pblock_t *p;
    struct libnet_arp_hdr arp_hdr;

    if (l == NULL)
    { 
        return (-1); 
    }

    n = LIBNET_ARP_H + (2 * hln) + (2 * pln) + payload_s;
    h = 0;  /* ARP headers have no checksum */

    /*
     *  Find the existing protocol block if a ptag is specified, or create
     *  a new one.
     */
    p = libnet_pblock_probe(l, ptag, n, LIBNET_PBLOCK_ARP_H);
    if (p == NULL)
    {
        return (-1);
    }

    arp_hdr.ar_hrd = htons(hrd);       /* hardware address type */
    arp_hdr.ar_pro = htons(pro);       /* protocol address type */
    arp_hdr.ar_hln = hln;              /* hardware address length */
    arp_hdr.ar_pln = pln;              /* protocol address length */
    arp_hdr.ar_op  = htons(op);        /* opcode command */

    n = libnet_pblock_append(l, p, (u_char *)&arp_hdr, LIBNET_ARP_H);
    if (n == -1)
    {
        /* err msg set in libnet_pblock_append() */
        goto bad; 
    }
    n = libnet_pblock_append(l, p, sha, hln);
    if (n == -1)
    {
        /* err msg set in libnet_pblock_append() */
        goto bad;
    }
    n = libnet_pblock_append(l, p, spa, pln);
    if (n == -1)
    {
        /* err msg set in libnet_pblock_append() */
        goto bad;
    }
    n = libnet_pblock_append(l, p, tha, hln);
    if (n == -1)
    {
        /* err msg set in libnet_pblock_append() */
        goto bad;
    } 
    n = libnet_pblock_append(l, p, tpa, pln);
    if (n == -1)
    {
        /* err msg set in libnet_pblock_append() */
        goto bad;
    }

    if (payload && payload_s)
    {
        n = libnet_pblock_append(l, p, payload, payload_s);
        if (n == -1)
        {
            /* err msg set in libnet_pblock_append() */
            goto bad;
        }
    }

    return (ptag ? ptag : libnet_pblock_update(l, p, h, LIBNET_PBLOCK_ARP_H));
bad:
    libnet_pblock_free(p);
    return (-1);
}


/* EOF */
