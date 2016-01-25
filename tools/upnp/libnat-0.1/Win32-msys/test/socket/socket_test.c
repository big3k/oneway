/* Copyright (c) 2006 Adam Warrington
** $Id: socket_test.c,v 1.1.1.1 2006/03/13 15:54:53 awarring Exp $
**
** Permission is hereby granted, free of charge, to any person obtaining a copy
** of this software and associated documentation files (the "Software"), to deal
** in the Software without restriction, including without limitation the rights
** to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
** copies of the Software, and to permit persons to whom the Software is
** furnished to do so, subject to the following conditions:
**
** The above copyright notice and this permission notice shall be included in
** all copies or substantial portions of the Software.
**
** THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
** IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
** FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
** AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
** LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
** OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
** SOFTWARE.
**
******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../../libnat/os.h"
#include "../../libnat/error.h"

#define CONNECT_TIMEOUT     2
#define RECEIVE_TIMEOUT     2

int main(int argc, char ** argv)
{
  int ret;
  int amt_sent;
  int amt_recv;
  int size_recv_so_far = 0;
  short int port = 80;
  char host[] = "xanga.com";
  char send_buf[] = "GET /index.html HTTP/1.1\r\nHost: www.xanga.com\r\n\r\n";
  char recv_buf[100001];
  OsSocket* sock;

  if((ret = LNat_Os_Socket_Connect(&sock, host, port, CONNECT_TIMEOUT)) != OK) {
    LNat_Print_Internal_Error(ret);
    exit(EXIT_FAILURE);
  } else {
    printf("CONNECT SUCCESS TO %s AT PORT %d!\n\n", host, port);
  }

  if((ret = LNat_Os_Socket_Send(sock, &send_buf[0], (int)strlen(send_buf), &amt_sent)) != OK) {
    LNat_Print_Internal_Error(ret);
    exit(EXIT_FAILURE);
  } else {
    printf("Send Suceeded sending %d bytes\n\n", amt_sent);
    printf("Sent Buffer:\n%s\n\n", send_buf);
  }

  do {
    ret = LNat_Os_Socket_Recv(sock, &(recv_buf[size_recv_so_far]), 1000, 
                              &amt_recv, RECEIVE_TIMEOUT);
    if(ret == OK) {
      size_recv_so_far += amt_recv;
    }
  } while(ret == OK && amt_recv > 0);
  recv_buf[size_recv_so_far] = '\0';
  printf("Recv Succeeded recieving %d bytes\n\n", size_recv_so_far);
  printf("Recieved Buffer:\n%s\n\n", recv_buf);

  if((ret = LNat_Os_Socket_Close(&sock)) != OK) {
    LNat_Print_Internal_Error(ret);
    exit(EXIT_FAILURE);
  } else {
    printf("CLOSE SOCKET SUCCESS!\n\n");
  }

  exit(EXIT_SUCCESS);
}
