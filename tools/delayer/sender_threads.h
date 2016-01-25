/* $Id: sender_threads.h,v 1.3 2010/12/28 23:37:30 oneway Exp $ */ 
#define _REENTRANT

/* number of initial threads used to handle requests */
#define NUM_THREADS 10

/* number of microseconds of delay for sending intercepted packet */ 
/* The longer the delay and the busier the server, the more threads needed */
#define UDELAY 100000 

/* Global variables */
extern struct nfq_q_handle *qh;
extern struct packet_queue p_queue; 
extern pthread_mutex_t lock_queue; 
extern pthread_mutex_t lock_qh; 
extern sem_t sem_queue; 

void *sender_threads(void *arg); 

// sending packet using raw socket 
int  send_packet(int rawsock, unsigned char *packet, int len);  

