/* Copyright (c) 2006 Adam Warrington
** $Id: ssdp_test.c,v 1.1.1.1 2006/03/13 15:54:53 awarring Exp $
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
#include "../../libnat/ssdp.h"
#include "../../libnat/error.h"

int main(int argc, char ** argv)
{
  int ret = 0;
  char * response;

  if((ret = LNat_Ssdp_Discover("urn:schemas-upnp-org:service:WANIPConnection:1", &response)) != OK) {
    LNat_Print_Internal_Error(ret);
    exit(EXIT_FAILURE);
  }
  /*(void)printf("SSDP Response:\n%s\n\n", response);*/
  (void)printf(response);

  exit(EXIT_SUCCESS);
}
