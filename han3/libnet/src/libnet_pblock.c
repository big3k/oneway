/*
 *  $Id: libnet_pblock.c,v 1.1.1.1 2002/08/05 22:26:03 route Exp $
 *
 *  libnet
 *  libnet_pblock.c - Memory protocol block routines.
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


libnet_pblock_t *
libnet_pblock_probe(libnet_t *l, libnet_ptag_t ptag, u_long n, u_char type)
{
    int offset;
    libnet_pblock_t *p;

    if (ptag == 0)
    {
        /*
         *  Create a new pblock and enough buffer space for the packet.
         */
        p = libnet_pblock_new(l, n);
        if (p == NULL)
        {
            /* err msg set in libnet_pblock_new() */
            return (NULL);
        }
    }
    else
    {
        /*
         *  Update this pblock, don't create a new one.  Note that if the
         *  new packet size is larger than the old one we will do a malloc.
         */
        p = libnet_pblock_find(l, ptag);

        if (p == NULL)
        {
            /* err msg set in libnet_pblock_find() */
            return (NULL);
        }
        if (p->type != type)
        {
            /*
             *  This is a different type than we were expecting.
             */
            snprintf(l->err_buf, LIBNET_ERRBUF_SIZE,
            "libnet_pblock_probe(): ptag references different type"
            " than expected %d != %d", p->type, type);
            return (NULL); 
        }
        /*
         *  If size is greater than the original block of memory, we need 
         *  to malloc more memory.  Should we use realloc?
         */
        if (n > p->b_len)
        {
            offset = n - p->b_len;  /* how many bytes larger new pblock is */
            free(p->buf);
            p->buf = malloc(n);
            if (p->buf == NULL)
            {
                snprintf(l->err_buf, LIBNET_ERRBUF_SIZE,
                        "libnet_pblock_probe(): tried to resize: %s",
                        strerror(errno));
                return (NULL);
            }
            memset(p->buf, 0, n);
            p->h_len += offset; /* new length for checksums */
            p->b_len = n;       /* new buf len */
        }
        else
        {
            offset = p->b_len - n;
            p->h_len -= offset; /* new length for checksums */
            p->b_len = n;       /* new buf len */
        }
        p->copied = 0;      /* reset copied counter */
    }
    return (p);
}


libnet_pblock_t *
libnet_pblock_new(libnet_t *l, u_long size)
{
    libnet_pblock_t *p;
    /*
     *  Should we do error checking the size of the pblock here, or
     *  should we rely on the underlying operating system to complain when
     *  the user tries to write some ridiculously huge packet?
     */

    /* make the head node if it doesn't exist */
    if (l->protocol_blocks == NULL)
    {
        l->protocol_blocks = malloc(sizeof (libnet_pblock_t));
        if (l->protocol_blocks == NULL)
        {
            goto bad;
        }
        memset(l->protocol_blocks, 0, sizeof (libnet_pblock_t));
        l->protocol_blocks->buf = malloc(size);
        if (l->protocol_blocks->buf == NULL)
        {
            free(l->protocol_blocks);
            l->protocol_blocks = NULL;
            goto bad;
        }
        memset(l->protocol_blocks->buf, 0, size);
        l->protocol_blocks->b_len   = size;
        l->protocol_blocks->copied  = 0;
        l->protocol_blocks->next    = NULL;
        l->protocol_blocks->prev    = NULL;
        return (l->protocol_blocks);
    }
    else
    {
        /* walk to the end of the list */
        for (p = l->protocol_blocks; p->next; p = p->next) ;

        p->next = malloc(sizeof (libnet_pblock_t));
        if (p->next == NULL)
        {
            goto bad;
        }
        memset(p->next, 0, sizeof (libnet_pblock_t));
        p->next->prev = p;
        p = p->next;
        p->buf = malloc(size);
        if (p->buf == NULL)
        {
            free(p);
            p = NULL;
            goto bad;
        }
        memset(p->buf, 0, size);
        p->b_len    = size;
        p->copied   = 0;
        p->next     = NULL;
        return (p);
    }

    bad:
    snprintf(l->err_buf, LIBNET_ERRBUF_SIZE, "libnet_pblock_new(): %s", 
            strerror(errno));
    return (NULL);
}


int
libnet_pblock_swap(libnet_t *l, libnet_ptag_t ptag1, libnet_ptag_t ptag2)
{
    libnet_pblock_t *p1, *p2;

    p1 = libnet_pblock_find(l, ptag1);
    p2 = libnet_pblock_find(l, ptag2);

    if (p1 == NULL || p2 == NULL)
    {
        return (-1);
    }

    p2->prev = p1->prev;
    p1->next = p2->next;
    p2->next = p1;
    p1->prev = p2;

    if (p1->next)
    {
        p1->next->prev = p1;
    }

    if (p2->prev)
    {
        p2->prev->next = p2;
    }
    else
    {
        /* first node on the list */
        l->protocol_blocks = p2;
    }

    if (l->pblock_end == p2)
    {
        l->pblock_end = p1;
    }
    return (1);
}

int
libnet_pblock_insert_before(libnet_t *l, libnet_ptag_t ptag1,
        libnet_ptag_t ptag2)
{
    libnet_pblock_t *p1, *p2;

    p1 = libnet_pblock_find(l, ptag1);
    p2 = libnet_pblock_find(l, ptag2);

    if (p1 == NULL || p2 == NULL)
    {
        sprintf(l->err_buf, "libnet_pblock_find(): could not find ptag");
        return (-1);
    }

    p2->prev = p1->prev;
    p2->next = p1;
    p1->prev = p2;

    if (p2->prev)  
    {
        p2->prev->next = p2;
    }
    else
    {
        /* first node on the list */
        l->protocol_blocks = p2;
    }
    
    return (1);
}
    

void
libnet_pblock_free(libnet_pblock_t *p)
{
    if (p)
    {
        if (p->prev)    /* this had BETTER be the case */
        {
            p->prev->next = NULL;
        }
        if (p->buf)
        {
            free(p->buf);
        }
        free(p);
        p = NULL;
    }
}


libnet_pblock_t *
libnet_pblock_find(libnet_t *l, libnet_ptag_t ptag)
{
    libnet_pblock_t *p;

    for (p = l->protocol_blocks; p; p = p->next)
    {
        if (p->ptag == ptag)
        {
            return (p); 
        }
    }
    snprintf(l->err_buf, LIBNET_ERRBUF_SIZE,
            "libnet_pblock_find(): couldn't find protocol block");
    return (NULL);
}


int
libnet_pblock_append(libnet_t *l, libnet_pblock_t *p, u_char *buf,
            u_long len)
{
    if (p->copied + len > p->b_len)
    {
        snprintf(l->err_buf, LIBNET_ERRBUF_SIZE,
                "libnet_pblock_append(): memcpy would overflow buffer");
        return (-1);
    }
    memcpy(p->buf + p->copied, buf, len);
    p->copied += len;
    return (1);
}


void
libnet_pblock_setflags(libnet_pblock_t *p, u_char flags)
{
    p->flags = flags;
}


libnet_ptag_t
libnet_pblock_update(libnet_t *l, libnet_pblock_t *p, u_short h, u_char type)
{
    p->type  =  type;
    p->ptag  =  ++(l->ptag_state);
    p->h_len = h;
    l->pblock_end = p;              /* point end of pblock list here */

    return (p->ptag);
}


int
libnet_pblock_coalesce(libnet_t *l, u_char **packet, u_long *size)
{
    libnet_pblock_t *p;
    u_long c, n, m;

    /*
     *  Determine the offset required to keep memory aligned (strict
     *  architectures like solaris enforce this, but's a good practice
     *  either way).  This is only required on the link layer with the
     *  14 byte ethernet offset (others are similarly unkind).
     */
    if (l->injection_type == LIBNET_LINK)
    {
        /* 8 byte alignment should work */
       l->aligner = 8 - (l->link_offset % 8);
    }
    else
    {
        l->aligner = 0;
    }

    /*
     *  Get packet chunk sizes and build the packet in memory as we go.
     */
    for (n = l->aligner, p = l->pblock_end; p; p = p->prev)
    {
        /* save the previous memory block size */
        m = n;

        /* get the pblock size */
        n += p->b_len;

        /* get memory for this chunk */
        *packet = realloc(*packet, n);
        if (*packet == NULL)
        {
            snprintf(l->err_buf, LIBNET_ERRBUF_SIZE,
                    "libnet_pblock_coalesce(): %s", strerror(errno));
            return (-1);
        }

        /* copy over the chunk */
        memcpy(*packet + m, p->buf, p->b_len);
    }
    *size = n;

    /*
     *  Finally, see who needs a checksum.
     *  This process should be folded into pblock_update().
     */
    for (n = 0, p = l->pblock_end; p; p = p->prev)
    {
        if ((p->flags) & LIBNET_PBLOCK_DO_CHECKSUM)
        {
            c = libnet_do_checksum(l, l->injection_type == LIBNET_LINK ?
                    *packet + l->link_offset + l->aligner : *packet,
                    libnet_pblock_p2p(p->type), p->h_len);
            if (c == -1)
            {
                /* err msg set in libnet_do_checksum() */
                return (-1);
            }
        }
    }

    /*
     *  Set the packet pointer to the true beginning of the packet and set
     *  the size for transmission.
     */
    if (l->injection_type == LIBNET_LINK && l->aligner)
    {
        *packet += l->aligner;
        *size -= l->aligner;
    }
    return (1);
}


int
libnet_pblock_p2p(u_char type)
{
    /* for checksum; return the protocol number given a pblock type*/
    switch (type)
    {
        case LIBNET_PBLOCK_CDP_H:
            return (LIBNET_PROTO_CDP);
        case LIBNET_PBLOCK_ICMPV4_H:
        case LIBNET_PBLOCK_ICMPV4_ECHO_H:
        case LIBNET_PBLOCK_ICMPV4_MASK_H:
        case LIBNET_PBLOCK_ICMPV4_UNREACH_H:
        case LIBNET_PBLOCK_ICMPV4_TIMXCEED_H:
        case LIBNET_PBLOCK_ICMPV4_REDIRECT_H:
        case LIBNET_PBLOCK_ICMPV4_TS_H:
            return (IPPROTO_ICMP);
        case LIBNET_PBLOCK_IGMP_H:
            return (IPPROTO_IGMP);
        case LIBNET_PBLOCK_IPV4_H:
            return (IPPROTO_IP);
        case LIBNET_ISL_H:
            return (LIBNET_PROTO_ISL);
        case LIBNET_PBLOCK_OSPF_H:
            return (IPPROTO_OSPF);
        case LIBNET_PBLOCK_LS_RTR_H:
            return (IPPROTO_OSPF_LSA);
        case LIBNET_PBLOCK_TCP_H:
            return (IPPROTO_TCP);
        case LIBNET_PBLOCK_UDP_H:
            return (IPPROTO_UDP);
        case LIBNET_PBLOCK_VRRP_H:
            return (IPPROTO_VRRP);
        default:
            return (-1);
    }
}

/* EOF */
