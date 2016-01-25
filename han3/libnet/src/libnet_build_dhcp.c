/*
 *  $Id: libnet_build_dhcp.c,v 1.1.1.1 2002/08/05 22:26:03 route Exp $
 *
 *  libnet
 *  libnet_build_dhcp.c - DHCP/BOOTP packet assembler
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
libnet_build_dhcpv4(u_char opcode, u_char htype, u_char hlen, u_char hopcount,
            u_long xid, u_short secs, u_short flags, u_long cip, u_long yip,
            u_long sip, u_long gip, u_char *chaddr, u_char *sname, u_char *file,
            u_char *payload, u_long payload_s, libnet_t *l, libnet_ptag_t ptag)
{
    u_long n;
    u_short h;
    libnet_pblock_t *p; 
    struct libnet_dhcpv4_hdr dhcp_hdr;

    if (l == NULL)
    { 
        return (-1);
    } 

    n = LIBNET_DHCPV4_H + payload_s;
    h = 0;          /* no checksum */
 
    /*
     *  Find the existing protocol block if a ptag is specified, or create
     *  a new one.
     */
    p = libnet_pblock_probe(l, ptag, n, LIBNET_PBLOCK_DHCPV4_H);
    if (p == NULL)
    {
        return (-1);
    }

    dhcp_hdr.dhcp_opcode    = opcode;
    dhcp_hdr.dhcp_htype     = htype;
    dhcp_hdr.dhcp_hlen      = hlen;
    dhcp_hdr.dhcp_hopcount  = hopcount;
    dhcp_hdr.dhcp_xid       = htonl(xid);
    dhcp_hdr.dhcp_secs      = htons(secs);
    dhcp_hdr.dhcp_flags     = htons(flags);
    dhcp_hdr.dhcp_cip       = htonl(cip);
    dhcp_hdr.dhcp_yip       = htonl(yip);
    dhcp_hdr.dhcp_sip       = htonl(sip);
    dhcp_hdr.dhcp_gip       = htonl(gip);

    if (chaddr)
    { 
        memcpy(dhcp_hdr.dhcp_chaddr, chaddr, sizeof (dhcp_hdr.dhcp_chaddr));
        dhcp_hdr.dhcp_chaddr[sizeof(dhcp_hdr.dhcp_chaddr)] = 0;
    }
    else
    {
        memset(dhcp_hdr.dhcp_chaddr, 0, sizeof (dhcp_hdr.dhcp_chaddr));
    }
 
    if (sname)
    { 
        memcpy(dhcp_hdr.dhcp_sname, sname, sizeof (dhcp_hdr.dhcp_sname));
        dhcp_hdr.dhcp_sname[sizeof(dhcp_hdr.dhcp_sname)] = 0;
    }
    else
    {
        memset(dhcp_hdr.dhcp_sname, 0, sizeof (dhcp_hdr.dhcp_sname));
    }

    if (file)
    {
        memcpy(dhcp_hdr.dhcp_file, file, sizeof (dhcp_hdr.dhcp_file));
        dhcp_hdr.dhcp_file[sizeof(dhcp_hdr.dhcp_file)] = 0;
    }
    else
    {
        memset(dhcp_hdr.dhcp_file, 0, sizeof(dhcp_hdr.dhcp_file));
    }
    dhcp_hdr.dhcp_magic = htonl(DHCP_MAGIC);    /* should this be tunable? */

    n = libnet_pblock_append(l, p, (u_char *)&dhcp_hdr, LIBNET_DHCPV4_H);
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
 
    return (ptag ? ptag : libnet_pblock_update(l, p, h, LIBNET_PBLOCK_DHCPV4_H));
bad:
    libnet_pblock_free(p);
    return (-1);
}


libnet_ptag_t
libnet_build_bootpv4(u_char opcode, u_char htype, u_char hlen, u_char hopcount,
            u_long xid, u_short secs, u_short flags, u_long cip, u_long yip,
            u_long sip, u_long gip, u_char *chaddr, u_char *sname, u_char *file,
            u_char *payload, u_long payload_s, libnet_t *l, libnet_ptag_t ptag)
{
    return (libnet_build_dhcpv4(opcode, htype, hlen, hopcount, xid, secs,
        flags, cip, yip, sip, gip, chaddr, sname, file, payload, payload_s, 
        l, ptag));
}

/* EOF */
