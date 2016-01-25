/* Copyright (c) 2006 Adam Warrington
** $Id: http_test.c,v 1.1.1.1 2006/03/13 15:54:53 awarring Exp $
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
#include "../../libnat/http.h"
#include "../../libnat/error.h"

int main(int argc, char ** argv)
{
  int ret = 0;
  GetMessage * gm;
  PostMessage * pm;
  char * response;
  int response_code;

  /* test get */
  if((ret = LNat_Generate_Http_Get("slashdot.org", "", 80, &gm)) != OK) {
    LNat_Print_Internal_Error(ret);
    exit(EXIT_FAILURE);
  }

  if((ret = LNat_Http_Request_Get(gm, &response, &response_code)) != OK) {
    (void)LNat_Destroy_Http_Get(&gm);
    LNat_Print_Internal_Error(ret);
    exit(EXIT_FAILURE);
  }

  printf("Response:\n%s\n\n", response);
  free(response);

  if((ret = LNat_Destroy_Http_Get(&gm)) != OK) {
    LNat_Print_Internal_Error(ret);
    exit(EXIT_FAILURE);
  }
  /* end test get */

  /* now test post */
  /* generate an http_post request */
  ret = LNat_Generate_Http_Post("slashdot.org", "", 80, "somepoststuff", &pm);
  if(OK != ret) {
    LNat_Print_Internal_Error(ret);
    return ret;
  }

  /* add some entity headers, start with content length */
  ret = LNat_Http_Post_Add_Entity_Header(pm, "Content-Length", "13");
  if(OK != ret) {
    LNat_Destroy_Http_Post(&pm);
    LNat_Print_Internal_Error(ret);
    return ret;
  }

  if((ret = LNat_Http_Request_Post(pm, &response, &response_code)) != OK) {
    (void)LNat_Destroy_Http_Post(&pm);
    LNat_Print_Internal_Error(ret);
    exit(EXIT_FAILURE);
  }
  printf(response);

  (void)LNat_Destroy_Http_Post(&pm);
  free(response);
  /* end test post */

  exit(EXIT_SUCCESS);
}
