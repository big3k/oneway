#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <semaphore.h>


unsigned int inspect_tcp(unsigned char *data, unsigned int ip_len) {
  //char buff[2048];
  //char buff[2048] = "HTTP/1.1 200 OK abcd";
  char buff[2048] = "xxxxxxxxxxx00 OK abcd";
  char *payload, *cmatch;
  const char keywords[] = "HTTP/1.1 200 OK";
  unsigned int paylen;

  printf("inside content inspection...\n");
  cmatch = strstr(buff, keywords);
  printf("done content inspection...\n");
  if ( cmatch ) {
        printf("matched content inspection...%p\n", cmatch);
        return 1;
  } else {
        printf("no matched content inspection...%p\n", cmatch);
        return 0;
  }
}


int main(int argc, char **argv)
{
        unsigned int i; 
        unsigned char *data; 

       i = inspect_tcp(data, i); 
       printf("i=%d \n", i); 

}


