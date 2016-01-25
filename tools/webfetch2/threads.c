#include <windows.h>
#include <winsock.h>
#include <process.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <time.h>

#include <curl/curl.h>
#include "webfetch2.h"
#include "threads.h"

void tcp_proc( LPVOID lpParam ) {  
   int *i; 
   char dbuff[2048]; 
   DWORD termEventResult; 
   struct sockaddr_in server; 
   int sock; 

   i = (int)lpParam; 

   memset(&server, 0, sizeof(server)); 
   server.sin_family      = AF_INET;             /* Internet address family */
   server.sin_addr.s_addr = inet_addr(ipstr[*i]);   /* Server IP address */
   server.sin_port        = htons(port[*i]); /* Server port */


   for (;;) { 
     //printf("TCP thread %d\n", *i); 

     if ((sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
         printf("TCP thread %d: socket open failed\n", *i); 
         _endthread(); 
     }

     if (connect(sock, (struct sockaddr *)&server, sizeof(server) ) < 0 ) {
       printf("TCP thread %d: socket connection failed. Keep trying ...\n", *i); 
     } else { 
       printf("TCP thread %d: socket connected. Receiving data ...\n", *i); 
       recv(sock, dbuff, 2047, 0); 
     }
     
     termEventResult = WaitForSingleObject(termEvent, 2000); 
     switch(termEventResult) { 
       case WAIT_OBJECT_0:
         printf("TCP thread %d exiting ...\n", *i); 
         fflush(stdout);
         closesocket(sock); 
         _endthread(); 
         break; 
       default: 
         //printf("TCP thread %d continue ...\n", *i); 
         closesocket(sock); 
         fflush(stdout);
         break; 
     } // end switch 

   } // end for(;;)
}

void udp_proc( LPVOID lpParam ) { 
   int *i;
   char dbuff[1024];
   int dsize, j; 
   DWORD termEventResult;
   struct sockaddr_in server;
   int sock;
   
   i = (int)lpParam;
   srand( GetCurrentThreadId() );
   //srand( (unsigned)time( NULL ) );

   memset(&server, 0, sizeof(server));
   server.sin_family      = AF_INET;             /* Internet address family */
   server.sin_addr.s_addr = inet_addr(ipstr[*i]);   /* Server IP address */
   server.sin_port        = htons(port[*i]); /* Server port */

   if ((sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
       printf("UDP thread %d: socket open failed\n", *i);
       _endthread();
   }

   for (;;) {
     //printf("UDP thread %d\n", *i);

     // create random data
     dsize = rand()%1024+10; 
     for (j=0; j<dsize; j++) dbuff[j] = rand()%256; 

     if (sendto(sock, dbuff, dsize, 0, (struct sockaddr *)&server, sizeof(server) ) < 0 ) {
       printf("UDP thread %d: sendto failed. Keep trying ...\n", *i);
     } else {
       //printf("UDP thread %d: data sent. short break ...\n", *i);
     }

     termEventResult = WaitForSingleObject(termEvent, 500);
     switch(termEventResult) {
       case WAIT_OBJECT_0:
         printf("UDP thread %d exiting ...\n", *i);
         fflush(stdout);
         closesocket(sock);
         _endthread();
         break;
       default:
         //printf("UDP thread %d continue ...\n", *i);
         fflush(stdout);
         break;
     } // end switch

   } // end for(;;)
}

static size_t mycb(void *ptr, size_t size, size_t nmemb, void *data) {
 return size*nmemb; 
}

void http_proc( LPVOID lpParam ) { 
  char url[MAXTEXT]; 
  CURL *curl;
  CURLcode res;
   int *i;
   DWORD termEventResult;

   i = (int)lpParam;
   snprintf(url, MAXTEXT, "http://%s", ipstr[*i]); 
   printf("HTTP thread %d, url=%s\n", *i, url);
     curl = curl_easy_init();
     curl_easy_setopt(curl, CURLOPT_URL, url); 
     curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, mycb); 
     curl_easy_setopt(curl, CURLOPT_USERAGENT, "Mozilla-agent/1.0");

   for (;;) {
     res = curl_easy_perform(curl);
     if (res==0) {
       //printf("HTTP thread fetching ok\n"); 
     } else { 
       //printf("HTTP thread fetching error: %d\n", res); 
     }
     fflush(stdout); 
     // check if told to quit
     termEventResult = WaitForSingleObject(termEvent, 2000);
     switch(termEventResult) {
       case WAIT_OBJECT_0:
         printf("HTTP thread %d exiting ...\n", *i);
         fflush(stdout);
         curl_easy_cleanup(curl);
         _endthread();
         break;
       default:
        //printf("HTT thread %d continue ...\n", *i);
         fflush(stdout);
         break;
     } // end switch

   } // end for(;;)
}

// largely copied from 
// http://www.binarytides.com/blog/dns-query-code-in-c-with-winsock-and-linux-sockets/

void ChangetoDnsNameFormat(unsigned char* dns,unsigned char* host) {
int lock=0 , i;
 
strcat((char*)host,".");
 
for(i=0;i<(int)strlen((char*)host);i++)
{
if(host[i]=='.')
{
*dns++=i-lock;
for(;lock<i;lock++)
{
*dns++=host[lock];
}
lock++; //or lock=i+1;
}
}
*dns++='\0';
}


void dns_proc( LPVOID lpParam ) { 
   int *i;
   char buf[4096], host[1024], dnsip[MAXTEXT], domain[MAXTEXT];
   char *ctmp, *qname; 
   int dsize, j;
   DWORD termEventResult;
   struct sockaddr_in server;
   int sock;
   struct DNS_HEADER *dns = NULL;
   struct QUESTION *qinfo = NULL;

   i = (int)lpParam;
   srand( GetCurrentThreadId() );
   //srand( (unsigned)time( NULL ) );

   //Set the DNS structure to standard queries
   dns = (struct DNS_HEADER *)&buf;
 
   dns->qr = 0; //This is a query
   dns->opcode = 0; //This is a standard query
   dns->aa = 0; //Not Authoritative
   dns->tc = 0; //This message is not truncated
   dns->rd = 1; //Recursion Desired
   dns->ra = 0; //Recursion not available! hey we dont have it (lol)
   dns->z = 0;
   dns->ad = 0;
   dns->cd = 0;
   dns->rcode = 0;
   dns->q_count = htons(1); //we have only 1 question
   dns->ans_count = 0;
   dns->auth_count = 0;
   dns->add_count = 0;
 
   //point to the query portion
   qname =(unsigned char*)&buf[sizeof(struct DNS_HEADER)];

   //ipstr has this format: edoors.com/72.52.66.12
   ctmp = ipstr[*i]; 
   memset(domain, 0, MAXTEXT);
   strncpy(domain, ctmp, strstr(ctmp, "/") - ctmp); 
   ctmp = strstr(ctmp, "/") + 1; 
   memset(dnsip, 0, MAXTEXT);
   strncpy(dnsip, ctmp, strlen(ctmp)); 

   memset(&server, 0, sizeof(server));
   server.sin_family      = AF_INET;             /* Internet address family */
   server.sin_addr.s_addr = inet_addr(dnsip);   /* Server IP address */
   server.sin_port        = htons(53);          /* Server port */

   if ((sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
       printf("DNS thread %d: socket open failed\n", *i);
       _endthread();
   }

   for (;;) {
     //printf("DNS thread %d\n", *i);

     // create random host data
     memset(host, 0, 1024);
     dsize = rand()%32+1;   // max host name length  
     for (j=0; j<dsize; j++) host[j] = rand()%26+97;
     host[dsize] = '.'; 
     strcat(host, domain); 
     //printf("Resolving host: %s\n", host); 

     dns->id = (unsigned short) rand(); 
     ChangetoDnsNameFormat(qname,host);
     qinfo =(struct QUESTION*)&buf[sizeof(struct DNS_HEADER) + 
                   (strlen((const char*)qname) + 1)]; //fill it
 
     qinfo->qtype = htons(1); //we are requesting the ipv4 address
     qinfo->qclass = htons(1); //its internet (lol)

     if (sendto(sock, buf, 
                sizeof(struct DNS_HEADER) + (strlen((const char*)qname)+1) 
                + sizeof(struct QUESTION), 
                0, (struct sockaddr *)&server, sizeof(server) ) < 0 ) {
       printf("DNS thread %d: sendto failed. Keep trying ...\n", *i);
     } else {
      //printf("DNS thread %d: data sent. short break ...\n", *i);
     }

     termEventResult = WaitForSingleObject(termEvent, 5000);
     switch(termEventResult) {
       case WAIT_OBJECT_0:
         printf("DNS thread %d exiting ...\n", *i);
         fflush(stdout);
         closesocket(sock);
         _endthread();
         break;
       default:
        // printf("DNS thread %d continue ...\n", *i);
         fflush(stdout);
         break;
     } // end switch

   } // end for(;;)
}
