/*
 *  $Id: libnet-macros.h,v 1.1.1.1 2002/08/05 22:26:04 route Exp $
 *
 *  libnet-macros.h - Network routine library macro header file
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

#ifndef __LIBNET_MACROS_H
#define __LIBNET_MACROS_H

/* for systems without snprintf */
#if defined(NO_SNPRINTF)
#define snprintf(buf, len, args...) sprintf(buf, ##args)
#endif


/* for name/address resolutions functions */
#define LIBNET_DONT_RESOLVE 0
#define LIBNET_RESOLVE      1

/* for libnet_toggle_checksum */
#define LIBNET_ON	0
#define LIBNET_OFF	1

/* IPv6 error code */
#ifndef IN6ADDR_ERROR_INIT
#define IN6ADDR_ERROR_INIT { { { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, \
                                 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, \
                                 0xff, 0xff } } }
#endif

/* prand constants */
#define LIBNET_PR2          0
#define LIBNET_PR8          1
#define LIBNET_PR16         2
#define LIBNET_PRu16        3
#define LIBNET_PR32         4
#define LIBNET_PRu32        5
#define LIBNET_PRAND_MAX    0xffffffff

#define LIBNET_MAX_PACKET   0xffff          /* as big as we can get */
#ifndef IP_MAXPACKET
#define IP_MAXPACKET        0xffff
#endif
#ifndef ETHER_ADDR_LEN
#define ETHER_ADDR_LEN      0x6
#endif

#define LIBNET_ERRBUF_SIZE      0x100
#define LIBNET_MAXOPTION_SIZE   0x28

/* some BSD variants have this endianess problem */
#if (LIBNET_BSD_BYTE_SWAP)
#define FIX(n)      ntohs(n)
#define UNFIX(n)    htons(n)
#else
#define FIX(n)      (n)
#define UNFIX(n)    (n)
#endif

/* checksum stuff */
#define LIBNET_CKSUM_CARRY(x) \
    (x = (x >> 16) + (x & 0xffff), (~(x + (x >> 16)) & 0xffff))

/* OSPF stuff */
#define LIBNET_OSPF_AUTHCPY(x, y)  memcpy((u_char *)x, (u_char *)y, sizeof(y))
#define LIBNET_OSPF_CKSUMBUF(x, y) memcpy((u_char *)x, (u_char *)y, sizeof(y))  

/* NTP leap indicator, version, and mode */
#define LIBNET_NTP_DO_LI_VN_MODE(li, vn, md) \
        ((u_char)((((li) << 6) & 0xc0) | (((vn) << 3) & 0x38) | ((md) & 0x7)))

/* not all systems have IFF_LOOPBACK */
#ifdef IFF_LOOPBACK
#define LIBNET_ISLOOPBACK(p) ((p)->ifr_flags & IFF_LOOPBACK)
#else
#define LIBNET_ISLOOPBACK(p) (strcmp((p)->ifr_name, "lo0") == 0)
#endif

/* advanced mode check */
#define LIBNET_ISADVMODE(x) (x & 0x08)

#endif  /* __LIBNET_MACROS_H */

/* EOF */
