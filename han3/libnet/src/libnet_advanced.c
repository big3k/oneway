/*
 *  $Id: libnet_advanced.c,v 1.1.1.1 2002/08/05 22:26:04 route Exp $
 *
 *  libnet
 *  libnet_advanced.c - Advanced routines
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
libnet_adv_cull_packet(libnet_t *l, u_char **packet, u_long *packet_s)
{
    *packet = NULL;
    *packet_s = 0;

    if (!((l->injection_type) & LIBNET_ADV_MASK))
    {
        snprintf(l->err_buf, LIBNET_ERRBUF_SIZE,
            "libnet_adv_cull_packet(): advanced mode not enabled");
            return (-1);
    }

    return (libnet_pblock_coalesce(l, packet, packet_s));
}

int
libnet_adv_write_link(libnet_t *l, u_char *packet, u_long packet_s)
{
    int c;

    if (!((l->injection_type) & LIBNET_ADV_MASK))
    {
        snprintf(l->err_buf, LIBNET_ERRBUF_SIZE,
            "libnet_adv_cull_packet(): advanced mode not enabled");
            return (-1);
    }
    if (l->injection_type != LIBNET_LINK_ADV)
    {
        snprintf(l->err_buf, LIBNET_ERRBUF_SIZE,
            "libnet_adv_cull_packet(): advanced link mode not enabled");
            return (-1);
    }
    c = libnet_write_link(l, packet, packet_s);

    /* do statistics */
    if (c == packet_s)
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
    return (c);
}

/* EOF */
