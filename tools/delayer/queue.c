
#include <string.h> 
#include "queue.h"

void init_queue(struct packet_queue *p_queue) {
        p_queue->first =0; 
        p_queue->last  = -1; 
        p_queue->count  =0; 
}

/* add new element to last */
void enqueue(struct packet_queue *p_queue, unsigned char *data, int len, int pid) {
        p_queue->last  = (p_queue->last+1) % QSIZE; 
        p_queue->count++; 
        memcpy(p_queue->p_data[p_queue->last].data, data, len); 
        p_queue->p_data[p_queue->last].len = len; 
        p_queue->p_data[p_queue->last].pid = pid; 
}


/* returnes oldest element, and length  */
/* count is guranteed to be non-zero because of the use of semaphore */
unsigned char *dequeue(struct packet_queue *p_queue, int *len, int *pid) {
        unsigned char *data; 
        *len=p_queue->p_data[p_queue->first].len; 
        *pid=p_queue->p_data[p_queue->first].pid; 
        data = p_queue->p_data[p_queue->first].data; 
        p_queue->first  = (p_queue->first+1) % QSIZE; 
        p_queue->count--; 
        return data; 
}

