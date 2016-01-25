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

    FILE *dmf, *nsf;  
    char *dms[MAXLINES], *nss[MAXLINES]; 
    int pay_s[MAXLINES]; 
    char *dmfn = "dm.txt";
    char *nsfn = "ns.txt";
    char *paybuf, *pp; 
    int jd = 0; 
    int jn = 0; 
    int kd, kn; 
    u_char bt;
    char lb[80]; 
    char linebuf[80]; 
    char payld[100], tpayld[100]; 

    if( (dmf = fopen(dmfn, "r")) == NULL) {
      fprintf(stderr, "Error opening file %s \n", dmfn); 
      exit(1);
    } /* end if */ 
	
    if( (nsf = fopen(nsfn, "r")) == NULL) {
      fprintf(stderr, "Error opening file %s \n", nsfn); 
      exit(1);
    } /* end if */ 
	
    printf("Reading domain names ...\n");
    while ((jd < MAXLINES) && (fgets(lb, 80, dmf)!=0) )  {
       kd = 0; 
       kn = 0;
       while (lb[kd] != '\0') {        /* get rid of space and CR */
        if ((lb[kd] != ' ') && (lb[kd] != '\n')) {
           linebuf[kn] = lb[kd];
           kn++;
        }  /* end if */
        kd++; 
       } /* end while */
       linebuf[kn] = '\0'; 
       paybuf = linebuf;
       if (kn != 0) { /* not empty line */ 
         /* convert domain name into DNS query message payload */
         *payld = '\0'; 
         while ( (pp = strchr(paybuf, '.') )!=NULL ) { 
            *pp++ = '\0';
            sprintf(tpayld, "%c%s", strlen(paybuf), paybuf); 
            strcat(payld, tpayld); 
            paybuf = pp; 
         }  /* end while */ 
            sprintf(tpayld, "%c%s", strlen(paybuf), paybuf); 
            strcat(payld, tpayld); 
          
         dms[jd] = (char *)malloc(strlen(payld)+1);  /* alloc mem */
         strcpy(dms[jd], payld); 
         pay_s[jd] = strlen(dms[jd]) + 1; 
         *(dms[jd] + pay_s[jd]) = '\x00'; 
         *(dms[jd] + pay_s[jd]+1) = '\xff';  /* query type=any */
         *(dms[jd] + pay_s[jd]+2) = '\x00'; 
         *(dms[jd] + pay_s[jd]+3) = '\x01'; 
         pay_s[jd] += 4; 
         for (kn=0; kn < pay_s[jd]; kn++) { 
            bt = *(dms[jd] + kn); 
            printf(" %02X", bt); 
         } 
         printf("\n");
     
         jd++;
       } /* end if  */
    } /* end while  */

    printf("Total domains: %d\n", jd); 
    fclose(dmf);
   
    printf("Reading resolver IPs ... \n");
    while ((jn < MAXLINES) && (fgets(lb, 80, nsf)!=0) )  {
       kd = 0; 
       kn = 0;
       while (lb[kd] != '\0') {        /* get rid of space and CR */
        if ((lb[kd] != ' ') && (lb[kd] != '\n')) {
           linebuf[kn] = lb[kd];
           kn++;
        }  /* end if */
        kd++; 
       } /* end while */
       linebuf[kn] = '\0'; 
       if (kn != 0) { /* not empty line */ 
         nss[jn] = (char *)malloc(strlen(linebuf)+1); 
         strcpy(nss[jn++], linebuf); 
         printf("%s-\n", nss[jn-1]);
       } /* end if */   
    } /* end while  */
   
    printf("Total DNS IPs: %d\n", jn); 
    fclose(nsf);
    
} /* end of main */
