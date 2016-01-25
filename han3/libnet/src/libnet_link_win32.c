/*
 *  $Id: libnet_link_win32.c,v 1.1.1.1 2002/08/05 22:26:03 route Exp $
 *
 *  libnet
 *  libnet_link_win32.c - low-level win32 libwpcap routines
 *
 *  Copyright (c) 2001 - 2002 Don Bowman <don@sandvine.com>
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
#include "packet32.h"

struct libnet_link_int
{
    LPADAPTER  lpAdapter;
};

int
libnet_open_link(libnet_t *l)
{
    DWORD dwErrorCode;
    struct libnet_link_int *lli;
    NetType IFType;

    if (l == NULL)
    { 
        return (-1);
    } 

    if (l->device == NULL)
    {
        snprintf(l->err_buf, LIBNET_ERRBUF_SIZE,
                "libnet_open_link: NULL device\n");
        return (-1);
    }

    lli = (struct libnet_link_int *)calloc(1,sizeof(struct libnet_link_int));
    if (lli == NULL)
    {
        snprintf(l->err_buf, LIBNET_ERRBUF_SIZE,
                "libnet_open_link: no memory\n");
	return (-1);
    }
    l->fd = (int )lli;

    /* open adapter */
    lli->lpAdapter = PacketOpenAdapter(l->device);
    if (!lli->lpAdapter || (lli->lpAdapter->hFile == INVALID_HANDLE_VALUE))
    {
        dwErrorCode=GetLastError();
        snprintf(l->err_buf, LIBNET_ERRBUF_SIZE,
                "Unable to open the driver, Error Code : %lx\n",dwErrorCode); 
        free(lli);
        return (-1);
    }

    /* increase the send buffer */
    PacketSetBuff(lli->lpAdapter,512000);

    if (PacketGetNetType(lli->lpAdapter, &IFType))
    {
        switch(IFType.LinkType)
        {
            case NdisMedium802_3:
                l->link_offset = 0x0e;
                break;
            default:
                snprintf(l->err_buf, LIBNET_ERRBUF_SIZE,
                         "Network type (%d) is not supported",
                         IFType.LinkType);
                free(lli);
                return (-1);
                break;
        }
    }
    else
    {
        dwErrorCode=GetLastError();
        snprintf(l->err_buf, LIBNET_ERRBUF_SIZE,
                "Unable to determine the network type, Error Code : %lx\n",
                dwErrorCode);
        free(lli);
        return (-1);
    }

    return (1);
}

int
libnet_close_link_interface(libnet_t *l)
{
    struct libnet_link_int *lli;

    lli = (struct libnet_link_int *)l->fd;
    if (lli->lpAdapter)
    {
        PacketSetHwFilter(lli->lpAdapter, NDIS_PACKET_TYPE_ALL_LOCAL);
        PacketCloseAdapter(lli->lpAdapter);
    }
    free(lli);
    return (1);
}

int
libnet_write_link(libnet_t *l, u_char *packet, u_long size)
{
    LPPACKET   lpPacket;
    DWORD      BytesTransfered;	
    struct libnet_link_int *lli;

    BytesTransfered = -1;

    lli = (struct libnet_link_int *)l->fd;

    if ((lpPacket = PacketAllocatePacket()) == NULL)
    {
        snprintf(l->err_buf, LIBNET_ERRBUF_SIZE,
                "libnet_write_link:failed to allocate the LPPACKET structure\n");
	return (-1);
    }
    PacketInitPacket(lpPacket, packet, size);

    /* PacketSendPacket returns a BOOLEAN */
    if (PacketSendPacket(lli->lpAdapter, lpPacket, TRUE))
    {
        BytesTransfered = size;
    }

    PacketFreePacket(lpPacket);
    return (BytesTransfered);
}


struct libnet_ether_addr *
libnet_get_hwaddr(libnet_t *l)
{
    struct libnet_ether_addr *ea;

    ea = malloc(sizeof(struct libnet_ether_addr));
    memset(ea, 0, sizeof(struct ether_addr));

    /* XXX - this doesn't work yet */
    return (ea);
}

/* EOF */
