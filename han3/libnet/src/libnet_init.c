/*
 *  $Id: libnet_init.c,v 1.1.1.1 2002/08/05 22:26:03 route Exp $
 *
 *  libnet
 *  libnet_init.c - Initilization routines.
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

libnet_t *
libnet_init(int injection_type, char *device, char *err_buf)
{
    libnet_t *l = NULL;

#if !defined(__WIN32__)
    if (getuid() && geteuid())
    {
        snprintf(err_buf, LIBNET_ERRBUF_SIZE, "UID != 0\n");
        goto bad;
    }
#else
    WSADATA wsaData;

    if (WSAStartup(MAKEWORD(2, 2), &wsaData))
    {
        /* tell the user that we could not find a usable winsock dll */
        snprintf(err_buf, LIBNET_ERRBUF_SIZE, "WSAStartup\n");
        goto bad;
    }
#endif

    l = (libnet_t *)malloc(sizeof (libnet_t));
    if (l == NULL)
    {
        snprintf(err_buf, LIBNET_ERRBUF_SIZE, "malloc(): %s\n", strerror(errno));
        goto bad;
    }
    memset(l, 0, sizeof (*l));

    l->injection_type   = injection_type;
    l->protocol         = IPPROTO_RAW;
    l->ptag_state       = LIBNET_PTAG_INITIALIZER;
    l->protocol_blocks  = NULL;
    l->pblock_end       = NULL;

    switch (l->injection_type)
    {
        case LIBNET_LINK:
        case LIBNET_LINK_ADV:
            l->device = device;
            if (libnet_select_device(l) == -1)
            {
                snprintf(err_buf, LIBNET_ERRBUF_SIZE,
                        "libnet_select_device(): %s\n", l->err_buf);
                goto bad;
            }
            if (libnet_open_link(l) == -1)
            {
                snprintf(err_buf, LIBNET_ERRBUF_SIZE, l->err_buf);
                goto bad;
            }
            break;
        case LIBNET_RAW4:
        case LIBNET_RAW4_ADV:
            l->device = device;
            /* we might need this later -- it's a good idea to get one now */
            if (libnet_select_device(l) == -1)
            {
                /* this isn't fatal in this case */
                snprintf(err_buf, LIBNET_ERRBUF_SIZE,
                        "libnet_select_device(): %s\n", l->err_buf);
            }
            if (libnet_open_raw4(l) == -1)
            {
                snprintf(err_buf, LIBNET_ERRBUF_SIZE, l->err_buf);
                goto bad;
            }
            break;
        case LIBNET_RAW6:
        case LIBNET_RAW6_ADV:
            l->device = device;
            /* we might need this later -- it's a good idea to get one now */
            if (libnet_select_device(l) == -1)
            {
                /* this isn't fatal in this case */
                snprintf(err_buf, LIBNET_ERRBUF_SIZE,
                        "libnet_select_device(): %s\n", l->err_buf);
            }
            if (libnet_open_raw6(l) == -1)
            {
                snprintf(err_buf, LIBNET_ERRBUF_SIZE, l->err_buf);
                goto bad;
            }
            break;
        default:
            snprintf(err_buf, LIBNET_ERRBUF_SIZE,
                    "unsupported injection type\n");
            goto bad;
            break;
    }

    return (l);

bad:
    if (l)
    {
        libnet_destroy(l);
    }
    return (NULL);
}


void
libnet_destroy(libnet_t *l)
{
    if (l)
    {
        close(l->fd);
        libnet_clear_packet(l);
        free(l);
    }
}


void
libnet_clear_packet(libnet_t *l)
{
    libnet_pblock_t *p;
    libnet_pblock_t *next;

    if (l)
    {
        p = l->protocol_blocks;
        if (p)
        {
            for (; p; p = next)
            {
                next = p->next;
                free(p->buf);
                free(p);
            }
        }
        l->protocol_blocks = NULL;
    }
}


void
libnet_stats(libnet_t *l, struct libnet_stats *ls)
{
    if (l == NULL)
    { 
        return;
    }

    ls->packets_sent    = l->stats.packets_sent;
    ls->packet_errors   = l->stats.packet_errors;
    ls->bytes_written   = l->stats.bytes_written;
}


int
libnet_getfd(libnet_t *l)
{
    if (l == NULL)
    { 
        return (-1);
    } 

    return (l->fd);
}


char *
libnet_getdevice(libnet_t *l)
{
    if (l == NULL)
    { 
        return (NULL);
    }

    return (l->device);
}


u_char *
libnet_getpbuf(libnet_t *l, libnet_ptag_t ptag)
{
    libnet_pblock_t *p;

    if (l == NULL)
    { 
        return (NULL);
    }

    p = libnet_pblock_find(l, ptag);
    if (p == NULL)
    {
        /* err msg set in libnet_pblock_find() */
        return (NULL);
    }
    else
    {
        return (p->buf);
    }
}


u_long
libnet_getpbuf_size(libnet_t *l, libnet_ptag_t ptag)
{
    libnet_pblock_t *p;

    if (l == NULL)
    { 
        return (0);
    } 

    p = libnet_pblock_find(l, ptag);
    if (p == NULL)
    {
        /* err msg set in libnet_pblock_find() */
        return (0);
    }
    else
    {
        return (p->b_len);
    }
}


/* EOF */
