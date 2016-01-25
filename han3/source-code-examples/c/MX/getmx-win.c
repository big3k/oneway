/***********************************************/
/** test dns mx query with an implementation ***/ 
/** $Id: getmx.c,v 1.3 2004/07/18 15:57:48 oneway Exp oneway $***/
/***********************************************/
//#define WINDOWS

#ifdef WINDOWS
#include "StdAfx.h"
#else
#include <stdio.h>
#include <string.h> 
#include <unistd.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#endif

#define TIME_OUT 4     /* time out for DNS reply in seconds  */

/** function to resolve MX given domain and dns server  ***/

/** return mx ip as unsigned int **/ 
unsigned int getmx(char *domain, char *dns_server_ip); 

char * getnextRR(char *, char *);  /* start of whole msg & pointer to 1stRR */

unsigned int parseAR(char *, char *); /* start of while msg, & pointer to RR */
                                      /* to get ip back for mx AR*/

unsigned int parseAN(char *, char *); /* start of while msg, & pointer to RR */
                                      /* to get ip back for mx AN */
int readable_timeo(int fd, int sec);  /* time out read socket */ 

#ifdef WINDOWS
int initWinsockDll()
{
	WORD wVersionRequested;
	WSADATA wsaData;
	int err; 

	wVersionRequested = MAKEWORD( 2, 2 ); 
	err = WSAStartup( wVersionRequested, &wsaData );
	if ( err != 0 ) 
	{
		printf("WSAStartup failed\n");
		return ( err );
	} 

	/* Confirm that the WinSock DLL supports 2.2.        */
	/* Note that if the DLL supports versions greater    */
	/* than 2.2 in addition to 2.2, it will still return */
	/* 2.2 in wVersion since that is the version we      */
	/* requested.                                        */ 

	if ( LOBYTE( wsaData.wVersion ) != 2 || 
         HIBYTE( wsaData.wVersion ) != 2 ) 
	{
		printf( "LOBYTE = %ld\n", LOBYTE( wsaData.wVersion ));
		printf( "HIBYTE = %ld\n", HIBYTE( wsaData.wVersion ));

		/* Tell the user that we could not find a usable*/
		/* WinSock DLL.*/
		WSACleanup( );
		return ( -1 ); 
	}
	return ( 0 );
	/* The WinSock DLL is acceptable. Proceed. */ 
}
#endif

int main (int argc, char **argv) {
    char *domain; 
    char *backup_dns = "128.107.241.185";  /* ns1.cisco.com */ 
    char *dns_svr; 
    char host[128]; 
    struct in_addr in;
    unsigned int mx_ip; 

    if(argc < 2) {
     printf("Usage: getmx <domain_name> [your_dns_server_ip]\n"); 
     exit(-1); 
    }
    domain = argv[1]; 
    dns_svr = argc > 2 ? argv[2] : backup_dns; 

#ifdef WINDOWS
	initWinsockDll();
#endif

    mx_ip = getmx(domain, dns_svr); 
    if (mx_ip > 0) {  
       memcpy(&in.s_addr, &mx_ip, 4);
       if (strncpy(host, inet_ntoa(in), sizeof(host))  > 0)
          printf("%s MX: %s\n", domain, host); 
    } else {
          printf("%s MX: not found!\n", domain, host); 
    }

#ifdef WINDOWS
	WSACleanup();
    return(0);
#else
	exit(0);
#endif
}

unsigned int getmx(char *domainp, char *dns_server_ip) {  
    static unsigned short id = 255;  /* starting query id */
    unsigned short nid, aid;  /* id in network byte order */ 
    unsigned short ancount, nscount, arcount; /* counts of 3 RR groups */ 
    unsigned int mxip; /* mx ip **/
    int sockfd, len, hdrlen = 12, alen; 
    struct sockaddr_in saddr;   /** server address */ 
    char qbuf[1024], abuf[2048];  /* query and answer buffers */ 
    char *paybuf, *pp, tmp[256]; 
    char domaincp[256], *domain; 
    int kn; 

    nid = htons(id++); 
    saddr.sin_family = AF_INET; 
    saddr.sin_port = htons(53); 

#ifdef WINDOWS
	saddr.sin_addr.s_addr = inet_addr(dns_server_ip);
#else
    inet_pton(AF_INET, dns_server_ip, &saddr.sin_addr); 
#endif

    strncpy (domaincp, domainp, 256); 
    domain = domaincp;

    /* construct the query */ 
    *qbuf = '\0';
    paybuf = qbuf + hdrlen;   /** query position */ 
    *paybuf = '\0'; 

    while ( (pp = strchr(domain, '.') )!=NULL ) {
            *pp++ = '\0';
            sprintf(tmp, "%c%s", strlen(domain), domain);
            strcat( paybuf, tmp);
            domain = pp;
    }  /* end while */
            sprintf(tmp, "%c%s", strlen(domain), domain);
            strcat( paybuf, tmp);

         len = strlen(paybuf) + 1; 
         *(paybuf + len ) = '\x00'; 
         *(paybuf + len + 1 ) = '\x0f';  /* qtype: MX */ 
         *(paybuf + len + 2 ) = '\x00'; 
         *(paybuf + len + 3 ) = '\x01'; 
         len += 4; 

     /* craft header */ 
         memcpy(qbuf, &nid, 2);  /* get query id in */
         *(qbuf + 2) = '\x01'; 
         *(qbuf + 3) = '\x00'; 
         *(qbuf + 4) = '\x00'; 
         *(qbuf + 5) = '\x01'; 
         *(qbuf + 6) = '\x00'; 
         *(qbuf + 7) = '\x00'; 
         *(qbuf + 8) = '\x00'; 
         *(qbuf + 9) = '\x00'; 
         *(qbuf + 10) = '\x00'; 
         *(qbuf + 11) = '\x00'; 
         len += 12; 

      /* ===== for message body debugging 
         for (kn=0; kn < len; kn++) {
            printf(" %02X", *(qbuf+kn) );
         }
         printf("\n");
       ===== for message body debugging  */

    sockfd = socket(AF_INET, SOCK_DGRAM, 0); 
    sendto(sockfd, qbuf, len, 0, (struct sockaddr *) &saddr, sizeof(saddr) ); 

    do { 
      if (readable_timeo(sockfd, TIME_OUT) == 0) {
        fprintf(stderr, "Timeout:\n"); 
        return(0); 
      }
     alen = recvfrom(sockfd, abuf, 2048, 0, NULL, NULL); 

     if (alen > len ) { //get some answer appended to query back

       memcpy(&aid, abuf, 2);  /* get query id in */
       memcpy(&ancount, abuf+6, 2);  /* number of AN RR */ 
       memcpy(&nscount, abuf+8, 2);  /* number of NS RR */ 
       memcpy(&arcount, abuf+10, 2);  /* number of AR RR */ 
       ancount = ntohs(ancount); 
       nscount = ntohs(nscount); 
       arcount = ntohs(arcount); 

       //printf("aid=%d, nid=%d\n", aid, nid); 

       printf("AN=%d, NS=%d, AR=%d\n", 
         ancount, nscount, arcount ); 
     }
    } while (aid != nid); 

#ifdef WINDOWS

    closesocket(sockfd); 

#else

    close(sockfd); 

#endif



       //printf("out of do: aid=%d, nid=%d\n", aid, nid); 

    if(ancount==0) {

      fprintf(stderr, "can not find mx!\n"); 

      return 0; 

    }



    /*** Now get the right reply ***/ 

    pp = getnextRR(abuf, abuf+len);   /** move to point to next RR */

    if(arcount==0) {   /* no AR RR. get ip from AN RR */ 

      mxip = parseAN(abuf, pp); 

    } else {   /** get ip from AR RR */

      

     for (kn = 1; kn < ancount + nscount; kn++) { /*skip AN and NS RRs */

      //printf("kn=%d, abuf=%p, pp=%p %d\n", kn,  abuf, pp, pp-abuf); 

      pp = getnextRR(abuf, pp); 

     }

     /* now pp points to the first AR RR */ 

     //printf("to parse ip: abuf=%p, pp=%p %d\n", abuf, pp, pp-abuf); 

     mxip = parseAR(abuf, pp); 



   } /* done if */ 

   

    return mxip; 

}  /* end of function */ 

      

char *getnextRR(char *abuf, char *pp) {

    unsigned short rdlen; 

    

   /** skip name field */

    while (*pp < 64 && *pp > 0 ) { /* no compression */ 

        pp += (*pp + 1);

    } 

    /* now points to \0 or C0 */



   if( (*pp & '\xc0') == '\xc0')  /** compression **/ 

      pp +=2; 

    else 

      pp +=1; 



   /* now skip type(2), class(2), TTL(4), and point to data length (2) */

      pp += 8; 

      memcpy(&rdlen, pp, 2);  /* data lengtht */ 

      pp += (2 + ntohs(rdlen) ); 

    return pp; 

}



unsigned int parseAR(char *abuf, char *pp)  /* start of while msg, & pointer to
RR */

{

   unsigned short rdlen; 

   unsigned int ip; 



   /** skip name field */

    while (*pp < 64 && *pp > 0 ) { /* no compression */ 

        pp += (*pp + 1);

    }

    /* now points to \0 or C0 */



   if( (*pp & '\xc0') == '\xc0')  /** compression **/

      pp +=2;

    else

      pp +=1;



   /* now skip type(2), class(2), TTL(4), and point to data length (2) */

      pp += 8;

      memcpy(&rdlen, pp, 2);  /* data lengtht */

      if(ntohs(rdlen) != 4 ) { 

         fprintf(stderr, "Parse error!\n"); 

         return 0; 

      }

      memcpy(&ip, pp+2, 4);  /* data lengtht */

    return ip; 

}





unsigned int parseAN(char *abuf, char *pp)  /* start of while msg, & pointer to
RR */

{

   unsigned short rdlen, pref;

   unsigned int ip;

   char tmp[1024]; 

   struct hostent *hn; 



   /** skip name field */

    while (*pp < 64 && *pp > 0 ) { /* no compression */

        pp += (*pp + 1);

    }

    /* now points to \0 or C0 */



   if( (*pp & '\xc0') == '\xc0')  /** compression **/

      pp +=2;

    else

      pp +=1;



   /* now skip type(2), class(2), TTL(4), and point to data length (2) */

      pp += 8;

      memcpy(&rdlen, pp, 2);  /* data length */

      rdlen = ntohs(rdlen); 

      pp +=2; /* points to preference now */

      memcpy(&pref, pp, 2);  /* data length */

      pref = ntohs(pref); 

      pp +=2; /* points to hostname now */



      tmp[0] = '\0'; 

      while (*pp != '\0') {

       if (*pp < 64 && *pp > 0 ) { /* no compression */

         strncat(tmp, pp+1, *pp); 

         strncat(tmp, ".", 1); 

         pp += ( 1 + *pp ); 

       } else if ( (*pp & '\xc0') == '\xc0') {  /** compression **/

         pp = abuf + *(pp+1); 

       } /* end if */

      } /* end while */

      tmp[strlen(tmp)-1] = '\0'; 

      hn = gethostbyname(tmp); 

      printf("MX host, %s, h_length=%d\n", tmp, hn->h_length);  

      memcpy(&ip, hn->h_addr_list[0], 4); 

    return ip;

}

int readable_timeo(int fd, int sec) {
  fd_set rset;
  struct timeval tv;
  int rc;

  FD_ZERO(&rset);
  FD_SET(fd, &rset);

  tv.tv_sec = sec;
  tv.tv_usec = 0;

  rc = select(fd+1, &rset, NULL, NULL, &tv);
  return rc;
}

