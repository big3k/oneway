
/*  This is to be compiled and run on the web server. 

Usage: 
encode_config config_file_name > config.txt

*/

#include <stdio.h>
#include <stdlib.h>
#include <strings.h>

#define MAXLINES 1000
#define MAXTEXT  256 

int dec_config(char *buf, int len) {

  int i;
  char key;
  for (i=0; i<len; i++) {
   key=((i+64)*63)%128;
   buf[i] = buf[i] ^ key;
  }

}

int main(int argc, char **argv) {

  char buf[MAXLINES*MAXTEXT]; 
  int i, cnt; 

  FILE *cin; 

  if (argc != 2) { 
    fprintf(stderr, "Usage: encode_config config_file_name\n"); 
    exit(1); 
  }

  if (( cin = fopen(argv[1], "rb" )) == NULL ) { 
    fprintf(stderr, "Cannot open %s\n", argv[1]); 
    exit(1); 
  }
 
  cnt = fread(buf, 1, MAXLINES*MAXTEXT, cin); 
  fclose(cin); 
  dec_config(buf, cnt); 
  fwrite(buf, 1, cnt, stdout); 

  return 0;
}


