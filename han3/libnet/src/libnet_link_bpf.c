/*
 *  $Id: libnet_link_bpf.c,v 1.1.1.1 2002/08/05 22:26:03 route Exp $
 *
 *  libnet
 *  libnet_link_bpf.c - low-level bpf routines
 *
 *  Copyright (c) 1998 - 2002 Mike D. Schiffman <mike@infonexus.com>
 *  All rights reserved.
 *
 * Copyright (c) 1993, 1994, 1995, 1996, 1998
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that: (1) source code distributions
 * retain the above copyright notice and this paragraph in its entirety, (2)
 * distributions including binary code include the above copyright notice and
 * this paragraph in its entirety in the documentation or other materials
 * provided with the distribution, and (3) all advertising materials mentioning
 * features or use of this software display the following acknowledgement:
 * ``This product includes software developed by the University of California,
 * Lawrence Berkeley Laboratory and its contributors.'' Neither the name of
 * the University nor the names of its contributors may be used to endorse
 * or promote products derived from this software without specific prior
 * written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */

#include <sys/param.h>  /* optionally get BSD define */
#include <sys/timeb.h>
#include <sys/file.h>
#include <sys/ioctl.h>

#include <sys/types.h>
#include <sys/time.h>
#include <net/bpf.h>

#if (HAVE_CONFIG_H)
#include "../include/config.h"
#endif 
#include "../include/libnet.h"
#include <sys/sysctl.h>
#include <net/route.h>
#include <net/if_dl.h>
#include "../include/gnuc.h"
#include "../include/bpf.h"

#ifdef HAVE_OS_PROTO_H
#include "../include/os-proto.h"
#endif

int
libnet_bpf_open(char *err_buf)
{
    int i, fd;
    char device[sizeof "/dev/bpf000"];

    /*
     *  Go through all the minors and find one that isn't in use.
     */
    for (i = 0;;i++)
    {
        sprintf(device, "/dev/bpf%d", i);

        fd = open(device, O_RDWR);
        if (fd == -1 && errno == EBUSY)
        {
            /*
             *  Device is busy.
             */
            continue;
        }
        else
        {
            /*
             *  Either we've got a valid file descriptor, or errno is not
             *  EBUSY meaning we've probably run out of devices.
             */
            break;
        }
    }

    if (fd == -1)
    {
        snprintf(err_buf, LIBNET_ERRBUF_SIZE, "libnet_bpf_open: %s: %s",
                device, strerror(errno));
    }
    return (fd);
}


int
libnet_open_link(libnet_t *l)
{
    struct ifreq ifr;
    struct bpf_version bv;
    u_int v;

#if defined(BIOCGHDRCMPLT) && defined(BIOCSHDRCMPLT)
    u_int spoof_eth_src = 1;
#endif

    if (l == NULL)
    { 
        return (-1);
    } 

    if (l->device == NULL)
    {
        snprintf(l->err_buf, LIBNET_ERRBUF_SIZE,
                "libnet_open_link: NULL device\n");
        goto bad;
    }

    l->fd = libnet_bpf_open(l->err_buf);
    if (l->fd == -1)
    {
        goto bad;
    }

    /*
     *  Get bpf version.
     */
    if (ioctl(l->fd, BIOCVERSION, (caddr_t)&bv) < 0)
    {
        snprintf(l->err_buf, LIBNET_ERRBUF_SIZE,
                "libnet_open_link: BIOCVERSION: %s\n", strerror(errno));
        goto bad;
    }

    if (bv.bv_major != BPF_MAJOR_VERSION || bv.bv_minor < BPF_MINOR_VERSION)
    {
        snprintf(l->err_buf, LIBNET_ERRBUF_SIZE,
                "libnet_open_link: kernel bpf filter out of date\n");
        goto bad;
    }

    /*
     *  Attach network interface to bpf device.
     */
    strncpy(ifr.ifr_name, l->device, sizeof(ifr.ifr_name));
    if (ioctl(l->fd, BIOCSETIF, (caddr_t)&ifr) == -1)
    {
        snprintf(l->err_buf, LIBNET_ERRBUF_SIZE,
                "libnet_open_link: BIOCSETIF: %s: %s", l->device,
                strerror(errno));
        goto bad;
    }

    /*
     *  Get the data link-layer type.
     */
    if (ioctl(l->fd, BIOCGDLT, (caddr_t)&v) == -1)
    {
        snprintf(l->err_buf, LIBNET_ERRBUF_SIZE,
                "libnet_open_link: BIOCGDLT: %s", strerror(errno));
        goto bad;
    }

    /*
     *  NetBSD and FreeBSD BPF have an ioctl for enabling/disabling
     *  automatic filling of the link level source address.
     */
#if defined(BIOCGHDRCMPLT) && defined(BIOCSHDRCMPLT)
    if (ioctl(l->fd, BIOCSHDRCMPLT, &spoof_eth_src) == -1)
    {
        snprintf(l->err_buf, LIBNET_ERRBUF_SIZE,
                "libnet_open_link: BIOCSHDRCMPLT: %s", strerror(errno));
        goto bad;
    }
#endif

    /*
     *  Assign link type and offset.
     */
    switch (v)
    {
        case DLT_SLIP:
            l->link_offset = 0x10;
            break;
        case DLT_RAW:
            l->link_offset = 0x0;
            break;
        case DLT_PPP:
            l->link_offset = 0x04;
            break;
        case DLT_EN10MB:
        default:
            l->link_offset = 0xe;     /* default to ethernet */
            break;
    }
#if _BSDI_VERSION - 0 >= 199510
    switch (v)
    {
        case DLT_SLIP:
            v = DLT_SLIP_BSDOS;
            l->link_offset = 0x10;
            break;
        case DLT_PPP:
            v = DLT_PPP_BSDOS;
            l->link_offset = 0x04;
            break;
    }
#endif
    l->link_type = v;

    return (1);

bad:
    if (l->fd > 0)
    {
        close(l->fd);      /* this can fail ok */
    }
    return (-1);
}


int
libnet_close_link(libnet_t *l)
{
    if (close(l->fd) == 0)
    {
        return (1);
    }
    else
    {
        return (-1);
    }
}


int
libnet_write_link(libnet_t *l, u_char *packet, u_long size)
{
    int c;

    if (l == NULL)
    { 
        return (-1);
    } 

    c = write(l->fd, packet, size);
    if (c != size)
    {
        snprintf(l->err_buf, LIBNET_ERRBUF_SIZE,
                "libnet_write_link: %d bytes written (%s)\n", c,
                strerror(errno));
    }
    return (c);
}


struct libnet_ether_addr *
libnet_get_hwaddr(libnet_t *l)
{
    int mib[6];
    size_t len;
    char *buf, *next, *end;
    struct if_msghdr *ifm;
    struct sockaddr_dl *sdl;
    struct libnet_ether_addr *ea = NULL;

    mib[0] = CTL_NET;
    mib[1] = AF_ROUTE;
    mib[2] = 0;
    mib[3] = AF_LINK;
    mib[4] = NET_RT_IFLIST;
    mib[5] = 0;

    if (l == NULL)
    { 
        return (NULL);
    } 

    if (l->device == NULL)
    {           
        if (libnet_select_device(l) == -1)
        {   
            snprintf(l->err_buf, LIBNET_ERRBUF_SIZE,
                    "libnet_get_hwaddr: can't figure out a device to use\n");
            return (NULL);
        }
    }

    if (sysctl(mib, 6, NULL, &len, NULL, 0) == -1)
    {
        snprintf(l->err_buf, LIBNET_ERRBUF_SIZE, "libnet_get_hwaddr: %s",
                strerror(errno));
        return (NULL);
    }

    buf = (char *)malloc(len);
    if (buf == NULL)
    {
        snprintf(l->err_buf, LIBNET_ERRBUF_SIZE, "libnet_get_hwaddr: %s",
                strerror(errno));
        return (NULL);
    }
    if (sysctl(mib, 6, buf, &len, NULL, 0) < 0)
    {
        snprintf(l->err_buf, LIBNET_ERRBUF_SIZE, "libnet_get_hwaddr: %s",
                strerror(errno));
        free(buf);
        return (NULL);
    }
    end = buf + len;

    for (next = buf ; next < end ; next += ifm->ifm_msglen)
    {
        ifm = (struct if_msghdr *)next;
        if (ifm->ifm_type == RTM_IFINFO)
        {
            sdl = (struct sockaddr_dl *)(ifm + 1);
            if (strncmp(&sdl->sdl_data[0], l->device, sdl->sdl_nlen) == 0)
            {
                if (!(ea = malloc(sizeof(struct libnet_ether_addr))))
                {
                    snprintf(l->err_buf, LIBNET_ERRBUF_SIZE,
                            "libnet_get_hwaddr: %s", strerror(errno));
                    free(buf);
                    return (NULL);
                }
                memcpy(ea->ether_addr_octet, LLADDR(sdl), ETHER_ADDR_LEN);
                break;
            }
        }
    }
    free(buf);
    return (ea);
}


/* EOF */
