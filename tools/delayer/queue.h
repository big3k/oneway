/*  $Id   */
/*  queue for packet data */

#define QSIZE 4096
#define PSIZE 4096   // max ip packet size 

struct packet_data {
       unsigned char data[PSIZE]; 
       int  len; 
       int  pid; 
}; 


struct packet_queue { 
       struct packet_data p_data[QSIZE]; 
       unsigned int first;
       unsigned int last; 
       int count; 
}; 


void init_queue(struct packet_queue *p_queue); 
void enqueue(struct packet_queue *p_queue, unsigned char *data, int len, int pid); 
unsigned char *dequeue(struct packet_queue *p_queue, int *len, int *pid);





