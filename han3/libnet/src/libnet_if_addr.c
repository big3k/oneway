/*
 *  $Id: libnet_if_addr.c,v 1.1.1.1 2002/08/05 22:26:03 route Exp $
 *
 *  libnet
 *  libnet_if_addr.c - interface selection code
 *
 *  Copyright (c) 1998 - 2002 Mike D. Schiffman <mike@infonexus.com>
 *  Originally pulled from traceroute sources.
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
#ifdef HAVE_SYS_SOCKIO_H
#include <sys/sockio.h>
#endif
#include "../include/ifaddrlist.h"

#define MAX_IPADDR 32

#if !(__WIN32__)
/*
 *  Return the interface list
 */
int
libnet_ifaddrlist(register struct libnet_ifaddr_list **ipaddrp,
            register char *errbuf)
{
    register int fd, nipaddr;
#ifdef HAVE_SOCKADDR_SA_LEN
    register int n;
#endif
    register struct ifreq *ifrp, *ifend, *ifnext, *mp;
    register struct sockaddr_in *sin;
    register struct libnet_ifaddr_list *al;
    struct ifconf ifc;
    struct ifreq ibuf[MAX_IPADDR], ifr;
    char device[sizeof(ifr.ifr_name) + 1];
    static struct libnet_ifaddr_list ifaddrlist[MAX_IPADDR];

    fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0)
    {
        sprintf(errbuf, "socket: %s", strerror(errno));
        return (-1);
    }
    ifc.ifc_len = sizeof(ibuf);
    ifc.ifc_buf = (caddr_t)ibuf;

    if (ioctl(fd,
            SIOCGIFCONF,
            (char *)&ifc) < 0 || ifc.ifc_len < sizeof(struct ifreq))
    {
        sprintf(errbuf, "SIOCGIFCONF: %s", strerror(errno));
        close(fd);
        return (-1);
    }
    ifrp = ibuf;
    ifend = (struct ifreq *)((char *)ibuf + ifc.ifc_len);

    al = ifaddrlist;
    mp = NULL;
    nipaddr = 0;
    for (; ifrp < ifend; ifrp = ifnext)
    {
#ifdef HAVE_SOCKADDR_SA_LEN
        n = ifrp->ifr_addr.sa_len + sizeof(ifrp->ifr_name);
        if (n < sizeof(*ifrp))
        {
            ifnext = ifrp + 1;
        }
        else
        {
            ifnext = (struct ifreq *)((char *)ifrp + n);
        }
        if (ifrp->ifr_addr.sa_family != AF_INET) continue;
#else
        ifnext = ifrp + 1;
#endif
        /*
         * Need a template to preserve address info that is
         * used below to locate the next entry.  (Otherwise,
         * SIOCGIFFLAGS stomps over it because the requests
         * are returned in a union.)
         */
        strncpy(ifr.ifr_name, ifrp->ifr_name, sizeof(ifr.ifr_name));
        if (ioctl(fd, SIOCGIFFLAGS, (char *)&ifr) < 0)
        {
            if (errno == ENXIO) continue;
            sprintf(errbuf,
                    "SIOCGIFFLAGS: %.*s: %s",
                    (int)sizeof(ifr.ifr_name),
                     ifr.ifr_name,
                     strerror(errno));
            close(fd);
            return (-1);
        }

        /* Must be up and not the loopback */
        if ((ifr.ifr_flags & IFF_UP) == 0 || LIBNET_ISLOOPBACK(&ifr))
        {
            continue;
        }
        
        strncpy(device, ifr.ifr_name, sizeof(ifr.ifr_name));
        device[sizeof(device) - 1] = '\0';
        if (ioctl(fd, SIOCGIFADDR, (char *)&ifr) < 0)
        {
            sprintf(errbuf, "SIOCGIFADDR: %s: %s", device, strerror(errno));
            close(fd);
            return (-1);
        }
    
        sin = (struct sockaddr_in *)&ifr.ifr_addr;
        al->addr = sin->sin_addr.s_addr;
        /*
         *  Replaced savestr() with strdup().  -- MDS
         */
        al->device = strdup(device);
        ++al;
        ++nipaddr;
    }
    close(fd);

    *ipaddrp = ifaddrlist;
    return (nipaddr);
}
#else
#include <packet32.h>
int
libnet_ifaddrlist(register struct libnet_ifaddr_list **ipaddrp,
            register char *errbuf)
{
    int   i = 0;
    DWORD dwVersion;
    DWORD dwWindowsMajorVersion;
    int	  AdapterNum=0;
    ULONG AdapterLength = 512;
    static struct libnet_ifaddr_list ifaddrlist[MAX_IPADDR];
    ULONG ip, netmask;
    
    /*
     *  The data returned by PacketGetAdapterNames is different in Win95 and
     *  in WinNT.
     */
    /* we have to check the os on which we are running */
    dwVersion=GetVersion();
    dwWindowsMajorVersion =  (DWORD)(LOBYTE(LOWORD(dwVersion)));
    if (!(dwVersion >= 0x80000000 && dwWindowsMajorVersion >= 4))
    {  
	/* Windows NT */
	int n;
        /* string that contains a list of the network adapters */
	WCHAR	AdapterName[512];
	char   _aname[512];
	WCHAR	*temp,*temp1;
	n = PacketGetAdapterNames((char *)AdapterName,&AdapterLength);
	temp=AdapterName;
	temp1=AdapterName;
	while ((*temp!='\0')||(*(temp-1)!='\0'))
	{
	    if (*temp=='\0') 
	    {
		char asn[512];
		memset(_aname,0,sizeof(_aname));
		memcpy(_aname, temp1,(temp-temp1)*2);
		temp1=temp+1;
                WideCharToMultiByte(CP_ACP, 0, (LPCWSTR) _aname, -1, asn,
                        sizeof (asn), NULL, NULL);
		ifaddrlist[i].device = strdup(asn);
		i++;
	    }
	    temp++;
	}
	AdapterNum = i;
    }
    else	//windows 95
    {
	/* ascii strings (win95) */
        /* string that contains a list of the network adapters */
	char	AdapterNamea[512];
	char	*tempa,*temp1a;
	PacketGetAdapterNames(AdapterNamea,&AdapterLength);
	tempa=AdapterNamea;
	temp1a=AdapterNamea;

	while ((*tempa!='\0')||(*(tempa-1)!='\0'))
	{
	    if (*tempa=='\0') 
	    {
		char   _aname[512];
		memcpy(_aname,temp1a,tempa-temp1a);
		temp1a=tempa+1;
		ifaddrlist[i].device = strdup(_aname);
		i++;
	    }
	    tempa++;
	}
	AdapterNum=i;
    }
    for (i = 0; i < AdapterNum; i++)
    {
	ip = 0;
	PacketGetNetInfo(ifaddrlist[i].device,&ip,&netmask);
	ifaddrlist[i].addr = ip;
    }
    *ipaddrp = ifaddrlist;
    return AdapterNum;
}
#endif /* __WIN32__ */


int
libnet_select_device(libnet_t *l)
{
    int c, i;
    char err_buf[LIBNET_ERRBUF_SIZE];
    struct libnet_ifaddr_list *address_list;
    u_long addr;

    if (l == NULL)
    { 
        return (-1);
    } 

    /*
     *  Number of interfaces.
     */
    c = libnet_ifaddrlist(&address_list, err_buf);
    if (c < 0)
    {
        snprintf(l->err_buf, LIBNET_ERRBUF_SIZE,
                "ifaddrlist : %s\n", err_buf);
        return (-1);
    }
    else if (c == 0)
    {
        snprintf(l->err_buf, LIBNET_ERRBUF_SIZE,
                "No network interfaces found.\n");
        return (-1);
    }
    if (l->device)
    {
        /*
         *  If the device was specified as an IP address, find the first
         *  device that matches it.
         */
        if (isdigit(l->device[0]))
        {
            /* on error this will be -1 */
            addr = libnet_name2addr4(l, l->device, 0);
        }
        else
        {
            addr = -1;
        }

        for (i = c; i; --i, ++address_list)
        {
            if (addr == -1)
            {
                if (!(strncmp(l->device, address_list->device,
                        strlen(address_list->device))))
                {
                    break;
                }
            }
            else if (address_list->addr == addr)
            {
                l->device = address_list->device;
                break;
            }
        }
        if (i <= 0)
        {
            snprintf(l->err_buf, LIBNET_ERRBUF_SIZE,
                    "Can't find interface %s\n", l->device);
            return (-1);
        }
    }

    /*
     *  Do we need to assign a name to device?
     */
    if (!(l->device))
    {
        l->device = address_list->device;
    }
    return (1);
}

/* EOF */
