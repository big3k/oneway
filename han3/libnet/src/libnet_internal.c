/*
 *  $Id: libnet_internal.c,v 1.1.1.1 2002/08/05 22:26:03 route Exp $
 *
 *  libnet
 *  libnet_internal.c - secret routines!
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


void
__libnet_hex_dump(u_char *packet, u_long len, int swap, FILE *stream)
{
    int i, s_cnt;
    u_short *p;

    p     = (u_short *)packet;
    s_cnt = len / sizeof(u_short);

    fprintf(stream, "\t");
    for (i = 0; --s_cnt >= 0; i++)
    {
        if ((!(i % 8)))
        {
            fprintf(stream, "\n%02x\t", (i * 2));
        }
        fprintf(stream, "%04x ", swap ? ntohs(*(p++)) : *(p++));
    }

    /*
     *  Mop up an odd byte.
     */
    if (len & 1)
    {
        if ((!(i % 8)))
        {
            fprintf(stream, "\n%02x\t", (i * 2));
        }
        fprintf(stream, "%02x ", *(u_char *)p);
    }
    fprintf(stream, "\n");
}


void
__libnet_handle_dump(libnet_t *l)
{
    struct protoent *p_ent;
 
    if (l == NULL)
    { 
        return;
    } 

    fprintf(stderr, "fd:\t\t%d\n", l->fd);
 
    p_ent = getprotobynumber(l->protocol);
    fprintf(stderr, "protocol:\t%s (%d)\n", p_ent->p_name, l->protocol);
 
    switch (l->injection_type)
    {
        case LIBNET_LINK:
            fprintf(stderr, "injection type:\tLIBNET_LINK\n");
            break;
        case LIBNET_RAW4:
            fprintf(stderr, "injection type:\tLIBNET_RAW4\n");
            break;
        case LIBNET_RAW6:
            fprintf(stderr, "injection type:\tLIBNET_RAW6\n");
            break;
        default:
            fprintf(stderr, "injection type:\t0D4YPR0+0<0|_\n");
            break;
    }
 
    fprintf(stderr, "device:\t\t%s\n", l->device);
    fprintf(stderr, "link type:\t%d\n", l->link_type);
    fprintf(stderr, "link offset:\t%d\n", l->link_offset);
    fprintf(stderr, "packets sent:\t%ld\n", l->stats.packets_sent);
    fprintf(stderr, "packet errors:\t%ld\n", l->stats.packet_errors);
    fprintf(stderr, "bytes written:\t%ld\n", l->stats.bytes_written);
}


/* EOF */
