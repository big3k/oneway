# $Id: Makefile.am,v 1.1.1.1 2002/08/05 22:26:04 route Exp $
#
# Libnet automake information file
# Copyright (c) 1998 - 2002 Mike D. Schiffman <mike@infonexus.com>
# All rights reserved.
#
# Process this file with automake to produce a Makefile.in script.

include $(top_srcdir)/Makefile.am.common

noinst_PROGRAMS = arp cdp dhcp_discover get_addr icmp_timestamp icmp_unreach \
                  ieee isl ntp ospf_hello ospf_lsa ping_of_death stp synflood \
                  tcp1 tcp2 udp1 udp2 synflood6 icmp6_echoreq

arp_SOURCES             = arp.c
cdp_SOURCES             = cdp.c
dhcp_discover_SOURCES   = dhcp_discover.c
get_addr_SOURCES        = get_addr.c
icmp_timestamp_SOURCES  = icmp_timestamp.c
icmp_unreach_SOURCES    = icmp_unreach.c
ieee_SOURCES            = ieee.c
isl_SOURCES             = isl.c
ntp_SOURCES             = ntp.c
ospf_hello_SOURCES      = ospf_hello.c
ospf_lsa_SOURCES        = ospf_lsa.c
ping_of_death_SOURCES   = ping_of_death.c
stp_SOURCES             = stp.c
synflood_SOURCES        = synflood.c
tcp1_SOURCES            = tcp1.c
tcp2_SOURCES            = tcp2.c
udp1_SOURCES            = udp1.c
udp2_SOURCES            = udp2.c
icmp6_echoreq_SOURCES   = icmp6_echoreq.c
synflood6_SOURCES       = synflood6.c

LDADD = $(top_srcdir)/src/libnet.a
