/*
 *  $Id: libnet_build_stp.c,v 1.1.1.1 2002/08/05 22:26:03 route Exp $
 *
 *  libnet
 *  libnet_build_stp.c - STP packet assembler
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
libnet_build_stp_conf(u_short id, u_char version, u_char bpdu_type,
            u_char flags, u_char *root_id, u_long root_pc, u_char *bridge_id,
            u_short port_id, u_short message_age, u_short max_age,
            u_short hello_time, u_short f_delay, u_char *payload,
            u_long payload_s, libnet_t *l, libnet_ptag_t ptag)
{
    u_long n;
    u_short h;
    libnet_pblock_t *p;

    /* until we get some data marshalling in place we can't use this */
    /* struct libnet_stp_conf_hdr stp_hdr; */
    u_char stp_hdr[35];
    u_short value_s;
    u_long value_l;

    if (l == NULL)
    { 
        return (-1);
    } 

    n = LIBNET_STP_CONF_H + payload_s;          /* size of memory block */
    h = 0;                                      /* no checksum */

    /*
     *  Find the existing protocol block if a ptag is specified, or create
     *  a new one.
     */
    p = libnet_pblock_probe(l, ptag, n, LIBNET_PBLOCK_STP_CONF_H);
    if (p == NULL)
    {
        return (-1);
    }

    /* until we get some data marshalling in place we can't use this */
    /*
    stp_hdr.stp_id      = htons(id);
    stp_hdr.stp_version = version;
    stp_hdr.stp_bpdu_type = bpdu_type;
    stp_hdr.stp_flags   = flags;
    memcpy(&stp_hdr.stp_rootid, root_id, 8);
    stp_hdr.stp_rootpc = htonl(root_pc);
    memcpy(&stp_hdr.stp_bridgeid, bridge_id, 8);
    stp_hdr.stp_portid  = htons(port_id);
    stp_hdr.stp_mage    = htons(message_age);
    stp_hdr.stp_maxage  = htons(max_age);
    stp_hdr.stp_hellot  = htons(hello_time);
    stp_hdr.stp_fdelay  = htons(f_delay);
    */

    value_s = htons(id);
    memcpy(stp_hdr, &value_s, 2);
    stp_hdr[2] = version;
    stp_hdr[3] = bpdu_type;
    stp_hdr[4] = flags;
    memcpy(&stp_hdr[5], root_id, 8);
    value_l = htonl(root_pc);
    memcpy(&stp_hdr[13], &value_l, 4);
    memcpy(&stp_hdr[17], bridge_id, 8);
    value_s = htons(port_id);
    memcpy(&stp_hdr[25], &value_s, 2);
    //    value_s = htons(message_age);
    value_s = message_age;
    memcpy(&stp_hdr[27], &value_s, 2);
    //    value_s = htons(max_age);
    value_s = max_age;
    memcpy(&stp_hdr[29], &value_s, 2);
    //    value_s = htons(hello_time);
    value_s = hello_time;
    memcpy(&stp_hdr[31], &value_s, 2);
    //    value_s = htons(f_delay);
    value_s = f_delay;
    memcpy(&stp_hdr[33], &value_s, 2);


    /* until we get some data marshalling in place we can't use this */
    /*n = libnet_pblock_append(l, p, (u_char *)&stp_hdr, LIBNET_STP_CONF_H); */
    n = libnet_pblock_append(l, p, stp_hdr, LIBNET_STP_CONF_H);
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
 
    return (ptag ? ptag : libnet_pblock_update(l, p, h,
            LIBNET_PBLOCK_STP_CONF_H));
bad:
    libnet_pblock_free(p);
    return (-1);
}


libnet_ptag_t
libnet_build_stp_tcn(u_short id, u_char version, u_char bpdu_type,
            u_char *payload, u_long payload_s, libnet_t *l, libnet_ptag_t ptag)
{
    u_long n;
    u_short h;
    libnet_pblock_t *p;

    struct libnet_stp_tcn_hdr stp_hdr;

    if (l == NULL)
    { 
        return (-1);
    } 

    n = LIBNET_STP_TCN_H + payload_s;           /* size of memory block */
    h = 0;                                      /* no checksum */

    /*
     *  Find the existing protocol block if a ptag is specified, or create
     *  a new one.
     */
    p = libnet_pblock_probe(l, ptag, n, LIBNET_PBLOCK_STP_TCN_H);
    if (p == NULL)
    {
        return (-1);
    }

    stp_hdr.stp_id        = htons(id);
    stp_hdr.stp_version   = version;
    stp_hdr.stp_bpdu_type = bpdu_type;

    n = libnet_pblock_append(l, p, (u_char *)&stp_hdr, LIBNET_STP_TCN_H);
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
 
    return (ptag ? ptag : libnet_pblock_update(l, p, h,
            LIBNET_PBLOCK_STP_TCN_H));
bad:
    libnet_pblock_free(p);
    return (-1);
}

/* EOF */
