 /* rc4.h */

#ifndef __RC4_H__
#define __RC4_H__

#ifdef __cplusplus
extern "C" {
#endif

#define RC4_BLOCK_SIZE	256

typedef struct rc4_key
{      
   unsigned char state[RC4_BLOCK_SIZE];       
   unsigned char x;        
   unsigned char y;
} rc4_key;

void prepare_key(unsigned char *key_data_ptr,int key_data_len,
		 rc4_key *key);
void rc4(unsigned char *buffer_ptr,int buffer_len,rc4_key * key);

#ifdef __cplusplus
}
#endif

#endif
