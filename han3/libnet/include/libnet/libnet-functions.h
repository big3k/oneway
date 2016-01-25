/*
 *  $Id: libnet-functions.h,v 1.1.1.1 2002/08/05 22:26:04 route Exp $
 *
 *  libnet-functions.h - function prototypes
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

#ifndef __LIBNET_FUNCTIONS_H
#define __LIBNET_FUNCTIONS_H

/*
 *  libnet_init
 *
 *  Function initializes the library for use.  It allocates memory for the
 *  monolithic control structure and sets various bits of state in the
 *  library.  The resulting file context l is passed to nearly every function
 *  in libnet.
 */
libnet_t *              /* libnet file context or NULL on error */
libnet_init(
    int,                /* packet injection_type (raw, raw6, or link) */
    char *,             /* device (for link only NULL for libnet to choose) */
    char *              /* error buffer */
    );


/*
 *  libnet_destroy
 *
 *  Function is a decontructor for libnet.  Should be called at the end of
 *  program or when a fatal error occurs.
 */
void
libnet_destroy(
    libnet_t *          /* libnet context pointer */
    );


/*
 *  libnet_clear_packet
 *
 *  Function clears the current packet that has been assembled and frees
 *  the memory associated.  Should be called when you want to build and
 *  send packet of a different type.
 */
void
libnet_clear_packet(
    libnet_t *          /* libnet context pointer */
    );


/*
 *  libnet_stats
 *
 *  Function fills in a libnet_stats structure with the statisics for the
 *  supplied libnet context.
 */
void
libnet_stats(
    libnet_t *,         /* libnet context pointer */
    struct libnet_stats */* libnet statistics pointer */
    );


/*
 *  libnet_getfd
 *
 *  Function returns the FILENO of the file descriptor used for packet
 *  injection.
 */
int                     /* FILENO of the fd */
libnet_getfd(
    libnet_t *          /* libnet context pointer */
    );


/*
 *  libnet_getdevice
 *
 *  Function returns the device name used for packet injection.
 */
char *                  /* device name */
libnet_getdevice(
    libnet_t *          /* libnet context pointer */
    );


/*
 *  libnet_getpbuf
 *
 *  Function returns the pblock buffer for the specified ptag.
 */
u_char *                /* the pblock buffer or NULL on error */
libnet_getpbuf(
    libnet_t *,         /* libnet context pointer */
    libnet_ptag_t       /* pblock to return information on */
    );


/*
 *  libnet_getpbuf_size
 *
 *  Function returns the pblock buffer size for the specified ptag.
 */
u_long                  /* size of the pblock buffer or 0 on error */
libnet_getpbuf_size(
    libnet_t *,         /* libnet context pointer */
    libnet_ptag_t       /* pblock to return information on */
    );


/*
 *  libnet_geterror
 *
 *  Function returns the last error string set inside of l.
 */
char *                  /* the error string (could be NULL) */
libnet_geterror(
    libnet_t *          /* libnet context pointer */
    );


/*
 *  libnet_seed_prand
 *
 *  Function seeds the psuedo-random number generator.
 */
int                     /* 1 on success, -1 on failure */
libnet_seed_prand(
    libnet_t *          /* libnet context pointer */
    );


/*
 *  libnet_get_prand
 *
 *  Function returns a psuedo-random integer of the range specified by the
 *  modulous.
 */
u_long                  /* the psuedo-random integer */
libnet_get_prand(
    int                 /* One of the PR* constants */
    );


/*
 *  libnet_toggle_checksum
 *
 *  Function switches the checksum on or off.  If off, libnet will NOT
 *  calculate a checksum for this protocol block, if on libnet WILL calculate
 *  a checksum for this protocol block assuming it has a checksum field.
 */
int
libnet_toggle_checksum(
    libnet_t *,         /* libnet context pointer */
    libnet_ptag_t,      /* pblock to toggle checksum flag on */
    int                 /* 1 == on, 0 == off */
    );


/*
 *  libnet_addr2name4
 *
 *  Function takes a network byte ordered IPv4 address and returns the
 *  DNS name for it (if it has one) or a string of dotted decimals.  This 
 *  may incur a DNS lookup (set mode to 0 to not do the lookup).
 *  XXX - This function is not-reentrant.
 */
u_char *                /* pointer to hostname or dotted decimal IP address */
libnet_addr2name4(
    u_long,             /* network byte ordered (big endian) IP address */
    u_short             /* use domain names or no */
    );


/*
 *  libnet_addr2name4_r
 *
 *  Function takes a network byte ordered IPv4 address and returns the
 *  DNS name for it (if it has one) or a string of dotted decimals.  This 
 *  may incur a DNS lookup (set mode to 0 to not do the lookup).
 */
void
libnet_addr2name4_r(
    u_long,             /* network byte ordered (big endian) IP address */
    u_short,            /* use domain names or no */
    u_char *,           /* pointer to hostname or dotted decimal IP address */
    int                 /* size of the hostname buffer passed in */
    );

/*
 *  libnet_name2addr4
 *
 *  Function takes a dotted decimal string or hostname and returns a big
 *  endian IPv4 address.  This may incur a DNS lookup (set mode to 0 to 
 *  not do the lookup).
 */
u_long                  /* network byte ordered IP address or -1 on error */
libnet_name2addr4(
    libnet_t *,         /* libnet context pointer */
    u_char *,           /* pointer the hostname or dotted decimal IP address */
    u_short             /* use domain names or no */
    );

/*
 *  libnet_name2addr6_r
 */
void
libnet_addr2name6_r(
    struct libnet_in6_addr addr, /* IPv6 address to convert */
    u_short,            /* use domain names or no */
    u_char *,           /* pointer to hostname or dotted decimal IP address */
    int                 /* size of the hostname buffer passed in */
    );

/*
 *  libnet_name2addr6
 *
 */ 
extern const struct libnet_in6_addr in6addr_error;
struct libnet_in6_addr  /* network byte ordered IP address */
libnet_name2addr6(
    libnet_t *,         /* libnet handle pointer */
    u_char *,           /* pointer the hostname or dotted decimal IP address */
    u_short             /* use domain names or no */
    );


/*
 *  libnet_plist_chain_new
 *
 *  Function creates a new port list list.  The token list should be a
 *  character string containing along the lines of the following: "xx-yy,z"
 */
int                     /* 1 on success, -1 on failure */
libnet_plist_chain_new(
    libnet_t *,         /* libnet context pointer */
    libnet_plist_t **,  /* pointer to the head of the list */
    char *              /* token list pointer */
    );


/*
 *  libnet_plist_chain_next_pair
 *
 *  Function returns the next pair of port list numbers, from beginning
 *  to end.  If there is only one port number, bport will == eport.
 */
int                     /* 1 if more nodes, 0 if not */
libnet_plist_chain_next_pair(
    libnet_plist_t *,   /* pointer to the head of the list */
    u_short *,          /* holds bport */
    u_short *           /* holds eport */
    );


/*
 *  libnet_plist_chain_next_dump
 *
 *  Function dumps the port list list to stdout.
 */
int                     /* 1 on success, -1 on failure (if !p) */
libnet_plist_chain_dump(
    libnet_plist_t *    /* pointer to the head of the list */
    );


/*
 *  libnet_plist_chain_dump_string
 *
 *  Function returns the port list list as a string.
 *  XXX - This function is not-reentrant.
 */
u_char *                /* the port list string on success, NULL on failure */
libnet_plist_chain_dump_string(
    libnet_plist_t *    /* pointer to the head of the list */
    );


/*
 *  libnet_plist_chain_free
 *
 *  Function destroys the list and frees memory.
 */
int
libnet_plist_chain_free(
    libnet_plist_t *    /* pointer to the head of the list */
    );


/*
 *  libnet_build_802_1q
 *
 *  Function builds an 802.1q header.
 */
libnet_ptag_t           /* packet id on success, -1 on failure */
libnet_build_802_1q(
    u_char *,           /* pointer to a 6 byte ethernet address */
    u_char *,           /* pointer to a 6 byte ethernet address */
    u_short,            /* tag protocol ID */
    u_char,             /* priority */
    u_char,             /* canonical format indicator */
    u_short,            /* vid */
    u_short,            /* length or protocol (802.3 / ethernet II) */
    u_char *,           /* payload (or NULL) */
    u_long,             /* payload length */
    libnet_t *,         /* libnet context pointer */
    libnet_ptag_t       /* packet id */
    );


/*
 *  libnet_build_802_2
 *
 *  Function builds an 802.2 LLC header.
 */
libnet_ptag_t           /* packet id on success, -1 on failure */
libnet_build_802_2(
    u_char,             /* DSAP */
    u_char,             /* SSAP */
    u_char,             /* control */
    u_char *,           /* payload (or NULL) */
    u_long,             /* payload length */
    libnet_t *,         /* libnet context pointer */
    libnet_ptag_t       /* packet id */
    );


/*
 *  libnet_build_802_2snap
 *
 *  Function builds an 802.2 LLC/SNAP header.
 */
libnet_ptag_t           /* packet id on success, -1 on failure */
libnet_build_802_2snap(
    u_char,             /* DSAP */
    u_char,             /* SSAP */
    u_char,             /* control */
    u_char *,           /* OUI */
    u_short,            /* type */
    u_char *,           /* payload (or NULL) */
    u_long,             /* payload length */
    libnet_t *,         /* libnet context pointer */
    libnet_ptag_t       /* packet id */
    );


/*
 *  libnet_build_802.3
 *
 *  Function builds an 802.3 header.
 */
libnet_ptag_t           /* packet id on success, -1 on failure */
libnet_build_802_3(
    u_char *,           /* pointer to a 6 byte ethernet address */
    u_char *,           /* pointer to a 6 byte ethernet address */
    u_short,            /* length */
    u_char *,           /* payload (or NULL) */
    u_long,             /* payload length */
    libnet_t *,         /* libnet context pointer */
    libnet_ptag_t       /* packet id */
    );


/*
 *  libnet_build_ethernet
 *
 *  Function builds an Ethernet header.
 */
libnet_ptag_t           /* packet id on success, -1 on failure */
libnet_build_ethernet(
    u_char *,           /* pointer to a 6 byte ethernet address */
    u_char *,           /* pointer to a 6 byte ethernet address */
    u_short,            /* type */
    u_char *,           /* payload (or NULL) */
    u_long,             /* payload length */
    libnet_t *,         /* libnet context pointer */
    libnet_ptag_t       /* packet id */
    );


/*
 *  libnet_autobuild_ethernet
 *
 *  Function builds an Ethernet header, automating the process.
 */
libnet_ptag_t           /* packet id on success, -1 on failure */
libnet_autobuild_ethernet(
    u_char *,           /* pointer to a 6 byte ethernet address */
    u_short,            /* packet type */
    libnet_t *          /* libnet context pointer */
    );


/*
 *  libnet_build_arp
 *
 *  Function builds an ARP header.
 */
libnet_ptag_t           /* packet id on success, -1 on failure */
libnet_build_arp(
    u_short,            /* hardware address type */
    u_short,            /* protocol address type */
    u_char,             /* hardware address length */
    u_char,             /* protocol address length */
    u_short,            /* ARP operation type */
    u_char *,           /* sender hardware address */
    u_char *,           /* sender protocol address */
    u_char *,           /* target hardware address */
    u_char *,           /* target protocol address */
    u_char *,           /* payload or NULL if none */
    u_long,             /* payload length */
    libnet_t *,         /* libnet context pointer */
    libnet_ptag_t       /* packet id */
    );


/*
 *  libnet_build_tcp
 *
 *  Function builds a TCP header.
 */
libnet_ptag_t           /* packet id on success, -1 on failure */
libnet_build_tcp(
    u_short,            /* Source port */
    u_short,            /* Destination port */
    u_long,             /* Sequence Number */
    u_long,             /* Acknowledgement Number */
    u_char,             /* Control bits */
    u_short,            /* Advertised Window Size */
    u_short,            /* Checksum */
    u_short,            /* Urgent Pointer */
    u_short,            /* length of payload if a protocol header - not data */
    u_char *,           /* Pointer to packet data (or NULL) */
    u_long,             /* payload length */
    libnet_t *,         /* libnet context pointer */
    libnet_ptag_t       /* packet id */
    );


/*
 *  libnet_build_tcp_options
 *
 *  Function builds TCP options list.
 */
libnet_ptag_t           /* packet id on success, -1 on failure */
libnet_build_tcp_options(
    u_char *,           /* options list */
    u_long,             /* options list size */
    libnet_t *,         /* libnet context pointer */
    libnet_ptag_t       /* packet id */
    );


/*
 *  libnet_build_udp
 *
 *  Function builds a UDP header.
 */
libnet_ptag_t           /* packet id on success, -1 on failure */
libnet_build_udp(
    u_short,            /* source port */
    u_short,            /* destination port */
    u_short,            /* total length (header + data) */
    u_short,            /* checksum */
    u_char *,           /* pointer to packet data (or NULL) */
    u_long,             /* payload length */
    libnet_t *,         /* libnet context pointer */
    libnet_ptag_t       /* packet id */
    );


/*
 *  libnet_build_cdp
 *
 *  Function builds a CDP header.
 */
libnet_ptag_t           /* packet id on success, -1 on failure */
libnet_build_cdp(
    u_char,             /* version */
    u_char,             /* ttl */
    u_short,            /* sum */
    u_short,            /* type */
    u_short,            /* len */
    u_char *,           /* value */
    u_char *,           /* pointer to packet data (or NULL) */
    u_long,             /* payload length */
    libnet_t *,         /* libnet context pointer */
    libnet_ptag_t       /* packet id */
    );


/*
 *  libnet_build_icmp4_echo
 *
 *  Function builds an ICMP echo header.
 */
libnet_ptag_t           /* packet id on success, -1 on failure */
libnet_build_icmpv4_echo(
    u_char,             /* type */
    u_char,             /* code */
    u_short,            /* checksum */
    u_short,            /* id */
    u_short,            /* sequence number */
    u_char *,           /* pointer to packet data (or NULL) */
    u_long,             /* payload length */
    libnet_t *,         /* libnet context pointer */
    libnet_ptag_t       /* packet id */
    );


/*
 *  libnet_build_icmp4_mask
 *
 *  Function builds an ICMP netmask header.
 */
libnet_ptag_t           /* packet id on success, -1 on failure */
libnet_build_icmpv4_mask(
    u_char,             /* type */
    u_char,             /* code */
    u_short,            /* checksum */
    u_short,            /* id */
    u_short,            /* sequence number */
    u_long,             /* address mask */
    u_char *,           /* pointer to packet data (or NULL) */
    u_long,             /* payload length */
    libnet_t *,         /* libnet context pointer */
    libnet_ptag_t       /* packet id */
    );


/*
 *  libnet_build_icmp4_unreach
 *
 *  Function builds an ICMP unreachable header (with corresponding IP header).
 */
libnet_ptag_t           /* packet id on success, -1 on failure */
libnet_build_icmpv4_unreach(
    u_char,             /* type */
    u_char,             /* code */
    u_short,            /* checksum */
    u_short,            /* original Length of packet data */
    u_char,             /* original IP tos */
    u_short,            /* original IP ID */
    u_short,            /* original Fragmentation flags and offset */
    u_char,             /* original TTL */
    u_char,             /* original Protocol */
    u_short,            /* original checksum */
    u_long,             /* original Source IP Address */
    u_long,             /* original Destination IP Address */
    u_char *,           /* pointer to packet data (or NULL) */
    u_long,             /* payload length */
    libnet_t *,         /* libnet context pointer */
    libnet_ptag_t       /* packet id */
    );


/*
 *  libnet_build_icmp4_redirect
 *
 *  Function builds an ICMP redirect header (with corresponding IP header).
 */
libnet_ptag_t           /* packet id on success, -1 on failure */
libnet_build_icmpv4_redirect(
    u_char,             /* type */
    u_char,             /* code */
    u_short,            /* checksum */
    u_long,             /* gateway host that should be used */
    u_short,            /* original length of packet data */
    u_char,             /* original IP tos */
    u_short,            /* original IP ID */
    u_short,            /* original fragmentation flags and offset */
    u_char,             /* original TTL */
    u_char,             /* original protocol */
    u_short,            /* original checksum */
    u_long,             /* original source IP address */
    u_long,             /* original destination IP address */
    u_char *,           /* pointer to packet data (or NULL) */
    u_long,             /* payload length */
    libnet_t *,         /* libnet context pointer */
    libnet_ptag_t       /* packet id */
    );


/*
 *  libnet_build_icmp4_timeexceed
 *
 *  Function builds an ICMP time exceeded header.
 */
libnet_ptag_t           /* packet id on success, -1 on failure */
libnet_build_icmpv4_timeexceed(
    u_char,             /* type */
    u_char,             /* code */
    u_short,            /* checksum */
    u_short,            /* original Length of packet data */
    u_char,             /* original IP tos */
    u_short,            /* original IP ID */
    u_short,            /* original Fragmentation flags and offset */
    u_char,             /* original TTL */
    u_char,             /* original Protocol */
    u_short,            /* original checksum */
    u_long,             /* original Source IP Address */
    u_long,             /* original Destination IP Address */
    u_char *,           /* pointer to packet data (or NULL) */
    u_long,             /* payload length */
    libnet_t *,         /* libnet context pointer */
    libnet_ptag_t       /* packet id */
    );


/*
 *  libnet_build_icmp4_timestamp
 *
 *  Function builds an ICMP timestamp header.
 */
libnet_ptag_t           /* packet id on success, -1 on failure */
libnet_build_icmpv4_timestamp(
    u_char,             /* type */
    u_char,             /* code */
    u_short,            /* checksum */
    u_short,            /* id */
    u_short,            /* sequence number */
    n_time,             /* original timestamp */
    n_time,             /* receive timestamp */
    n_time,             /* transmit timestamp */
    u_char *,           /* pointer to packet data (or NULL) */
    u_long,             /* payload length */
    libnet_t *,         /* libnet context pointer */
    libnet_ptag_t       /* packet id */
    );


/*
 *  libnet_build_igmp
 *
 *  Function builds an IGMP header.
 */
libnet_ptag_t           /* packet id on success, -1 on failure */
libnet_build_igmp(
    u_char,             /* type */
    u_char,             /* code */
    u_short,            /* checksum */
    u_long,             /* ip address */
    u_char *,           /* pointer to packet data (or NULL) */
    u_long,             /* payload length */
    libnet_t *,         /* libnet context pointer */
    libnet_ptag_t       /* packet id */
    );


/*
 *  libnet_build_ipv4
 *
 *  Function builds an IPv4 header.
 */
libnet_ptag_t           /* packet id on success, -1 on failure */
libnet_build_ipv4(
    u_short,            /* entire packet length */
    u_char,             /* tos */
    u_short,            /* ID */
    u_short,            /* fragmentation flags and offset */
    u_char,             /* TTL */
    u_char,             /* protocol */
    u_short,            /* checksum */
    u_long,             /* source address */
    u_long,             /* destination address */
    u_char *,           /* pointer to packet data (or NULL) */
    u_long,             /* payload length */
    libnet_t *,         /* libnet context pointer */
    libnet_ptag_t       /* packet id */
    );


/*
 *  libnet_build_ipv4_options
 *
 *  Function builds IPv4 options list.
 */
libnet_ptag_t           /* packet id on success, -1 on failure */
libnet_build_ipv4_options(
    u_char *,           /* options list */
    u_long,             /* options list size */
    libnet_t *,         /* libnet context pointer */
    libnet_ptag_t       /* packet id */
    );


/*
 *  libnet_autobuild_ipv4
 *
 *  Function builds an IPv4 header, automating much of the process.
 */
libnet_ptag_t           /* packet id on success, -1 on failure */
libnet_autobuild_ipv4(
    u_short,            /* entire packet length */
    u_char,             /* protocol */
    u_long,             /* dst */
    libnet_t *          /* libnet context pointer */
    );



 /*
 *  libnet_build_ipv6
 *  
 *  Function builds an IPv6 header.
 */ 
libnet_ptag_t
libnet_build_ipv6(
    u_char,             /* traffic control */
    u_long,             /* flow label */
    u_short,            /* length */
    u_char,             /* next header */
    u_char,             /* hop limit */
    struct libnet_in6_addr,/* src */
    struct libnet_in6_addr,/* dst */
    u_char *,           /* pointer to packet data (or NULL) */
    u_long,             /* payload length */
    libnet_t *,         /* libnet context pointer */
    libnet_ptag_t       /* packet id */
    );


/*  
 *  libnet_autobuild_ipv6
 *  
 *  Function builds an IPv6 header, automating much of the process.
 */
libnet_ptag_t
libnet_autobuild_ipv6(
    u_short,            /* length */
    u_char,             /* next header */
    struct libnet_in6_addr,/* dst */
    libnet_t *          /* libnet context pointer */
    );


/*
 *  libnet_build_isl
 *
 *  Function builds a Cisco ISL header.
 */
libnet_ptag_t
libnet_build_isl(
    u_char *,
    u_char,
    u_char,
    u_char *,
    u_short,
    u_char *,
    u_short,
    u_short,
    u_short,
    u_char *,
    u_long,
    libnet_t *,
    libnet_ptag_t
    );

/*
 *  libnet_build_ipsec_esp_hdr
 *
 *  Function builds an IPsec ESP header.
 */
libnet_ptag_t           /* packet id on success, -1 on failure */
libnet_build_ipsec_esp_hdr(
    u_long,             /* security parameter index */
    u_long,             /* ESP sequence number */
    u_long,             /* initialization vector */
    u_char *,           /* pointer to packet data (or NULL) */
    u_long,             /* payload length */
    libnet_t *,         /* libnet context pointer */
    libnet_ptag_t       /* packet id */
    );


/*
 *  libnet_build_ipsec_esp_ftr
 *
 *  Function builds an IPsec ESP footer.
 */
libnet_ptag_t           /* packet id on success, -1 on failure */
libnet_build_ipsec_esp_ftr(
    u_char,             /* padding length */
    u_char,             /* next header pointer */
    char *,             /* authentication data */
    u_char *,           /* pointer to packet data (or NULL) */
    u_long,             /* payload length */
    libnet_t *,         /* libnet context pointer */
    libnet_ptag_t       /* packet id */
    );


/*
 *  libnet_build_ipsec_ah
 *
 *  Function builds an IPsec AH header.
 */
libnet_ptag_t           /* packet id on success, -1 on failure */
libnet_build_ipsec_ah(
    u_char,             /* next header */
    u_char,             /* payload length */
    u_short,            /* reserved */
    u_long,             /* security parameter index  */
    u_long,             /* AH sequence number */
    u_long,             /* authentication data */
    u_char *,           /* pointer to packet data (or NULL) */
    u_long,             /* payload length */
    libnet_t *,         /* libnet context pointer */
    libnet_ptag_t       /* packet id */
    );


/*
 *  libnet_build_dns
 *
 *  Function builds a DNSv4 header.
 */
libnet_ptag_t           /* packet id on success, -1 on failure */
libnet_build_dnsv4(
    u_short,            /* ID */
    u_short,            /* flags */
    u_short,            /* number of questions */
    u_short,            /* number of answer resource records */
    u_short,            /* number of authority resource records */
    u_short,            /* number of additional resource records */
    u_char *,           /* pointer to packet data (or NULL) */
    u_long,             /* payload length */
    libnet_t *,         /* libnet context pointer */
    libnet_ptag_t       /* packet id */
    );


/*
 *  libnet_build_rip
 *
 *  Function builds a RIP header.
 */
libnet_ptag_t           /* packet id on success, -1 on failure */
libnet_build_rip(
    u_char,             /* command */
    u_char,             /* version */
    u_short,            /* zero (v1) or routing domain (v2) */
    u_short,            /* address family */
    u_short,            /* zero (v1) or route tag (v2) */
    u_long,             /* IP address */
    u_long,             /* zero (v1) or subnet mask (v2) */
    u_long,             /* zero (v1) or next hop IP address (v2) */
    u_long,             /* metric */
    u_char *,           /* pointer to packet data (or NULL) */
    u_long,             /* payload length */
    libnet_t *,         /* libnet context pointer */
    libnet_ptag_t       /* packet id */
    );


/*
 *  libnet_build_stp_conf
 *
 *  Function builds an STP conf header.
 */
libnet_ptag_t           /* packet id on success, -1 on failure */
libnet_build_stp_conf(
    u_short,            /* protocol id */
    u_char,             /* protocol version */
    u_char,             /* bridge protocol data unit type */
    u_char,             /* control flags */
    u_char *,           /* root id */
    u_long,             /* root path cost */
    u_char *,           /* bridge id */
    u_short,            /* port id */
    u_short,            /* message age */
    u_short,            /* max age */
    u_short,            /* hello time */
    u_short,            /* forward delay */
    u_char *,           /* pointer to packet data (or NULL) */
    u_long,             /* payload length */
    libnet_t *,         /* libnet context pointer */
    libnet_ptag_t       /* packet id */
    );


/*
 *  libnet_build_stp_tcn
 *
 *  Function builds an STP tcn header.
 */
libnet_ptag_t           /* packet id on success, -1 on failure */
libnet_build_stp_tcn(
    u_short,            /* protocol id */
    u_char,             /* protocol version */
    u_char,             /* bridge protocol data unit type */
    u_char *,           /* pointer to packet data (or NULL) */
    u_long,             /* payload length */
    libnet_t *,         /* libnet context pointer */
    libnet_ptag_t       /* packet id */
    );


/*
 *  libnet_build_vrrp
 *
 *  Function builds a VRRP header.
 */
libnet_ptag_t           /* packet id on success, -1 on failure */
libnet_build_vrrp(
    u_char,             /* version */
    u_char,             /* packet type */
    u_char,             /* virtual router id */
    u_char,             /* priority */
    u_char,             /* number of IP addresses */
    u_char,             /* auth type */
    u_char,             /* advertisement interval */
    u_short,            /* checksum */
    u_char *,           /* pointer to packet data (or NULL) */
    u_long,             /* payload length */
    libnet_t *,         /* libnet context pointer */
    libnet_ptag_t       /* packet id */
    );


/*
 *  libnet_build_ntp
 *
 *  Function builds an NTP header.
 */
libnet_ptag_t           /* packet id on success, -1 on failure */
libnet_build_ntp(
    u_char,             /* leap indicator */
    u_char,             /* version */
    u_char,             /* mode */
    u_char,             /* stratum */
    u_char,             /* polling interval */
    u_char,             /* precision */
    u_short,            /* root delay integer */
    u_short,            /* root delay fraction */
    u_short,            /* root dispersion integer */
    u_short,            /* root dispersion fraction */
    u_long,             /* reference ID */
    u_long,             /* reference timestamp integer */
    u_long,             /* reference timestamp fraction */
    u_long,             /* originate timestamp integer */
    u_long,             /* originate timestamp fraction */ 
    u_long,             /* receive timestamp integer */ 
    u_long,             /* receive timestamp fraction */
    u_long,             /* transmit timestamp integer */
    u_long,             /* transmit timestamp fraction */
    u_char *,           /* pointer to packet data (or NULL) */
    u_long,             /* payload length */
    libnet_t *,         /* libnet context pointer */
    libnet_ptag_t       /* packet id */
    );


/*
 *  libnet_build_ospfv2
 *
 *  Function builds a V2 OSPF header.
 */
libnet_ptag_t           /* packet id on success, -1 on failure */
libnet_build_ospfv2(
    u_short,            /* length of entire OSPF packet */
    u_char,             /* type */
    u_long,             /* router id */
    u_long,             /* area id */
    u_short,            /* checksum */
    u_short,            /* auth type */
    u_char *,           /* pointer to packet data (or NULL) */
    u_long,             /* payload length */
    libnet_t *,         /* libnet context pointer */
    libnet_ptag_t       /* packet id */
    );
 
 
/*
 *  libnet_build_ospf_hellov2
 *
 *  Function builds an OSPF hello header.
 */
libnet_ptag_t           /* packet id on success, -1 on failure */
libnet_build_ospfv2_hello(
    u_long,             /* netmask */
    u_short,            /* interval (seconds since last packet sent) */
    u_char,             /* options */
    u_char,             /* priority */
    u_int,              /* time til router is deemed down */
    u_long,             /* network's designated router */
    u_long,             /* network's backup router */
    u_long,             /* neighbor */
    u_char *,           /* pointer to packet data (or NULL) */
    u_long,             /* payload length */
    libnet_t *,         /* libnet context pointer */
    libnet_ptag_t       /* packet id */
    );
 
 
/*
 *  libnet_build_ospfv2_dbd
 *
 *  Function builds an OSPF dbd header.
 */
libnet_ptag_t           /* packet id on success, -1 on failure */
libnet_build_ospfv2_dbd(
    u_short,            /* MTU of interface */
    u_char,             /* options */
    u_char,             /* type of exchange */
    u_int,              /* DBD sequence number */
    u_char *,           /* pointer to packet data (or NULL) */
    u_long,             /* payload length */
    libnet_t *,         /* libnet context pointer */
    libnet_ptag_t       /* packet id */
    );
 
 
/*
 *  libnet_build_ospfv2_lsr
 *
 *  Function builds an OSPF lsr header.
 */
libnet_ptag_t           /* packet id on success, -1 on failure */
libnet_build_ospfv2_lsr(
    u_int,              /* type link state being requested */
    u_int,              /* link state ID */
    u_long,             /* advertising router */
    u_char *,           /* pointer to packet data (or NULL) */
    u_long,             /* payload length */
    libnet_t *,         /* libnet context pointer */
    libnet_ptag_t       /* packet id */
    );
 
 
/*
 *  libnet_build_ospfv2_lsu
 *
 *  Function builds an OSPF lsu header.
 */
libnet_ptag_t           /* packet id on success, -1 on failure */
libnet_build_ospfv2_lsu(
    u_int,              /* number of LSU's that will be broadcasted */
    u_char *,           /* pointer to packet data (or NULL) */
    u_long,             /* payload length */
    libnet_t *,         /* libnet context pointer */
    libnet_ptag_t       /* packet id */
    );


/*
 *  libnet_build_ospfv2_lsa
 *
 *  Function builds an OSPF lsa header.
 */
libnet_ptag_t           /* packet id on success, -1 on failure */
libnet_build_ospfv2_lsa(
    u_short,            /* age */
    u_char,             /* options */
    u_char,             /* type */
    u_int,              /* link state id */
    u_long,             /* advertisting router */
    u_int,              /* sequence number */
    u_short,            /* checksum */
    u_short,            /* packet length */
    u_char *,           /* pointer to packet data (or NULL) */
    u_long,             /* payload length */
    libnet_t *,         /* libnet context pointer */
    libnet_ptag_t       /* packet id */
    );
 
 
/*
 *  libnet_build_ospfv2_lsa_rtr
 *
 *  Function builds an OSPF lsa rtr header.
 */
libnet_ptag_t           /* packet id on success, -1 on failure */
libnet_build_ospfv2_lsa_rtr(
    u_short,            /* flags */
    u_short,            /* number */
    u_int,              /* id */
    u_int,              /* data */
    u_char,             /* type */
    u_char,             /* type of service */
    u_short,            /* metric */
    u_char *,           /* pointer to packet data (or NULL) */
    u_long,             /* payload length */
    libnet_t *,         /* libnet context pointer */
    libnet_ptag_t       /* packet id */
    );
 
 
/*
 *  libnet_build_ospfv2_lsa_net
 *
 *  Function builds an OSPF lsa net header.
 */
libnet_ptag_t           /* packet id on success, -1 on failure */
libnet_build_ospfv2_lsa_net(
    u_long,             /* netmask */
    u_int,              /* router id */
    u_char *,           /* pointer to packet data (or NULL) */
    u_long,             /* payload length */
    libnet_t *,         /* libnet context pointer */
    libnet_ptag_t       /* packet id */
    );
 
 
/*
 *  libnet_build_ospfv2_lsa_sum
 *
 *  Function builds an OSPF lsa sum header.
 */
libnet_ptag_t           /* packet id on success, -1 on failure */
libnet_build_ospfv2_lsa_sum(
    u_long,             /* netmask */
    u_int,              /* metric */
    u_int,              /* type of service */
    u_char *,           /* pointer to packet data (or NULL) */
    u_long,             /* payload length */
    libnet_t *,         /* libnet context pointer */
    libnet_ptag_t       /* packet id */
    );
 
 
/*
 *  libnet_build_ospfv2_lsa_as
 *
 *  Function builds an OSPF lsa as header.
 */
libnet_ptag_t           /* packet id on success, -1 on failure */
libnet_build_ospfv2_lsa_as(
    u_long,             /* netmask */
    u_int,              /* metric */
    u_long,             /* forward address */
    u_int,              /* tag */
    u_char *,           /* pointer to packet data (or NULL) */
    u_long,             /* payload length */
    libnet_t *,         /* libnet context pointer */
    libnet_ptag_t       /* packet id */
    );


/*
 *  libnet_build_data
 *
 *  Function builds a generic data unit.
 */
libnet_ptag_t           /* packet id on success, -1 on failure */
libnet_build_data(
    u_char *,           /* pointer to packet data (or NULL) */
    u_long,             /* payload length */
    libnet_t *,         /* libnet context pointer */
    libnet_ptag_t       /* packet id */
    );


/*
 *  libnet_build_dhcpv4
 *
 *  Function builds a DHCPv4 header.
 */
libnet_ptag_t           /* packet id on success, -1 on failure */
libnet_build_dhcpv4(
    u_char,             /* opcode */
    u_char,             /* hardware address type */
    u_char,             /* hardware address length */
    u_char,             /* hopcount */
    u_long,             /* transaction ID */
    u_short,            /* number of seconds since trying to bootstrap */
    u_short,            /* unused */
    u_long,             /* client's IP */
    u_long,             /* your IP */
    u_long,             /* server's IP */
    u_long,             /* gateway IP */
    u_char *,           /* client hardware address */
    u_char *,           /* server host name */
    u_char *,           /* boot file name */
    u_char *,           /* pointer to packet data (or NULL) */
    u_long,             /* payload length */
    libnet_t *,         /* libnet context pointer */
    libnet_ptag_t       /* packet id */
    );


/*
 *  libnet_build_bootpv4
 *
 *  Function builds a BOOTPv4 header.
 */
libnet_ptag_t           /* packet id on success, -1 on failure */
libnet_build_bootpv4(
    u_char,             /* opcode */
    u_char,             /* hardware address type */
    u_char,             /* hardware address length */
    u_char,             /* hopcount */
    u_long,             /* transaction ID */
    u_short,            /* number of seconds since trying to bootstrap */
    u_short,            /* flags */
    u_long,             /* client's IP */
    u_long,             /* your IP */
    u_long,             /* server's IP */
    u_long,             /* gateway IP */
    u_char *,           /* client hardware address */
    u_char *,           /* server host name */
    u_char *,           /* boot file name */
    u_char *,           /* pointer to packet data (or NULL) */
    u_long,             /* payload length */
    libnet_t *,         /* libnet context pointer */
    libnet_ptag_t       /* packet id */
    );


/*
 *  libnet_write
 *
 *  Function writes a packet to the wire.  Libnet will know which injection
 *  method to use, as well as if the user wants checksums to be calculated.
 *  Planned functionality for multiple packets will allow for tunable sleep
 *  times between packet writes.
 */
int                     /* number of bytes written if successful, -1 on error */
libnet_write(
    libnet_t *          /* libnet context pointer */
    );


/*
 *  libnet_write_ipv4
 *
 *  Function writes an ipv4 packet through the raw socket interface.  Used
 *  internally by libnet.
 */
int                     /* number of bytes written if successful, -1 on error */
libnet_write_raw_ipv4(
    libnet_t *,         /* libnet context pointer */
    u_char *,           /* pointer to the assembled packet */
    u_long              /* size of the assembled packet */
    );


/*
 *  libnet_write_ipv6
 *
 *  Function writes an ipv6 packet through the raw socket interface.  Used
 *  internally by libnet.
 */
int                     /* number of bytes written if successful, -1 on error */
libnet_write_raw_ipv6(
    libnet_t *,         /* libnet context pointer */
    u_char *,           /* pointer to the assembled packet */
    u_long              /* size of the assembled packet */
    );


/*
 *  libnet_write_link
 *
 *  Function writes a packet through the link-layer interface.  Used
 *  internally by libnet.
 */
int                     /* number of bytes written if successful, -1 on error */
libnet_write_link(
    libnet_t *,         /* libnet context pointer */
    u_char *,           /* pointer to the assembled packet */
    u_long              /* size of the assembled packet */
    );


/*
 *  libnet_open_raw4
 *
 *  Function opens a IPv4 raw socket and sets IP_HDRINCL socket option.
 *  Under linux it also sets SO_BROADCAST in order to write broadcast packets.
 */
int                     /* opened file desciptor, or -1 on error */
libnet_open_raw4(
    libnet_t *          /* libnet context pointer */
    );


/*
 *  libnet_close_raw4
 *
 *  Function closes the IPv4 raw socket.
 */
int                     /* 1 upon success, or -1 on error */
libnet_close_raw4(
    libnet_t *          /* libnet context pointer */
    );


/*  
 *  libnet_open_raw6
 * 
 *  Function opens a IPv6 raw socket and sets IP_HDRINCL socket option.
 *  Under linux it also sets SO_BROADCAST in order to write broadcast packets.
 */
int                     /* opened file desciptor, or -1 on error */
libnet_open_raw6(
    libnet_t *          /* libnet context pointer */
    );   
       
       
/*
 *  libnet_close_raw6
 *
 *  Function closes the IPv6 raw socket.
 */
int                     /* 1 upon success, or -1 on error */
libnet_close_raw6(
    libnet_t *          /* libnet context pointer */
    );


/*
 *  libnet_select_device
 *
 *  Function finds a device for use with libnet's link-layer interface.
 */
int
libnet_select_device(
    libnet_t *          /* libnet context pointer */
    );


/*
 *  libnet_open_link
 *
 *  Function opens a link-layer interface for eventual packet injection.  Used
 *  internally by libnet.
 */
int
libnet_open_link(
    libnet_t *          /* libnet context pointer */
    );


/*
 *  libnet_close_link
 *
 *  Function closes a link interface.  Used internally by libnet.
 */
int                     /* 1 on success, -1 on failure */
libnet_close_link(
    libnet_t *          /* libnet context pointer */
    );


/*
 *  libnet_get_ipaddr4
 *
 *  Function returns host-byte order IPv4 address.
 */
u_long                  /* 0 upon error, address upon success */
libnet_get_ipaddr4(
    libnet_t *          /* libnet context pointer */
    );


/*
 *  libnet_get_ipaddr6
 *
 *  Function returns host-byte order IPv6 address.
 */
struct libnet_in6_addr
libnet_get_ipaddr6(
    libnet_t *          
    );


/*
 *  libnet_get_hwaddr
 *
 *  Function returns a 6-byte ethernet address of the interface libnet is
 *  currently using.
 */
struct libnet_ether_addr * /* 0 upon error, address upon success */
libnet_get_hwaddr(
    libnet_t *          /* libnet context pointer */
    );


/*
 *  libnet_do_checksum
 *
 *  Function calculates the one's compliment checksum for a given protocol
 *  over the given packet buffer.
 */
int                     /* 1 on success, -1 on failure */
libnet_do_checksum(
    libnet_t *,         /* libnet context pointer */
    u_char *,           /* pointer to the packet buffer */
    int,                /* protocol */
    int                 /* packet size */
    );


/*
 *  libnet_compute_crc
 *
 *  Function computes the 32-bit CRC as per RFC 2083 over the given packet 
 *  buffer.
 */
u_long                  /* 32-bit CRC */
libnet_compute_crc(
    u_char *,           /* pointer to the packet buffer */
    u_long              /* packet size */
    );


/*
 *  libnet_ip_check
 *
 *  Function is a quick down and dirty IP checksum wrapper.
 */
u_short                 /* standard IP checksum */
libnet_ip_check(
    u_short *,          /* pointer to the buffer to be summed */
    int                 /* packet length */
    );


/*
 *  libnet_in_cksum
 *
 *  Function is the standard workhorse IP checksum routine.
 */
int                     /* standard IP checksum */
libnet_in_cksum(
    u_short *,          /* pointer to the buffer to be summed */
    int                 /* packet length */
    );


/*
 *  libnet_pblock_probe
 *
 *  If ptag is 0, function will create a pblock for the protocol unit type,
 *  append it to the list and return a pointer to it.  If ptag is not 0,
 *  function will search the pblock list for the specified protocol block 
 *  and return a pointer to it.
 */
libnet_pblock_t *       /* the pblock or NULL on error */
libnet_pblock_probe(
    libnet_t *,         /* libnet context pointer */
    libnet_ptag_t,      /* ptag to look for, or 0 to create a new one */
    u_long,             /* size of protocol unit to create (or resize to) */
    u_char              /* type of protocol unit */
    );


/*
 *  libnet_pblock_new
 *
 *  Function creates the pblock list if l->protocol_blocks == NULL or appends
 *  an entry to the doubly linked list.
 */
libnet_pblock_t *       /* the pblock or NULL on error */
libnet_pblock_new(
    libnet_t *,         /* libnet context pointer */
    u_long              /* size of object (amount of memory to malloc) */
    );


/*
 *  libnet_pblock_swap
 *
 *  Function swaps two pblocks in memory.
 */
int
libnet_pblock_swap(
    libnet_t *,         /* libnet context pointer */
    libnet_ptag_t,      /* ptag 1 */
    libnet_ptag_t       /* ptag 2 */
    );
  

/*
 *  libnet_pblock_insert_before
 *
 *  Function inserts a pblock into the doubly linked list.
 */
int
libnet_pblock_insert_before(
    libnet_t *,         /* libnet context pointer */
    libnet_ptag_t,      /* ptag 1 */
    libnet_ptag_t       /* ptag 2 */
    );

/*
 *  libnet_pblock_free
 *  
 *  Function frees the memory associated with p.
 */
void
libnet_pblock_free(
    libnet_pblock_t *    /* the pblock to destroy */
    );


/*
 *  libnet_pblock_update
 *
 *  Function updates the pblock meta-inforation.  Internally it updates the
 *  ptag with a monotonically increasing variable kept in l.  This way each
 *  pblock has a succesively increasing ptag identifier.
 */
libnet_ptag_t           /* the pblock's updated ptag */
libnet_pblock_update(
    libnet_t *,         /* libnet context pointer */
    libnet_pblock_t *,  /* pointer of the pblock to update */
    u_short,            /* header length */
    u_char              /* type of pblock */
    );    


/*
 *  libnet_pblock_find
 *
 *  Function locates a given block by it's ptag. 
 */
libnet_pblock_t *       /* the pblock or NULL on error */
libnet_pblock_find(
    libnet_t *,         /* libnet context pointer */
    libnet_ptag_t       /* ptag to locate */
    );


/*
 *  libnet_pblock_append
 *
 *  Function copies protocol block data over.
 */
int                     /* 1 on success, -1 on failure */
libnet_pblock_append(
    libnet_t *,         /* libnet context pointer */
    libnet_pblock_t *,  /* pointer of the pblock to copy data to */
    u_char *,           /* data to copy */
    u_long              /* size of data to copy */
    );


/*
 *  libnet_pblock_setflags
 *
 *  Function sets pblock flags.
 */
void
libnet_pblock_setflags(
    libnet_pblock_t *,  /* pointer of the pblock to set flags on */
    u_char              /* flags byte */
    );


/*
 *  libnet_pblock_p2p
 *
 *  Function returns the protocol number for the protocol block type.  If
 *  the type is unknown, the function defaults to returning IPPROTO_IP.
 */
int                     /* IP proto number */
libnet_pblock_p2p(
    u_char              /* pblock type */
    );


/*
 *  libnet_pblock_coalesce
 *
 *  Function assembles the packet for subsequent writing.  Function makes two
 *  passes through the pblock list:
 *  1st & 2nd) determine total size of the packet for contiguous malloc
 *             and copy over packet chunks 
 *  3rd run) run through the original list and see which protocol blocks had
 *           the checksum flag set (checksums usually need to be done over
 *           an assembled packet so it's easier to do it here)
 */
int                     /* 1 on success, -1 on failure */
libnet_pblock_coalesce(
    libnet_t *,         /* libnet context pointer */
    u_char **,          /* resulting packet will be here */
    u_long *            /* size of packet will be here */
    );


/*
 *  __libnet_context_dump
 *
 *  Function returns the contents of the libnet file context.  Not meant for
 *  the applications programer.
 */
void
__libnet_context_dump(
    libnet_t *          /* libnet context pointer */
    );


/*
 *  __libnet_hex_dump
 *
 *  Function dumps the contents of the supplied buffer to the supplied
 *  stream pointer.  Very useful for debugging.  Will swap endianness based
 *  disposition of mode variable.  Use requires unwrapping the libnet file
 *  context structure so it's hidden down here.  If you find it, consider
 *  yourself a trepid adventurer.
 */
void
__libnet_hex_dump(
    u_char *,           /* buffer to dump */
    u_long,             /* length of buffer */
    int,                /* mode; to swap (1) or not to swap (0) */
    FILE *              /* stream pointer to dump to */
    );


/*
 *  libnet_hex_aton
 *
 *  hexidecimal strings of the format "##:##:##:## ... :##:##" to a uchar.
 *
 */
u_char *
libnet_hex_aton(
    char *,
    int *
    );

/*
 *  libnet_adv_cull_packet
 *
 *  advanced interface, culls the packet from inside libnet, wraps
 *  libnet_pblock_coalesce().
 *
 */
int
libnet_adv_cull_packet(
    libnet_t *,         /* libnet context pointer */
    u_char **,          /* resulting packet will be here */
    u_long *            /* size of packet will be here */
    );

/*
 *  libnet_adv_write_link
 *
 *  advanced interface, writes a prebuilt frame to the wire
 *
 */
int
libnet_adv_write_link(
    libnet_t *,         /* libnet context pointer */
    u_char *,           /* packet goes here */
    u_long              /* size of packet goes here */
    );

#endif  /* __LIBNET_FUNCTIONS_H */

/* EOF */
