
#include <time.h> 
#include <string.h> 

int main(int argc, char **argv)
{

   int i; 
   char *tstr; 
   time_t rawt; 

      time ( &rawt );
      tstr = ctime( &rawt );
      
      for (i=4; i<strlen(tstr)-5; i++) putchar(tstr[i]);
      printf("TCP: \n"); 

    return 0; 
} 

