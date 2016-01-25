
/*
** This  file shows sample usage of LibNat's UPnP functions.
*/

#include <stdlib.h>
#include <stdio.h>
#include "libnat.h"


int main(int argc, char* argv[])
{
  int ret, port;
  UpnpController * controller;
  char public_ip[32];

  if(argc != 2) {
    printf("Usage: forward port-number\n"); 
    exit(-1); 
  }
  
  port = atoi(argv[1]); 
  
  if((ret = LNat_Upnp_Discover(&controller)) != 0) {
    LNat_Print_Error(ret);
    exit(EXIT_FAILURE);
  }
  printf("Discovery Successful.\n");

  if((ret = LNat_Upnp_Get_Public_Ip(controller, public_ip, 32)) != 0) {
    LNat_Print_Error(ret);
    LNat_Upnp_Controller_Free(&controller);
    exit(EXIT_FAILURE);
  }
  printf("Public IP: %s.\n", public_ip);

/* forwarding for other than local machine 
  if((ret = LNat_Upnp_Set_Port_Mapping(controller, "192.168.1.4", port, "TCP")) != 0) {
*/
   if((ret = LNat_Upnp_Set_Port_Mapping(controller, NULL, port, "TCP")) != 0) {
    LNat_Print_Error(ret);
    LNat_Upnp_Controller_Free(&controller);
    free(public_ip);
    exit(EXIT_FAILURE);
  }
  printf("Port Mapping Successful On Port %d.\n", port);

  printf("Press Enter to Remove Port Mapping On Port %d.\n", port);
  getchar(); 
  if((ret = LNat_Upnp_Remove_Port_Mapping(controller, port, "TCP")) != 0) {
    LNat_Print_Error(ret);
    LNat_Upnp_Controller_Free(&controller);
    free(public_ip);
    exit(EXIT_FAILURE);
  }
  printf("Port Mapping Removal Successful On Port %d.\n", port);

  /* destroy the UpnpController object */
  LNat_Upnp_Controller_Free(&controller);

  printf("Everything Successful, Exiting...\n");
  return 0;
}

