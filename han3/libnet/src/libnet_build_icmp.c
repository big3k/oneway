/*
 *  $Id: libnet_build_icmp.c,v 1.1.1.1 2002/08/05 22:26:03 route Exp $
 *
 *  libnet
 *  libnet_build_icmp.c - ICMP packet assemblers
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
libnet_build_icmpv4_echo(u_char type, u_char code, u_short sum,
            u_short id, u_short seq, u_char *payload, u_long payload_s, 
            libnet_t *l, libnet_ptag_t ptag)
{
    u_long n;
    u_short h;
    libnet_pblock_t *p;
    struct libnet_icmpv4_hdr icmp_hdr;

    if (l == NULL)
    { 
        return (-1);
    } 

    n = LIBNET_ICMPV4_ECHO_H + payload_s;        /* size of memory block */
    h = LIBNET_ICMPV4_ECHO_H + payload_s;        /* hl for checksum */

    /*
     *  Find the existing protocol block if a ptag is specified, or create
     *  a new one.
     */
    p = libnet_pblock_probe(l, ptag, n, LIBNET_PBLOCK_ICMPV4_ECHO_H);
    if (p == NULL)
    {
        return (-1);
    }

    icmp_hdr.icmp_type = type;          /* packet type */
    icmp_hdr.icmp_code = code;          /* packet code */
    icmp_hdr.icmp_sum  = sum;           /* checksum */
    /*
     *  XXX - Does this need to be Big Endian or what?  I was testing on
     *  04/23/2002 and stuff was breaking with these htons()'d.
     */
    icmp_hdr.icmp_id   = id;            /* packet id */
    icmp_hdr.icmp_seq  = seq;           /* packet seq */

    n = libnet_pblock_append(l, p, (u_char *)&icmp_hdr, LIBNET_ICMPV4_ECHO_H);
    if (n == -1)
    {
        goto bad;
    }
 
    if (payload && payload_s)
    {
        n = libnet_pblock_append(l, p, payload, payload_s);
        if (n == -1)
        {
            goto bad;
        }
    }
 
    if (sum == 0)
    {
        /*
         *  If checksum is zero, by default libnet will compute a checksum
         *  for the user.  The programmer can override this by calling
         *  libnet_toggle_checksum(l, ptag, 1);
         */
        libnet_pblock_setflags(p, LIBNET_PBLOCK_DO_CHECKSUM);
    }
    return (ptag ? ptag : libnet_pblock_update(l, p, h, LIBNET_PBLOCK_ICMPV4_ECHO_H));
bad:
    libnet_pblock_free(p);
    return (-1);
}


libnet_ptag_t
libnet_build_icmpv4_mask(u_char type, u_char code, u_short sum,
            u_short id, u_short seq, u_long mask, u_char *payload,
            u_long payload_s, libnet_t *l, libnet_ptag_t ptag)
{
    u_long n;
    u_short h;
    libnet_pblock_t *p;
    struct libnet_icmpv4_hdr icmp_hdr;

    if (l == NULL)
    { 
        return (-1);
    } 

    n = LIBNET_ICMPV4_MASK_H + payload_s;        /* size of memory block */
    h = LIBNET_ICMPV4_MASK_H + payload_s;        /* hl for checksum */

    /*
     *  Find the existing protocol block if a ptag is specified, or create
     *  a new one.
     */
    p = libnet_pblock_probe(l, ptag, n, LIBNET_PBLOCK_ICMPV4_MASK_H);
    if (p == NULL)
    {
        return (-1);
    }

    icmp_hdr.icmp_type = type;          /* packet type */
    icmp_hdr.icmp_code = code;          /* packet code */
    icmp_hdr.icmp_sum  = sum;           /* checksum */
    icmp_hdr.icmp_id   = htons(id);     /* packet id */
    icmp_hdr.icmp_seq  = htons(seq);    /* packet seq */
    icmp_hdr.icmp_mask = htonl(mask);   /* address mask */

    n = libnet_pblock_append(l, p, (u_char *)&icmp_hdr, LIBNET_ICMPV4_MASK_H);
    if (n == -1)
    {
        goto bad;
    }
 
    if (payload && payload_s)
    {
        n = libnet_pblock_append(l, p, payload, payload_s);
        if (n == -1)
        {
            goto bad;
        }
    }
 
    if (sum == 0)
    {
        /*
         *  If checksum is zero, by default libnet will compute a checksum
         *  for the user.  The programmer can override this by calling
         *  libnet_toggle_checksum(l, ptag, 1);
         */
        libnet_pblock_setflags(p, LIBNET_PBLOCK_DO_CHECKSUM);
    }
    return (ptag ? ptag : libnet_pblock_update(l, p, h, LIBNET_PBLOCK_ICMPV4_MASK_H));
bad:
    libnet_pblock_free(p);
    return (-1);
}


libnet_ptag_t
libnet_build_icmpv4_timestamp(u_char type, u_char code, u_short sum,
            u_short id, u_short seq, n_time otime, n_time rtime, n_time ttime,
            u_char *payload, u_long payload_s, libnet_t *l, libnet_ptag_t ptag)
{
    u_long n;
    u_short h;
    libnet_pblock_t *p;
    struct libnet_icmpv4_hdr icmp_hdr;

    if (l == NULL)
    { 
        return (-1);
    } 

    n = LIBNET_ICMPV4_TS_H + payload_s;        /* size of memory block */
    h = LIBNET_ICMPV4_TS_H + payload_s;        /* hl for checksum */

    /*
     *  Find the existing protocol block if a ptag is specified, or create
     *  a new one.
     */
    p = libnet_pblock_probe(l, ptag, n, LIBNET_PBLOCK_ICMPV4_TS_H);
    if (p == NULL)
    {
        return (-1);
    }

    icmp_hdr.icmp_type  = type;             /* packet type */
    icmp_hdr.icmp_code  = code;             /* packet code */
    icmp_hdr.icmp_sum   = sum;              /* checksum */
    icmp_hdr.icmp_id    = htons(id);        /* packet id */
    icmp_hdr.icmp_seq   = htons(seq);       /* packet seq */
    icmp_hdr.icmp_otime = htonl(otime);     /* original timestamp */
    icmp_hdr.icmp_rtime = htonl(rtime);     /* receive timestamp */
    icmp_hdr.icmp_ttime = htonl(ttime);     /* transmit timestamp */

    n = libnet_pblock_append(l, p, (u_char *)&icmp_hdr, LIBNET_ICMPV4_TS_H);
    if (n == -1)
    {
        goto bad;
    }
 
    if (payload && payload_s)
    {
        n = libnet_pblock_append(l, p, payload, payload_s);
        if (n == -1)
        {
            goto bad;
        }
    }
 
    if (sum == 0)
    {
        /*
         *  If checksum is zero, by default libnet will compute a checksum
         *  for the user.  The programmer can override this by calling
         *  libnet_toggle_checksum(l, ptag, 1);
         */
        libnet_pblock_setflags(p, LIBNET_PBLOCK_DO_CHECKSUM);
    }
    return (ptag ? ptag : libnet_pblock_update(l, p, h, LIBNET_PBLOCK_ICMPV4_TS_H));
bad:
    libnet_pblock_free(p);
    return (-1);
}


libnet_ptag_t
libnet_build_icmpv4_unreach(u_char type, u_char code, u_short sum,
            u_short orig_len, u_char orig_tos, u_short orig_id,
            u_short orig_frag, u_char orig_ttl, u_char orig_prot,
            u_short orig_check, u_long orig_src, u_long orig_dst,
            u_char *payload, u_long payload_s, libnet_t *l, libnet_ptag_t ptag)
{
    u_long n;
    u_short h;
    libnet_ptag_t ipv4, ret;
    libnet_pblock_t *p, *q;
    struct libnet_icmpv4_hdr icmp_hdr;

    if (l == NULL)
    { 
        return (-1);
    } 

    n = LIBNET_ICMPV4_UNREACH_H;        /* size of memory block */
    /* hl for checksum */
    h = LIBNET_ICMPV4_UNREACH_H + LIBNET_IPV4_H;

    /*
     *  Find the existing protocol block if a ptag is specified, or create
     *  a new one.
     */
    p = libnet_pblock_probe(l, ptag, n, LIBNET_PBLOCK_ICMPV4_UNREACH_H);
    if (p == NULL)
    {
        return (-1);
    }

    icmp_hdr.icmp_type = type;          /* packet type */
    icmp_hdr.icmp_code = code;          /* packet code */
    icmp_hdr.icmp_sum  = sum;           /* checksum */
    icmp_hdr.icmp_id   = 0;             /* must be 0 */
    icmp_hdr.icmp_seq  = 0;             /* must be 0 */

    n = libnet_pblock_append(l, p, (u_char *)&icmp_hdr,
            LIBNET_ICMPV4_UNREACH_H);
    if (n == -1)
    {
        goto bad;
    }

    /*
     *  Build the original IPv4 header in its own pblock.  We stick the
     *  payload here.
     */
    ipv4 = libnet_build_ipv4(orig_len, orig_tos, orig_id, orig_frag, orig_ttl,
        orig_prot, orig_check, orig_src, orig_dst, payload, payload_s, l, 0);
    if (ipv4 == -1)
    {
        /* error set elsewhere */
        goto bad;
    }
    if (orig_check == 0)
    {
        /* XXX - this isn't working. */
        q = libnet_pblock_probe(l, ipv4, n, LIBNET_PBLOCK_IPV4_H);
        libnet_pblock_setflags(q, LIBNET_PBLOCK_DO_CHECKSUM);
    }

    if (sum == 0)
    {
        /*
         *  If checksum is zero, by default libnet will compute a checksum
         *  for the user.  The programmer can override this by calling
         *  libnet_toggle_checksum(l, ptag, 1);
         */
        libnet_pblock_setflags(p, LIBNET_PBLOCK_DO_CHECKSUM);
    }
    ret = (ptag ? ptag : libnet_pblock_update(l, p, h,
            LIBNET_PBLOCK_ICMPV4_UNREACH_H));

    /* reorder ICMP and IPv4 headers to make the packet assemble properly */
    libnet_pblock_swap(l, ret, ipv4);
    return (ret);
bad:
    libnet_pblock_free(p);
    return (-1);
}


libnet_ptag_t
libnet_build_icmpv4_timeexceed(u_char type, u_char code, u_short sum,
            u_short orig_len, u_char orig_tos, u_short orig_id,
            u_short orig_frag, u_char orig_ttl, u_char orig_prot,
            u_short orig_check, u_long orig_src, u_long orig_dst,
            u_char *payload, u_long payload_s, libnet_t *l, libnet_ptag_t ptag)
{
    u_long n;
    u_short h;
    libnet_ptag_t ipv4, ret;
    libnet_pblock_t *p;
    struct libnet_icmpv4_hdr icmp_hdr;

    if (l == NULL)
    { 
        return (-1);
    } 

    /* size of memory block */
    n = LIBNET_ICMPV4_TIMXCEED_H;
    /* hl for checksum */
    h = LIBNET_ICMPV4_TIMXCEED_H + LIBNET_IPV4_H + payload_s;

    /*
     *  Find the existing protocol block if a ptag is specified, or create
     *  a new one.
     */
    p = libnet_pblock_probe(l, ptag, n, LIBNET_PBLOCK_ICMPV4_TIMXCEED_H);
    if (p == NULL)
    {
        return (-1);
    }

    icmp_hdr.icmp_type = type;          /* packet type */
    icmp_hdr.icmp_code = code;          /* packet code */
    icmp_hdr.icmp_sum  = sum;           /* checksum */
    icmp_hdr.icmp_id   = 0;             /* must be 0 */
    icmp_hdr.icmp_seq  = 0;             /* must be 0 */

    n = libnet_pblock_append(l, p, (u_char *)&icmp_hdr,
            LIBNET_ICMPV4_TIMXCEED_H);
    if (n == -1)
    {
        goto bad;
    }
 
    /*
     *  Build the original IPv4 header in its own pblock.  We stick the
     *  payload here.
     */
    ipv4 = libnet_build_ipv4(orig_len, orig_tos, orig_id, orig_frag, orig_ttl,
        orig_prot, orig_check, orig_src, orig_dst, payload, payload_s, l, 0);
    if (ipv4 == -1)
    {
        /* error set below */
        goto bad;
    }

    if (sum == 0)
    {
        /*
         *  If checksum is zero, by default libnet will compute a checksum
         *  for the user.  The programmer can override this by calling
         *  libnet_toggle_checksum(l, ptag, 1);
         */
        libnet_pblock_setflags(p, LIBNET_PBLOCK_DO_CHECKSUM);
    }
    ret = (ptag ? ptag : libnet_pblock_update(l, p, h,
            LIBNET_PBLOCK_ICMPV4_TIMXCEED_H));

    /* reorder ICMP and IPv4 headers to make the packet assemble properly */
    libnet_pblock_swap(l, ret, ipv4);
    return (ret);
bad:
    libnet_pblock_free(p);
    return (-1);
}


libnet_ptag_t
libnet_build_icmpv4_redirect(u_char type, u_char code, u_short sum,
            u_long gateway, u_short orig_len, u_char orig_tos, u_short orig_id,
            u_short orig_frag, u_char orig_ttl, u_char orig_prot,
            u_short orig_check, u_long orig_src, u_long orig_dst,
            u_char *payload, u_long payload_s, libnet_t *l, libnet_ptag_t ptag)

{
    u_long n;
    u_short h;
    libnet_ptag_t ipv4, ret;
    libnet_pblock_t *p;
    struct libnet_icmpv4_hdr icmp_hdr;

    if (l == NULL)
    { 
        return (-1);
    } 

    n = LIBNET_ICMPV4_REDIRECT_H;               /* size of memory block */
    /* hl for checksum */
    h = LIBNET_ICMPV4_REDIRECT_H + LIBNET_IPV4_H + payload_s;

    /*
     *  Find the existing protocol block if a ptag is specified, or create
     *  a new one.
     */
    p = libnet_pblock_probe(l, ptag, n, LIBNET_PBLOCK_ICMPV4_REDIRECT_H);
    if (p == NULL)
    {
        return (-1);
    }

    icmp_hdr.icmp_type      = type;             /* packet type */
    icmp_hdr.icmp_code      = code;             /* packet code */
    icmp_hdr.icmp_sum       = sum;              /* checksum */
    icmp_hdr.hun.gateway    = htonl(gateway);   /* gateway address */

    n = libnet_pblock_append(l, p, (u_char *)&icmp_hdr, LIBNET_ICMPV4_REDIRECT_H);
    if (n == -1)
    {
        goto bad;
    }
 
    /*
     *  Build the original IPv4 header in its own pblock.  We stick the
     *  payload here.
     */
    ipv4 = libnet_build_ipv4(orig_len, orig_tos, orig_id, orig_frag, orig_ttl,
        orig_prot, orig_check, orig_src, orig_dst, payload, payload_s, l, 0);
    if (ipv4 == -1)
    {
        /* error set below */
        goto bad;
    }

    if (sum == 0)
    {
        /*
         *  If checksum is zero, by default libnet will compute a checksum
         *  for the user.  The programmer can override this by calling
         *  libnet_toggle_checksum(l, ptag, 1);
         */
        libnet_pblock_setflags(p, LIBNET_PBLOCK_DO_CHECKSUM);
    }
    ret = (ptag ? ptag : libnet_pblock_update(l, p, h,
            LIBNET_PBLOCK_ICMPV4_REDIRECT_H));

    /* reorder ICMP and IPv4 headers to make the packet assemble properly */
    libnet_pblock_swap(l, ret, ipv4);
    return (ret);
bad:
    libnet_pblock_free(p);
    return (-1);
}


/* EOF */
