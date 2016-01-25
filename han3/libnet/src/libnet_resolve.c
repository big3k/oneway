/*
 *  $Id: libnet_resolve.c,v 1.1.1.1 2002/08/05 22:26:03 route Exp $
 *
 *  libnet
 *  libnet_resolve.c - various name resolution type routines
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

u_char *
libnet_addr2name4(u_long in, u_short use_name)
{
    static u_char hostname[512], hostname2[512];
    static u_short which;
    u_char *p;

    struct hostent *host_ent = NULL;
    struct in_addr addr;

    /*
     *  Swap to the other buffer.  We swap static buffers to avoid having to
     *  pass in a char *.  This makes the code that calls this function more
     *  intuitive, but makes this function ugly.  This function is seriously
     *  non-reentrant.  For threaded applications (or for signal handler code)
     *  use host_lookup_r().
     */
    which++;
    
    if (use_name == LIBNET_RESOLVE)
    {
        addr.s_addr = in;
        host_ent = gethostbyaddr((char *)&addr, sizeof(struct in_addr), AF_INET);
        /* if this fails, we silently ignore the error and move to plan b! */
    }
    if (!host_ent)
    {

        p = (u_char *)&in;
        sprintf(((which % 2) ? hostname : hostname2),  "%d.%d.%d.%d",
                (p[0] & 255), (p[1] & 255), (p[2] & 255), (p[3] & 255));
    }
    else if (use_name == LIBNET_RESOLVE)
    {
        strncpy(((which % 2) ? hostname : hostname2), host_ent->h_name, 
                                                        sizeof(hostname));
    }
    return (which % 2) ? (hostname) : (hostname2);
}

void
libnet_addr2name4_r(u_long in, u_short use_name, u_char *hostname,
        int hostname_len)
{
    u_char *p;
    struct hostent *host_ent = NULL;
    struct in_addr addr;

    if (use_name == LIBNET_RESOLVE)
    {    
        addr.s_addr = in;
        host_ent = gethostbyaddr((char *)&addr, sizeof(struct in_addr),
                AF_INET);
    }
    if (!host_ent)
    {
        p = (u_char *)&in;
        sprintf(hostname, "%d.%d.%d.%d",
                (p[0] & 255), (p[1] & 255), (p[2] & 255), (p[3] & 255));
    }
    else
    {
        strncpy(hostname, host_ent->h_name, hostname_len);
    }
}

u_long
libnet_name2addr4(libnet_t *l, u_char *host_name, u_short use_name)
{
    struct in_addr addr;
    struct hostent *host_ent; 
    u_long m;
    u_int val;
    int i;
   
    if (use_name == LIBNET_RESOLVE)
    {
        if ((addr.s_addr = inet_addr(host_name)) == -1)
        {
            if (!(host_ent = gethostbyname(host_name)))
            {
                snprintf(l->err_buf, LIBNET_ERRBUF_SIZE,
                        "libnet_name2addr4(): %s", strerror(errno));
                /* XXX - this is actually 255.255.255.255 */
                return (-1);
            }
            memcpy(&addr.s_addr, host_ent->h_addr, host_ent->h_length);
        }
        /* host byte order */
        return (addr.s_addr);
    }
    else
    {
        /*
         *  We only want dots 'n decimals.
         */
        if (!isdigit(host_name[0]))
        {
            if (l)
            {
                snprintf(l->err_buf, LIBNET_ERRBUF_SIZE,
                    "libnet_name2addr4(): expecting dots and decimals\n");
            }
            /* XXX - this is actually 255.255.255.255 */
            return (-1);
        }

        m = 0;
        for (i = 0; i < 4; i++)
        {
            m <<= 8;
            if (*host_name)
            {
                val = 0;
                while (*host_name && *host_name != '.')
                {   
                    val *= 10;
                    val += *host_name - '0';
                    if (val > 255)
                    {
                        if (l)
                        {
                            snprintf(l->err_buf, LIBNET_ERRBUF_SIZE,
                                "libnet_name2addr4(): value greater than 255\n");
                        }
                        /* XXX - this is actually 255.255.255.255 */
                        return (-1);
                    }
                    host_name++;
                }
                m |= val;
                if (*host_name)
                {
                    host_name++;
                }
            }
        }
        /* host byte order */
       return (ntohl(m));
    }
}

void
libnet_addr2name6_r(struct libnet_in6_addr addr, u_short use_name,
            u_char *hostname, int hostname_len)
{
    struct hostent *host_ent = NULL;

    if (use_name == LIBNET_RESOLVE)
    {    
        host_ent = gethostbyaddr((char *)&addr, sizeof(struct in_addr),
                AF_INET6);
    }
    if (!host_ent)
    {
        sprintf(hostname, "%x:%x:%x:%x:%x:%x:%x:%x",
                ntohs(addr.libnet_s6_addr[0]),
                ntohs(addr.libnet_s6_addr[1]),
                ntohs(addr.libnet_s6_addr[2]),
                ntohs(addr.libnet_s6_addr[3]),
                ntohs(addr.libnet_s6_addr[4]),
                ntohs(addr.libnet_s6_addr[5]),
                ntohs(addr.libnet_s6_addr[6]),
                ntohs(addr.libnet_s6_addr[7]));
    }
    else
    {
        strncpy(hostname, host_ent->h_name, hostname_len);
    }
}

const struct libnet_in6_addr in6addr_error = IN6ADDR_ERROR_INIT;

struct libnet_in6_addr
libnet_name2addr6(libnet_t *l, u_char *host_name, u_short use_name)
{
    unsigned int tmp[8];
    struct libnet_in6_addr addr;
    struct hostent *host_ent; 
    int i;
   
    if (use_name == LIBNET_RESOLVE)
    {
        if (!(host_ent = gethostbyname2(host_name, AF_INET6)))
        {
            snprintf(l->err_buf, LIBNET_ERRBUF_SIZE,
                    "libnet_name2addr6(): %s", strerror(errno));
            return (in6addr_error);
        }
        memcpy(&addr, host_ent->h_addr, host_ent->h_length);
        return addr;
    }
    else
    {
        if (!isxdigit(host_name[0]))
        {
            if (l)
            {
                snprintf(l->err_buf, LIBNET_ERRBUF_SIZE,
                    "libnet_name2addr6(): expecting hexadecimal format\n");
            }
            return (in6addr_error);
        }
        if (sscanf(host_name,"%x:%x:%x:%x:%x:%x:%x:%x",
                    &tmp[0], &tmp[1], &tmp[2], &tmp[3], &tmp[4], &tmp[5],
                    &tmp[6], &tmp[7]) < 8)
        {
            if (l)
            {
                snprintf(l->err_buf, LIBNET_ERRBUF_SIZE,
                "libnet_name2addr6(): value greater than 0xffff\n");
            }
                return (in6addr_error);
        }
        for (i = 0; i < 8; i++)
        {
            addr.libnet_s6_addr[i] = htons(tmp[i]);
        }
        return (addr);
    }
}

struct libnet_in6_addr
libnet_get_ipaddr6(libnet_t *l)
{
    snprintf(l->err_buf, LIBNET_ERRBUF_SIZE,
           "libnet_get_ipaddr6(): Not yet Implemented.\n");
    return (in6addr_error);
}

#if !defined(__WIN32__)
u_long
libnet_get_ipaddr4(libnet_t *l)
{
    struct ifreq ifr;
    register struct sockaddr_in *sin;
    int fd;

    if (l == NULL)
    {
        return (-1);
    }

    /*
     *  Create dummy socket to perform an ioctl upon.
     */
    fd = socket(PF_INET, SOCK_DGRAM, 0);
    if (fd == -1)
    {
        snprintf(l->err_buf, LIBNET_ERRBUF_SIZE,
                "libnet_get_ipaddr4 socket: %s", strerror(errno));
        return (-1);
    }

    memset(&ifr, 0, sizeof(ifr));
    sin = (struct sockaddr_in *)&ifr.ifr_addr;

    if (l->device == NULL)
    {
        if (libnet_select_device(l) == -1)
        {
            snprintf(l->err_buf, LIBNET_ERRBUF_SIZE,
                    "libnet_get_ipaddr4: can't figure out a device to use\n");
            close(fd);
            return (-1);
        }
    }
    strncpy(ifr.ifr_name, l->device, sizeof(ifr.ifr_name));
    ifr.ifr_addr.sa_family = AF_INET;

    if (ioctl(fd, SIOCGIFADDR, (char*) &ifr) < 0)
    {
        snprintf(l->err_buf, LIBNET_ERRBUF_SIZE,
                "libnet_get_ipaddr4 ioctl: %s", strerror(errno));
        close(fd);
        return (-1);
    }
    close(fd);
    return (sin->sin_addr.s_addr);
}
#else
#include "packet32.h"
u_long
libnet_get_ipaddr4(libnet_t *l)
{
    ULONG ip, netmask;
    PacketGetNetInfo(l->device, &ip, &netmask);
    return (ip);
}
#endif /* WIN32 */

u_char *
libnet_hex_aton(char *s, int *len)
{
    u_char *buf;
    int i;
    long l;
    char *pp;
        
    while (isspace(*s))
    {
        s++;
    }
    for (i = 0, *len = 0; s[i]; i++)
    {
        if (s[i] == ':')
        {
            (*len)++;
        }
    }
    buf = malloc(*len + 1);
    if (buf == NULL)
    {
        return (NULL);
    }
    /* expect len hex octets separated by ':' */
    for (i = 0; i < *len + 1; i++)
    {
        l = strtol(s, &pp, 16);
        if (pp == s || l > 0xFF || l < 0)
        {
            *len = 0;
            free(buf);
            return (NULL);
        }
        if (!(*pp == ':' || (i == *len && (isspace(*pp) || *pp == '\0'))))
        {
            *len = 0;
            free(buf);
            return (NULL);
        }
        buf[i] = (u_char)l;
        s = pp + 1;
    }
    /* return character after the octets ala strtol(3) */
    (*len)++;
    return (buf);
}

/* EOF */
