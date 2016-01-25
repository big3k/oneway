
// $Id: sj-enc.cpp,v 1.4 2011/06/06 18:52:52 oneway Exp oneway $ 
// oneway modified a bit to eliminate malloc calls inside 
// functions for performance,
// and re-factored into Algorithms 0, 1, 2 and 3.  

//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

inline char swap_bit(char byte) {
    unsigned char b = (unsigned char)byte;
    b = ((b & 0x0f) << 4) | ((b & 0xf0) >> 4);
    return (char)b;
}


inline void block_encrypt1(char* data, int len, char key) {
    for (int i = 0; i < len; ++i) {
        data[i] ^= key;
    }
}

inline void block_decrypt1(char* data, int len, char key) {
    for (int i = 0; i < len; ++i) {
        data[i] ^= key;
    }
}


inline void block_encrypt2(char* data, int len, char key) {
    if (len) {
      data[0] = data[0] ^ key;
    }
    for (int i = 1; i < len; ++i) {
        data[i] ^= data[i-1];
        data[i] = swap_bit(data[i]);
    }
}

inline void block_decrypt2(char* data, int len, char key) {
    for (int i = len - 1; i > 0; --i) {
        data[i] = swap_bit(data[i]);
        data[i] ^= data[i-1];
    }
    if (len) {
        data[0] ^= key;
    }
}

/* En/Decryption routines: 0, 1, 2 and 3.
   All the routines have the same interface. 

   For encryption, the returned ciphertext is always 2-byte longer than 
   the input plaintext. For decryption, it is the opposite. 

input: 
    pack -- pointer to input data buffer
    len  -- input data size 
    ret  -- pointer to output data buffer, preallocated, can be re-used. 
output: 
    each function returns the new size of the de/encrypted data pointed by ret. 

Note: 
    The following statement can be used at the beginning of the calling 
     program  to seed rand(). 

    srand(time(NULL));
*/
    

/* no en/decryption at all -- straight copy w/ 2-byte padding */ 
int EncryptPacket0(char* pack, int len, char* ret) {
    memcpy(ret, pack, len);
    ret[len]   =  rand() & 0xff;
    ret[len+1] =  rand() & 0xff;
    return len+2; 
}


int DecryptPacket0(char* pack, int len, char* ret) {
    memcpy(ret, pack, len-2);
    return len-2; 
}

/* SJ-enc: rewritten to eliminate malloc calls. */ 
   
int EncryptPacket1(char* pack, int len, char* ret) {

    unsigned char pad1 = rand() & 0xff;
    unsigned char pad2 = rand() & 0xff;

    memcpy(ret, pack, len);
    block_encrypt1(ret, len, pad1);

    int loc1 = pad2 % len;
    ret[len] = ret[loc1];
    ret[loc1] = pad1;

    block_encrypt1(ret, len, pad2);

    unsigned int loc2 = 0;
    for (int i = 0; i < len / 2; ++i) {
        loc2 ^= ret[i];
    }
    if (len > 1) {
        loc2 = loc2 % (len / 2);
    }
    loc2 += len / 2;

    ret[len+1] = ret[loc2];
    ret[loc2] = pad2;

    return len+2;
}

int DecryptPacket1(char* pack, int len, char* ret) {
    len -= 2;

    //no need:  srand(time(NULL));

    unsigned int loc2 = 0;
    int i;
    for (i = 0; i < len / 2; ++i) {
        loc2 ^= pack[i];
    }

    if (len > 1) {
        loc2 = loc2 % (len / 2);
    }
    loc2 += len / 2;

    unsigned char pad2 = pack[loc2];
    pack[loc2] = pack[len + 1];
    memcpy(ret, pack, len);
    block_decrypt1(ret, len, pad2);

    int loc1 = pad2 % len;
    unsigned char pad1 = ret[loc1];
    ret[loc1] = pack[len];
    block_decrypt1(ret, len, pad1);

    return len;
}

int EncryptPacket2(char* pack, int len, char* ret) {

    unsigned char pad1 = rand() & 0xff;
    unsigned char pad2 = rand() & 0xff;

    memcpy(ret, pack, len);
    block_encrypt2(ret, len, pad1);

    int loc1 = pad2 % len;
    ret[len] = ret[loc1];
    ret[loc1] = pad1;

    block_encrypt2(ret, len, pad2);

    unsigned int loc2 = 0;
    for (int i = 0; i < len / 2; ++i) {
        loc2 ^= ret[i];
    }
    if (len > 1) {
        loc2 = loc2 % (len / 2);
    }
    loc2 += len / 2;

    ret[len+1] = ret[loc2];
    ret[loc2] = pad2;

    return len+2;
}

int DecryptPacket2(char* pack, int len, char* ret) {
    len -= 2;


    unsigned int loc2 = 0;
    int i;
    for (i = 0; i < len / 2; ++i) {
        loc2 ^= pack[i];
    }

    if (len > 1) {
        loc2 = loc2 % (len / 2);
    }
    loc2 += len / 2;

    unsigned char pad2 = pack[loc2];
    pack[loc2] = pack[len + 1];
    memcpy(ret, pack, len);
    block_decrypt2(ret, len, pad2);

    int loc1 = pad2 % len;
    unsigned char pad1 = ret[loc1];
    ret[loc1] = pack[len];
    block_decrypt2(ret, len, pad1);

    return len;
}



/* oneway's en/decryption algorithm. it has to know tcp header  */ 

int DecryptPacket3(char* pack, int len, char* ret) {
    unsigned i; 
    unsigned char key;
    unsigned char pad1; 
    unsigned char pad2; 

    len=len-2; 
    pad1 = pack[len]; 
    pad2 = pack[len+1]; 
    memcpy(ret, pack, len);
    key = ( ( pad1 * pad2 ) % 127 ) + 1;
    for (i=0; i<len; i++) ret[i] = ret[i] ^ key;

    return len; 
}

int EncryptPacket3(char* pack, int len, char* ret) {
    unsigned i; 
    unsigned char key;
    unsigned char pad1 = rand() & 0xff;
    unsigned char pad2 = rand() & 0xff;

    key = ( ( pad1 * pad2 ) % 127 ) + 1;
    memcpy(ret, pack, len);
    for (i=0; i<len; i++) ret[i] = ret[i] ^ key;
    ret[len] = pad1; 
    ret[len+1] = pad2; 
    
    return len+2;
}


