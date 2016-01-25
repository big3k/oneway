/*
 *  $Id: libnet_build_ip.c,v 1.1.1.1 2002/08/05 22:26:03 route Exp $
 *
 *  libnet
 *  libnet_build_ip.c - IP packet assembler
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
libnet_build_ipv4(u_short len, u_char tos, u_short id, u_short frag,
            u_char ttl, u_char prot, u_short sum, u_long src, u_long dst,
            u_char *payload, u_long payload_s, libnet_t *l, libnet_ptag_t ptag)
{
    int i, j, offset;
    u_long n;
    u_short h;
    libnet_pblock_t *p, *p_data, *p_temp;
    struct libnet_ipv4_hdr ip_hdr;
    libnet_ptag_t ptag_data, ptag_hold;

    if (l == NULL)
    { 
        return (-1);
    } 

    n = LIBNET_IPV4_H + payload_s;          /* size of memory block */
    h = len;                                /* header length */
    ptag_data = 0;                          /* used if options are present */

    if (h + payload_s > IP_MAXPACKET)
    {
        sprintf(l->err_buf, "libnet_build_ipv4(): IP packet too large\n");
        return (-1);
    }

    /*
     *  Find the existing protocol block if a ptag is specified, or create
     *  a new one.
     */
    p = libnet_pblock_probe(l, ptag, n, LIBNET_PBLOCK_IPV4_H);
    if (p == NULL)
    {
        return (-1);
    }

    ip_hdr.ip_v          = 4;                         /* version 4 */
    ip_hdr.ip_hl         = 5;                         /* 20 byte header */

    /* check to see if there are IP options to include */
    if (p->prev)
    {
        if (p->prev->type == LIBNET_PBLOCK_IPO_H)
        {
            /*
             *  Count up number of 32-bit words in options list, padding if
             *  neccessary.
             */
            for (i = 0, j = 0; i < p->prev->b_len; i++)
            {
                (i % 4) ? j : j++;
            }
            ip_hdr.ip_hl += j;
        }
    }

    ip_hdr.ip_tos        = tos;                       /* IP tos */
    ip_hdr.ip_len        = htons(h);                  /* total length */
    ip_hdr.ip_id         = htons(id);                 /* IP ID */
    ip_hdr.ip_off        = htons(frag);               /* fragmentation flags */
    ip_hdr.ip_ttl        = ttl;                       /* time to live */
    ip_hdr.ip_p          = prot;                      /* transport protocol */
    ip_hdr.ip_sum        = sum;                       /* checksum */
    ip_hdr.ip_src.s_addr = src;                       /* source ip */
    ip_hdr.ip_dst.s_addr = dst;                       /* destination ip */

    n = libnet_pblock_append(l, p, (u_char *)&ip_hdr, LIBNET_IPV4_H);
    if (n == -1)
    {
        goto bad;
    }

    /* save the original ptag value */
    ptag_hold = ptag;

    /* if this is a new ptag, update the block */
    if (ptag == LIBNET_PTAG_INITIALIZER)
    {
        ptag = libnet_pblock_update(l, p, LIBNET_IPV4_H, LIBNET_PBLOCK_IPV4_H);
    }

    if (payload && payload_s)
    {
       /* find and set the appropriate ptag, or else use the default of 0 */
        if (ptag_hold && p->prev)
        {
            p_temp = p->prev;
            while (p_temp->prev && (p_temp->type != LIBNET_PBLOCK_IPDATA))
            {
                p_temp = p_temp->prev;
            }

            if (p_temp->type == LIBNET_PBLOCK_IPDATA)
            {
                /* save the IP data pblock */
                ptag_data = p_temp->ptag;
                offset = payload_s - p_temp->b_len;
                p->h_len += offset;
            }
            else
            {
                offset = payload_s;
            }
        }

        p_data = libnet_pblock_probe(l, ptag_data, payload_s,
                LIBNET_PBLOCK_IPDATA);
        if (p_data == NULL)
        {
            return (-1);
        }

        if (libnet_pblock_append(l, p, payload, payload_s) == -1)
        {
            goto bad;
        }

        if (ptag_data == LIBNET_PTAG_INITIALIZER)
        {
            if (p_data->prev->type == LIBNET_PBLOCK_IPV4_H)
            {
                libnet_pblock_update(l, p_data, payload_s,
                        LIBNET_PBLOCK_IPDATA);
                /* swap pblocks to correct the protocol order */
                libnet_pblock_swap(l, p->ptag, p_data->ptag);
            }
            else
            {
                /* update without setting this as the final pblock */
                p_data->type  =  LIBNET_PBLOCK_IPDATA;
                p_data->ptag  =  ++(l->ptag_state);
                p_data->h_len =  payload_s;

                /* Adjust h_len for checksum. */
                p->h_len += payload_s;

                /* data was added after the initial construction */
                for (p_temp = l->protocol_blocks;
                        p_temp->type == LIBNET_PBLOCK_IPV4_H ||
                        p_temp->type == LIBNET_PBLOCK_IPO_H;
                        p_temp = p_temp->next)
                {
                    libnet_pblock_insert_before(l, p_temp->ptag, p_data->ptag);
                    break;
                }

                /* The end block needs to have its next pointer cleared. */
                l->pblock_end->next = NULL;
            }

            if (p_data->prev && p_data->prev->type == LIBNET_PBLOCK_IPO_H)
            {
                libnet_pblock_swap(l, p_data->prev->ptag, p_data->ptag);
            }
        }
    }

    if (sum == 0 && l->injection_type != LIBNET_RAW4)
    {
        /*
         *  If checksum is zero, by default libnet will compute a checksum
         *  for the user.  The programmer can override this by calling
         *  libnet_toggle_checksum(l, ptag, 1);
         */
        libnet_pblock_setflags(p, LIBNET_PBLOCK_DO_CHECKSUM);
    }
    return (ptag);
bad:
    libnet_pblock_free(p);
    return (-1);
}

libnet_ptag_t
libnet_autobuild_ipv4(u_short len, u_char prot, u_long dst, libnet_t *l)
{
    int i, j;
    u_long n;
    u_long src;
    u_short h;
    libnet_pblock_t *p;
    libnet_ptag_t ptag;
    struct libnet_ipv4_hdr ip_hdr;

    if (l == NULL)
    { 
        return (-1);
    } 

    n = LIBNET_IPV4_H;                                /* size of memory block */
    h = len;                                          /* header length */
    ptag = LIBNET_PTAG_INITIALIZER;
    src = libnet_get_ipaddr4(l);
    if (src == -1)
    {
        /* err msg set in libnet_get_ipaddr() */ 
        return (-1);
    }

    /*
     *  Create a new pblock.
     */
    p = libnet_pblock_probe(l, ptag, n, LIBNET_PBLOCK_IPV4_H);
    if (p == NULL)
    {
        return (-1);
    }
    ip_hdr.ip_v          = 4;                         /* version 4 */
    ip_hdr.ip_hl         = 5;                         /* 20 byte header */

    /* check to see if there are IP options to include */
    if (p->prev)
    {
        if (p->prev->type == LIBNET_PBLOCK_IPO_H)
        {
            /*
             *  Count up number of 32-bit words in options list, padding if
             *  neccessary.
             */
            for (i = 0, j = 0; i < p->prev->b_len; i++)
            {
                (i % 4) ? j : j++;
            }
            ip_hdr.ip_hl += j;
        }
    }

    ip_hdr.ip_tos        = 0;                         /* IP tos */
    ip_hdr.ip_len        = htons(h);                  /* total length */
    ip_hdr.ip_id         = htons((l->ptag_state) & 0x0000ffff); /* IP ID */
    ip_hdr.ip_off        = 0;                         /* fragmentation flags */
    ip_hdr.ip_ttl        = 64;                        /* time to live */
    ip_hdr.ip_p          = prot;                      /* transport protocol */
    ip_hdr.ip_sum        = 0;                         /* checksum */
    ip_hdr.ip_src.s_addr = src;                       /* source ip */
    ip_hdr.ip_dst.s_addr = dst;                       /* destination ip */

    n = libnet_pblock_append(l, p, (u_char *)&ip_hdr, LIBNET_IPV4_H);
    if (n == -1)
    {
        goto bad;
    }

    if (l->injection_type != LIBNET_RAW4)
    {
        libnet_pblock_setflags(p, LIBNET_PBLOCK_DO_CHECKSUM);
    }
    return (libnet_pblock_update(l, p, LIBNET_IPV4_H, LIBNET_PBLOCK_IPV4_H));
bad:
    libnet_pblock_free(p);
    return (-1);
}

libnet_ptag_t
libnet_build_ipv4_options(u_char *options, u_long options_s, libnet_t *l, 
        libnet_ptag_t ptag)
{
    int i, j, offset, underflow;
    u_long n, adj_size;
    libnet_pblock_t *p, *p_temp;
    struct libnet_ipv4_hdr *ip_hdr;

    if (l == NULL)
    { 
        return (-1);
    }
    underflow = 0;
    offset = 0;

    /* check options list size */
    if (options_s > LIBNET_MAXOPTION_SIZE)
    {
        snprintf(l->err_buf, LIBNET_ERRBUF_SIZE,
            "libnet_build_ipv4_options: options list is too large %ld\n",
            options_s);
        return (-1);
    }

    adj_size = options_s;
    if (adj_size % 4)
    {
        /* size of memory block with padding */
        adj_size += 4 - (options_s % 4);
    }

    /* if this pblock already exists, determine if there is a size diff */
    if (ptag)
    {
        p_temp = libnet_pblock_find(l, ptag);
        if (p_temp)
        {
            if (adj_size >= p_temp->b_len)
            {                                   
                offset = adj_size - p_temp->b_len;                             
            }
            else
            {                                                           
                offset = p_temp->b_len - adj_size;                             
                underflow = 1;
            }
        }
    }

    /*
     *  Find the existing protocol block if a ptag is specified, or create
     *  a new one.
     */
    p = libnet_pblock_probe(l, ptag, adj_size, LIBNET_PBLOCK_IPO_H);
    if (p == NULL)
    {
        return (-1);
    }

    n = libnet_pblock_append(l, p, options, adj_size);
    if (n == -1)
    {
        goto bad;
    }

    if (ptag && p->next)
    {
        p_temp = p->next;
        while ((p_temp->next) && (p_temp->type != LIBNET_PBLOCK_IPV4_H))
        {
            p_temp = p_temp->next;
        }

        /* fix the IP header size */
        if (p_temp->type == LIBNET_PBLOCK_IPV4_H)
        {
            /*
             *  Count up number of 32-bit words in options list, padding if
             *  neccessary.
             */
            for (i = 0, j = 0; i < p->b_len; i++)
            {
                (i % 4) ? j : j++;
            }
            ip_hdr = (struct libnet_ipv4_hdr *) p_temp->buf;
            ip_hdr->ip_hl = j + 5;

            if (!underflow)
            {
                p_temp->h_len += offset;
            }
            else
            {
                p_temp->h_len -= offset;
            }
        }
    }

    return (ptag ? ptag : libnet_pblock_update(l, p, adj_size,
            LIBNET_PBLOCK_IPO_H));
bad:
    libnet_pblock_free(p);
    return (-1);
}

libnet_ptag_t
libnet_build_ipv6(u_char tc, u_long fl, u_short len, u_char nh, u_char hl,
            struct libnet_in6_addr src, struct libnet_in6_addr dst,
            u_char *payload, u_long payload_s, libnet_t *l, libnet_ptag_t ptag)
{   
    u_long n;
    libnet_pblock_t *p;
    struct libnet_ipv6_hdr ip_hdr;

    if (l == NULL)
    {
        return (-1);
    }

    n = LIBNET_IPV6_H + payload_s;          /* size of memory block */
       
    if (LIBNET_IPV6_H + payload_s > IP_MAXPACKET)
    {  
        sprintf(l->err_buf, "libnet_build_ipv6(): IP packet too large\n");
        return (-1);
    }  
       
    /*
     *  Find the existing protocol block if a ptag is specified, or create
     *  a new one.
     */
    p = libnet_pblock_probe(l, ptag, n, LIBNET_PBLOCK_IPV6_H);
    if (p == NULL)
    {   
        return (-1);
    }  
    
       ip_hdr.ip_flags[0] =  0x06 << 4;
       ip_hdr.ip_flags[1] = 0;
       ip_hdr.ip_flags[2] = 0;
       ip_hdr.ip_flags[3] = 0;
       /* XXX: TODO: traffic class, flow label */
       ip_hdr.ip_len      = htons(len);
       ip_hdr.ip_nh       = nh;
       ip_hdr.ip_hl       = hl;
       ip_hdr.ip_src      = src;
       ip_hdr.ip_dst      = dst;
     
    n = libnet_pblock_append(l, p, (u_char *)&ip_hdr, LIBNET_IPV6_H);
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

    if (l->injection_type != LIBNET_RAW6)
    {
        /*
         *  If checksum is zero, by default libnet will compute a checksum
         *  for the user.  The programmer can override this by calling
         *  libnet_toggle_checksum(l, ptag, 1);
         */
        libnet_pblock_setflags(p, LIBNET_PBLOCK_DO_CHECKSUM);
    }
       
    return (ptag ? ptag : libnet_pblock_update(l, p, LIBNET_IPV6_H,
            LIBNET_PBLOCK_IPV6_H));
bad:
    libnet_pblock_free(p);
    return (-1);
}

libnet_ptag_t
libnet_autobuild_ipv6(u_short len, u_char nh, struct libnet_in6_addr dst,
            libnet_t *l)
{

    /* NYI */
    sprintf(l->err_buf, "libnet_autobuild_ipv6(): not yet implemented");
    return (-1);

#if 0
    struct libnet_in6_addr src = libnet_get_ipaddr6(l);

    if (strncmp((char*)&src, (char*)&in6addr_error, sizeof(in6addr_error)) == 0)
    {
        return (-1);
    }

    return (libnet_build_ipv6(0, 0, len, nh, 64, src, dst, NULL, 0, l, 0));
#endif
}

/* EOF */
