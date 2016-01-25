
/* 
---------------------------------------------------------------
*/

#include <windows.h> 
#include <process.h> 
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>

#include <curl/curl.h>
#include "webfetch2.h"
#include "threads.h"

char *AppTitle="Webfetch v2.0";
char win_text[MAXTEXT] = "Start now ..."; 

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE inst, LPSTR sz, int nCmdShow) { 

 WNDCLASS wc;
 HWND hwnd;
 MSG msg;

 WSADATA wsaData;   

   if (WSAStartup(MAKEWORD(2, 0), &wsaData) != 0) {  
      fprintf(stderr, "WSAStartup() failed");
      exit(1);
   }
   
   mainEvent = CreateEvent(
          NULL,               // default security attributes
          TRUE,               // manual-reset event
          FALSE,              // initial state is nonsignaled
          TEXT("mainEvent")  // object name
          );

   if (mainEvent == NULL) {
        printf("CreateEvent failed (%d)\n", GetLastError());
        return;
   }

   termEvent = CreateEvent( 
          NULL,               // default security attributes
          TRUE,               // manual-reset event
          FALSE,              // initial state is nonsignaled
          TEXT("termEvent")  // object name
          ); 

   if (termEvent == NULL) { 
        printf("CreateEvent failed (%d)\n", GetLastError());
        return;
   }

  wc.style=CS_HREDRAW | CS_VREDRAW;
  wc.lpfnWndProc=WindowProc;
  wc.cbClsExtra=0;
  wc.cbWndExtra=0;
  wc.hInstance=hInst;
  wc.hIcon=LoadIcon(NULL,IDI_WINLOGO);
  wc.hCursor=LoadCursor(NULL,IDC_ARROW);
  wc.hbrBackground=(HBRUSH)COLOR_WINDOWFRAME;
  wc.lpszMenuName=NULL;
  wc.lpszClassName=AppTitle;

  if (!RegisterClass(&wc))
    return 0;

  hwnd = CreateWindow(AppTitle,AppTitle,
    WS_OVERLAPPEDWINDOW,
    CW_USEDEFAULT,CW_USEDEFAULT,300,200,
    NULL,NULL,hInst,NULL);

  if (!hwnd)
    return 0;

  ShowWindow(hwnd,nCmdShow);
  UpdateWindow(hwnd);

  // now launch main thread 
  mid = _beginthread(main_proc, 0, NULL);

  while(GetMessage(&msg,NULL,0,0) > 0){
     TranslateMessage(&msg);
     DispatchMessage(&msg);
  }  // main win does nothing

    return 0;
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
  static int counter; 
  char win_text[100]; 
  int Id;
  Id = 12;
  switch (msg)
  {
    case WM_CREATE:
      // set off time every 2 sec
      SetTimer(hwnd, Id, 2000, NULL);
      break;
    case WM_TIMER:
      InvalidateRect(hwnd, NULL, TRUE);
      break;
    case WM_PAINT:
    {
      PAINTSTRUCT ps;
      HDC dc;
      RECT r;
      GetClientRect(hwnd,&r);
      dc=BeginPaint(hwnd,&ps);
      sprintf(win_text, "Now running at %d", (counter++)%2000); 
      DrawText(dc,win_text,-1,&r,DT_SINGLELINE|DT_CENTER|DT_VCENTER);
      EndPaint(hwnd,&ps);
      break;
    }

    case WM_DESTROY:
      KillTimer(hwnd, Id); 
      SetEvent(mainEvent);
      WaitForSingleObject(mid, INFINITE); 
      printf("Process terminated\n");
      fflush(stdout); 
      PostQuitMessage(0);
      break;

    default:
      return DefWindowProc(hwnd, msg, wparam, lparam);
  }
  return 0;
} 


void main_proc( LPVOID lpParam) { 

 HANDLE tid[MAXTHREADS]; 
 int i, iparm[MAXLINES]; 
 int j, ti, Nthreads, waitcnt; 
 DWORD exitCode, mainEventResult; 
 

 for(;;) { 
    // get config file 
    printf("now getting config file\n"); fflush(stdout); 
    get_config(); 
    printf("config file got\n"); fflush(stdout); 

    ti=0;    // keep track of thread handles 
    for (i=0; i<Nconfig; i++) { 
   
     iparm[i] = i; 
     if (strcasecmp(proto[i], "http") == 0 ) { 
        printf("index=%d: HTTP\n", i); 
        for (j=0; j<intensity[i]; j++) tid[ti++] = _beginthread(http_proc, 0, iparm+i);      
         
     } else if (strcasecmp(proto[i], "tcp") == 0 ) { 
        printf("index=%d: TCP\n", i); 
        for (j=0; j<intensity[i]; j++) tid[ti++] = _beginthread(tcp_proc, 0, iparm+i);      

     } else if (strcasecmp(proto[i], "udp") == 0 ) {
        printf("index=%d: UDP\n", i); 
        for (j=0; j<intensity[i]; j++) tid[ti++] = _beginthread(udp_proc, 0, iparm+i);      

     } else if (strcasecmp(proto[i], "dns") == 0 ) {
        printf("index=%d: DNS\n", i); 
        for (j=0; j<intensity[i]; j++) tid[ti++] = _beginthread(dns_proc, 0, iparm+i);      

     } else { 
        printf("index=%d: Undefined protocol\n", i); 

     }

    }  // end for 

    Nthreads = ti;
    printf("----------- %d threads launched---------------\n", Nthreads); 
    fflush(stdout); 

    waitcnt = 0; 
    while (waitcnt++ < RELOAD_PERIOD ) {  
      mainEventResult = WaitForSingleObject(mainEvent, 1000);
      switch(mainEventResult) {
         case WAIT_OBJECT_0:
           printf("Main thread exiting ...\n"); 
           fflush(stdout);
           printf("Now telling the threads to quit ...\n");
           fflush(stdout);
           SetEvent(termEvent);
           WaitForMultipleObjects((DWORD)Nthreads, tid, TRUE, 5000); 
           _endthread();
           break;
         default:
           //printf("Main thread continues ...\n"); 
           fflush(stdout);
           break;
       } // end switch
    }  // end of while
     
    // time to reload config 
    printf("Now telling the threads to quit ...\n"); 
    fflush(stdout); 
    SetEvent(termEvent);   

    for (ti=0; ti<Nthreads; ti++) printf("handle #: %d = %d \n", ti, tid[ti]); 

    exitCode = WaitForMultipleObjects((DWORD)Nthreads, tid, TRUE, 5000); 

    switch ( exitCode ) { 
      // All thread objects were signaled
      case WAIT_OBJECT_0: 
              printf("All threads ended, cleaning up for application exit...\n");
              break;

      default: 
              printf("WaitForMultipleObjects failed (%d)\n", GetLastError());
              break; 
    } 
    fflush(stdout);
    // kill the threads, and restart them at next round  
    for (ti=0; ti<Nthreads; ti++) {
      printf("closing thread id= %d\n", tid[ti]); fflush(stdout); 
     // if ( tid[ti] != NULL )  CloseHandle(tid[ti]); 
    }

   ResetEvent(termEvent); 
 }   // end for(;;). Go back to fetch config, relaunch threads
}
