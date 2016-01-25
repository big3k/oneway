/****************************************************************************
** sender_threads.c 
** $Id: sender_threads.c,v 1.1 2010/12/28 23:58:20 oneway Exp oneway $ 
**
******************************************************************************/

#include <stdio.h>             /* standard I/O routines                      */
#include <pthread.h>           /* pthread functions and data structures      */
#include <semaphore.h>           /* pthread functions and data structures      */
#include <stdlib.h>            /* rand() and srand() functions               */
#include <unistd.h>            /* sleep()                                    */
#include <assert.h>            /* assert()                                   */
#include <string.h>            /* sleep()                                    */
#include <sys/select.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <signal.h>
#include <errno.h>
#include <netdb.h>
#include <ctype.h>
#include <netinet/ip.h>
#include <linux/netfilter.h>            /* for NF_ACCEPT */
#include <libnetfilter_queue/libnetfilter_queue.h>

#define __FAVOR_BSD
#include <netinet/tcp.h>

#include "queue.h" 
#include "sender_threads.h" 


#define __USE_BSD 1
#define __BYTE_ORDER __LITTLE_ENDIAN

void *sender_threads(void *arg) {   /** slave thread **/

unsigned char *packet; 
int len; 
int id, pid;   // thread id and packet id

id = *((int *) arg); 


for (;;) {  
#ifdef DEBUG
  printf("Thread %d standby ...\n", id); 
#endif
  sem_wait(&sem_queue); 
  pthread_mutex_lock (&lock_queue); 
  packet = dequeue(&p_queue, &len, &pid); 
  pthread_mutex_unlock (&lock_queue); 

#ifdef DEBUG
  printf("Thread %d got job ...\n", id); 
#endif
  // now job assigned. do it
  usleep(UDELAY); 

  pthread_mutex_lock (&lock_qh);
  //send_packet(rawsock, packet, len); 
  nfq_set_verdict(qh, pid, NF_ACCEPT, len, packet);
  pthread_mutex_unlock (&lock_qh);
}  // end for

} /** end of thread **/

// Sending packet with raw socket. Not used anymore. 
int send_packet(int rawsock, unsigned char *packet, int len) { 
  int i;
  struct ip *iph = (struct ip *) packet;
  struct tcphdr *tcph = (struct tcphdr *) ( packet + sizeof(struct ip) );
  struct sockaddr_in dsin;  // dst addr for sendto()

  dsin.sin_family = AF_INET;
  dsin.sin_port = ntohs(tcph->th_dport); 
  dsin.sin_addr.s_addr = iph->ip_dst.s_addr;

  i=sendto(rawsock, packet, len, 0, (struct sockaddr *) &dsin, sizeof (dsin));

  if ( i == -1 )  {
       fprintf(stderr, "sending to %s failed!\n", inet_ntoa(dsin.sin_addr));
  } else  {
       printf("Outgoing packet: \n");
       printf("%u bytes sent to  %s \n", i, inet_ntoa(dsin.sin_addr));
       /**--- for debugging only **/
       fprintf(stderr, "Src IP: %s ==>", inet_ntoa(iph->ip_src));
       fprintf(stderr, "Dst IP: %s\n", inet_ntoa(iph->ip_dst));
       fprintf(stderr, "Src port: %d ==>", ntohs(tcph->th_sport)); 
       fprintf(stderr, "Dst port: %d\n\n", dsin.sin_port);
       fflush(stderr);
       /* */
  }
  return i; 
}  // end of function





