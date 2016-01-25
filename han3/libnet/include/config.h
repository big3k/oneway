/* include/config.h.  Generated automatically by configure.  */
/* include/config.h.in.  Generated automatically from configure.in by autoheader.  */

/* #undef LIBNET_BSDISH_OS */
/* #undef LIBNET_BSD_BYTE_SWAP */
/* #undef DLPI_DEV_PREFIX */
/* #undef HAVE_DEV_DLPI */
/* #undef HAVE_SOLARIS */
/* #undef HAVE_HPUX11 */
/* #undef HAVE_SOCKADDR_SA_LEN */
/* #undef HAVE_DLPI */
#define HAVE_PACKET_SOCKET 1
/* #undef HAVE_STRUCT_IP_CSUM */
/* #undef HAVE_LIB_PCAP */
/* #undef LBL_ALIGN */
/* #undef STUPID_SOLARIS_CHECKSUM_BUG */
#define _BSD_SOURCE 1
#define __BSD_SOURCE 1
#define __FAVOR_BSD 1
/* #undef LIBNET_BIG_ENDIAN */
#define LIBNET_LIL_ENDIAN 1
/* #undef NO_SNPRINTF */

/* Define if you have the strerror function.  */
#define HAVE_STRERROR 1

/* Define if you have the <net/ethernet.h> header file.  */
#define HAVE_NET_ETHERNET_H 1

/* Define if you have the <sys/bufmod.h> header file.  */
/* #undef HAVE_SYS_BUFMOD_H */

/* Define if you have the <sys/dlpi_ext.h> header file.  */
/* #undef HAVE_SYS_DLPI_EXT_H */

/* Define if you have the <sys/sockio.h> header file.  */
/* #undef HAVE_SYS_SOCKIO_H */

/* Define if you have the nsl library (-lnsl).  */
/* #undef HAVE_LIBNSL */

/* Define if you have the packet library (-lpacket).  */
/* #undef HAVE_LIBPACKET */

/* Define if you have the socket library (-lsocket).  */
/* #undef HAVE_LIBSOCKET */

/* Define if you have the wpcap library (-lwpcap).  */
/* #undef HAVE_LIBWPCAP */

/* Name of package */
#define PACKAGE "libnet"

/* Version number of package */
#define VERSION "1.1.0"

