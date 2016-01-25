/*
 *  $Id: read2f.c,v 1.2 2002/11/01 19:55:31 yudong Exp yudong $
 *
 *  Build a DNS type=any query packet
 *
 */

#include <libnet.h>
#define MAXLINES 10000

int main(int argc, char *argv[])
{
u_char a=18; 
u_short b = 3; 
  b= (b << 8) | a; 

 printf("%ld %ld %ld %ld\n", a, b, b & 0x00ff, b >> 8);  
}
