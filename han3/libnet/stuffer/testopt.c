#include <stdio.h> 
#include <stdlib.h> 
#include <pcap.h>
#include <libnet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/if_ether.h>
#include <net/ethernet.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>


int main(int argc, char *argv[])
{
    int c, i; 
    while ((c = getopt(argc, argv, "d:s:p:f:i:")) != EOF)
    {
        switch (c)
        {
         case 'd':
            printf("%s\n", optarg); 
           break;
         case 's':
            printf("%s\n", optarg); 
           break;
         case 'p':
           break;
         case 'f':
           break;
         case 'i':
           break;
         default:
                exit(1);  
        }
    }

}


