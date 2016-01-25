
/* ================================================================
  $Id: delayer.c,v 1.5 2010/12/29 00:28:55 oneway Exp oneway $ 
  Check intercepted TCP segments, and if anyone's payload matches a 
  predefined string (keywords, e.g., "HTTP OK"), save a copy of the 
  segment, tell kernel to drop the original segment, and let slave threads
  send the copied segment out after a delay. The purpose of doing this is to 
  swap the IP packets sequences to make it harder for keyword-inspection firewalls
  to reassemble a TCP stream. 
  
  A smaple iptables rule to divert packets: 
  /sbin/iptables -I OUTPUT -m length -p tcp --length 200:4000 --sport 80 \
   --tcp-flags ALL ACK -j QUEUE
====================================================================*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h> 
#include <pthread.h> 
#include <semaphore.h>
#include <unistd.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <linux/types.h>
#include <linux/netfilter.h>		/* for NF_ACCEPT */

#include <libnetfilter_queue/libnetfilter_queue.h>

#define __FAVOR_BSD
#include <netinet/tcp.h>

#include "queue.h" 
#include "sender_threads.h" 

/* Global variables */
struct nfq_q_handle *qh;
struct packet_queue p_queue; 
pthread_mutex_t lock_queue = PTHREAD_MUTEX_INITIALIZER; 
pthread_mutex_t lock_qh = PTHREAD_MUTEX_INITIALIZER; 
sem_t sem_queue; 

//General Networking Functions
 
/* open a TCP/IP raw socket for sending data */
/*  returns socket id if successful, -1 otherwise */
 int open_rawsock() { 
    int on=1; /* IP_HDRINCL on or off. 0: let kernel make header */
    int rawsocket;

    if ( (rawsocket = socket(AF_INET, SOCK_RAW, IPPROTO_IPIP)) < 0){
        perror("main:open_rawsock()");
        return(-1);
    }

    //* we build the ip header 
    if( setsockopt(rawsocket, IPPROTO_IP, IP_HDRINCL, &on, sizeof(on)) < 0){
        perror("main:setsockopt()");
        return(-1);
    }
    
    return rawsocket; 
}

void print_ip_pkt(unsigned char *data, unsigned int ip_len) { 
    int i; 

    printf("payload_len=%d \n", ip_len);
    // display packet data
    printf("%43s", " ");
    for (i = 0; i < ip_len; i++) {
        printf("%02x ", data[i]);
        switch ( (i+15)% 16 ) {
           case 0: printf("\n");  break;
           case 8: printf(" ");  break;
        }
    }
   fputc('\n', stdout);

}


unsigned int inspect_tcp(unsigned char *data, int ip_len) { 
  char buff[PSIZE]; 
  char *payload, *cmatch; 
  const char keywords[] = "HTTP/1.1 200 OK"; 
  int paylen; 

  payload = (char *)(data + sizeof(struct ip) + sizeof(struct tcphdr) ); 
  paylen = ip_len - sizeof(struct ip) - sizeof(struct tcphdr); 
  if (paylen == 0 || paylen > PSIZE) return 0;  // no further questions 
  strncpy(buff, payload, paylen); 
  buff[paylen] = '\0';     // end it 

  cmatch = strstr(buff, keywords); 
  if ( cmatch ) return 1; 
  return 0; 
}
  
/* returns packet id */
static u_int32_t display_pkt (struct nfq_data *tb)
{
	int id = 0;
	struct nfqnl_msg_packet_hdr *ph;
	int ret;
	unsigned char *data;

	ph = nfq_get_msg_packet_hdr(tb);
	if (ph) {
		id = ntohl(ph->packet_id);
#ifdef DEBUG
		printf("hw_protocol=0x%04x hook=%u id=%u \n",
			ntohs(ph->hw_protocol), ph->hook, id);
#endif
	}

	ret = nfq_get_payload(tb, &data);
	// if (ret >= 0) print_ip_pkt(data, ret); 

	return id;
}
	

static int cb(struct nfq_q_handle *qh, struct nfgenmsg *nfmsg,
	      struct nfq_data *nfa, void *data)
{
	unsigned char *payload;
	int pay_len, match, rlen; 
	u_int32_t id = display_pkt(nfa);
	pay_len = nfq_get_payload(nfa, &payload);  
#ifdef DEBUG
        printf("Content inspection...pay_len=%d\n", pay_len); 
#endif
        match = inspect_tcp(payload, pay_len);  // return 1 if matched, 0 otherwise 
        if (match && p_queue.count < QSIZE) {  // if queue is full, let them pass 
           //print_ip_pkt(payload, pay_len); 
           // now lock the queue
           pthread_mutex_lock (&lock_queue); 
           enqueue(&p_queue, payload, pay_len, id);  
#ifdef DEBUG
	   printf("Matched packet. Queue size: %d\n", p_queue.count); 
#endif
           pthread_mutex_unlock (&lock_queue); 
           sem_post(&sem_queue);   // wake up sending threads 
	   return 0; 
       } else {
#ifdef DEBUG
	   printf("No match found\n");         // should be ok for not locking the queue
#endif
           pthread_mutex_lock (&lock_qh); 
	   rlen = nfq_set_verdict(qh, id, NF_ACCEPT, pay_len, payload);
           pthread_mutex_unlock (&lock_qh); 
	   return rlen; 
       }
}

int main(int argc, char **argv)
{
	struct nfq_handle *h;
	int fd, i, rc;
	int rv;
        pthread_t tid[NUM_THREADS+1]; 
        int id[NUM_THREADS+1]; 

	char buf[4096] __attribute__ ((aligned));

        init_queue(&p_queue); 
        sem_init(&sem_queue, 0, 0);
        /*** launch threads **/
        for (i=1; i<= NUM_THREADS; i++) { 
          id[i] = i; 
          rc = pthread_create(&tid[i], NULL, sender_threads, &id[i]); 
        }

	printf("opening library handle\n");
	h = nfq_open();
	if (!h) {
		fprintf(stderr, "error during nfq_open()\n");
		exit(1);
	}

	printf("unbinding existing nf_queue handler for AF_INET (if any)\n");
	if (nfq_unbind_pf(h, AF_INET) < 0) {
		fprintf(stderr, "error during nfq_unbind_pf()\n");
		exit(1);
	}

	printf("binding nfnetlink_queue as nf_queue handler for AF_INET\n");
	if (nfq_bind_pf(h, AF_INET) < 0) {
		fprintf(stderr, "error during nfq_bind_pf()\n");
		exit(1);
	}

	printf("binding this socket to queue '0'\n");
	qh = nfq_create_queue(h,  0, &cb, NULL);
	if (!qh) {
		fprintf(stderr, "error during nfq_create_queue()\n");
		exit(1);
	}

	printf("setting copy_packet mode\n");
	if (nfq_set_mode(qh, NFQNL_COPY_PACKET, 0xffff) < 0) {
		fprintf(stderr, "can't set packet_copy mode\n");
		exit(1);
	}

	fd = nfq_fd(h);

	while ((rv = recv(fd, buf, sizeof(buf), 0)) && rv >= 0) {
		printf("pkt received\n");
		nfq_handle_packet(h, buf, rv);
	}

	printf("unbinding from queue 0\n");
	nfq_destroy_queue(qh);
        // clean up threads 
        for (i=1; i<= NUM_THREADS; i++) pthread_join(tid[i], NULL);
        pthread_mutex_destroy (&lock_queue);
        sem_destroy(&sem_queue); 

#ifdef INSANE
	/* normally, applications SHOULD NOT issue this command, since
	 * it detaches other programs/sockets from AF_INET, too ! */
	printf("unbinding from AF_INET\n");
	nfq_unbind_pf(h, AF_INET);
#endif

	printf("closing library handle\n");
	nfq_close(h);

	exit(0);
}
