/*
 *  $Id: libnet_raw.c,v 1.1.1.1 2002/08/05 22:26:03 route Exp $
 *
 *  libnet
 *  libnet_raw.c - raw sockets routines
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
libnet_open_raw4(libnet_t *l)
{
#if !(__WIN32__)
     int one = 1;
#if (__svr4__)
     void *oneptr = &one;
#else
    int *oneptr = &one;
#endif  /* __svr4__ */
#else
    BOOL one;
#endif

    if (l == NULL)
    { 
        return (-1);
    } 

    l->fd = socket(AF_INET, SOCK_RAW, l->protocol);
    if (l->fd == -1)
    {
        snprintf(l->err_buf, LIBNET_ERRBUF_SIZE, 
                "libnet_open_raw: SOCK_RAW allocation failed: %s\n",
                strerror(errno));
        goto bad;
    }
#if !(__WIN32__)
    if (setsockopt(l->fd, IPPROTO_IP, IP_HDRINCL, oneptr, sizeof(one)) == -1)
#else
    one = TRUE;
    if (setsockopt(l->fd, IPPROTO_IP, IP_HDRINCL, (char *)&one,
            sizeof(one)) == -1)
#endif

    {
        snprintf(l->err_buf, LIBNET_ERRBUF_SIZE, 
                "libnet_open_raw: set IP_HDRINCL failed: %s\n",
                strerror(errno));
        goto bad;
    }
#if (__linux__)
    if (setsockopt(l->fd, SOL_SOCKET, SO_BROADCAST, oneptr, sizeof(one)) == -1)
    {
        snprintf(l->err_buf, LIBNET_ERRBUF_SIZE,
                "libnet_open_raw: set SO_BROADCAST failed: %s\n",
                strerror(errno));
        goto bad;
    }
#endif  /* __linux__ */
    return (l->fd);

bad:
    return (-1);    
}


int
libnet_close_raw4(libnet_t *l)
{
    if (l == NULL)
    { 
        return (-1);
    }

    return (close(l->fd));
}

int
libnet_open_raw6(libnet_t *l)
{
#if !(__WIN32__)
     int one = 1;
#if (__svr4__)
     void *oneptr = &one;
#else
    int *oneptr = &one;
#endif  /* __svr4__ */
#else
    BOOL one;
#endif

    if (l == NULL)
    { 
        return (-1);
    } 

    l->fd = socket(PF_INET6, SOCK_RAW, l->protocol);
    if (l->fd == -1)
    {
        snprintf(l->err_buf, LIBNET_ERRBUF_SIZE, 
                "libnet_open_raw6(): SOCK_RAW allocation failed: %s\n",
                strerror(errno));
        goto bad;
    }

#if !(__WIN32__)
    if (setsockopt(l->fd, IPPROTO_IP, IP_HDRINCL, oneptr, sizeof(one)) == -1)
#else
/* XXX need to port this
    one = TRUE;
    if (setsockopt(l->fd, IPPROTO_IP, IP_HDRINCL, (char *)&one,
            sizeof(one)) == -1)
*/
#endif

    {
        snprintf(l->err_buf, LIBNET_ERRBUF_SIZE, 
                "libnet_open_raw6(): set IP_HDRINCL failed: %s\n",
                strerror(errno));
        goto bad;
    }

#if (__linux__)
    if (setsockopt(l->fd, SOL_SOCKET, SO_BROADCAST, oneptr, sizeof(one)) == -1)
    {
        snprintf(l->err_buf, LIBNET_ERRBUF_SIZE,
                "libnet_open_raw6(): set SO_BROADCAST failed: %s\n",
                strerror(errno));
        goto bad;
    }
#endif  /* __linux__ */
    return (l->fd);

bad:
    return (-1);    
}

int
libnet_close_raw6(libnet_t *l)
{
    if (l == NULL)
    { 
         return (-1);
    }
    return (close(l->fd));
}



/* EOF */
