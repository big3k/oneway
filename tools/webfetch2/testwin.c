
#include <windows.h>
#include <stdio.h>
#include <process.h>

char win_text[256]="Starting now"; 

char *AppTitle="Webfetch2";
LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE inst, LPSTR sz, int nCmdShow)
{
  int counter=0; 
  WNDCLASS wc;
  HWND hwnd;
  MSG msg;
  DWORD exitCode; 

    STARTUPINFO si;
    PROCESS_INFORMATION pi;

    ZeroMemory( &si, sizeof(si) );
    si.cb = sizeof(si);
    ZeroMemory( &pi, sizeof(pi) );

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

 if ( strlen(sz) == 0 ) {  // parent process 
  for(;;) {  // keep process running  
    // Start the child process. 
    if( !CreateProcess( NULL,   // No module name (use command line)
        "testwin.exe 1",  //  "webfetch2.exe",        
        NULL,           // Process handle not inheritable
        NULL,           // Thread handle not inheritable
        FALSE,          // Set handle inheritance to FALSE
        0,              // No creation flags
        NULL,           // Use parent's environment block
        NULL,           // Use parent's starting directory 
        &si,            // Pointer to STARTUPINFO structure
        &pi )           // Pointer to PROCESS_INFORMATION structure
    ) 
    {
        printf( "CreateProcess failed (%d).\n", GetLastError() );
        return;
    }

  printf("Process launched \n"); 
  exitCode = WAIT_TIMEOUT; 
  while (exitCode == WAIT_TIMEOUT) {   // keep waiting 
   sprintf(win_text, "Now at %d", counter++); 
   if (PeekMessage(&msg,NULL,0,0,1) > 0){ 
     printf("got message\n"); 
     fflush(stdout);
     TranslateMessage(&msg);
     DispatchMessage(&msg);
   }
    // Wait until child process exits.
   exitCode = WaitForSingleObject( pi.hProcess, 500);  
  }  // end while

  printf("Process exited. Relaunch ... \n"); 
  // Close process and thread handles. 
  CloseHandle( pi.hProcess );
  CloseHandle( pi.hThread );

 }  // end for(;;;) 
}  else { // end if (sz == NULL )

  while(GetMessage(&msg,NULL,0,0) > 0){
     TranslateMessage(&msg);
     DispatchMessage(&msg);
  }  // main win does nothing

}  // end if 

}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
  int Id;  
  //Id = GetCurrentProcessId(); 
  Id = 1; 
  switch (msg)
  {
    
    case WM_CREATE:
      // set off time every 2 sec
      SetTimer(hwnd, Id, 2000, NULL);  
      printf("Timer started \n"); 
      fflush(stdout); 
      break; 
    case WM_TIMER: 
      printf("Timer set off \n"); 
      fflush(stdout); 
      InvalidateRect(hwnd, NULL, TRUE); 
      break; 
    case WM_PAINT:
    {
      PAINTSTRUCT ps;
      HDC dc;
      RECT r;
      printf("WM_PAINT message recv'd \n"); 
      fflush(stdout); 
      GetClientRect(hwnd,&r);
      dc=BeginPaint(hwnd,&ps);
      DrawText(dc,win_text,-1,&r,DT_SINGLELINE|DT_CENTER|DT_VCENTER);
      EndPaint(hwnd,&ps);
      break;
    }

    case WM_DESTROY:
      KillTimer(hwnd, Id);
      PostQuitMessage(0);
      break;

    default:
      return DefWindowProc(hwnd, msg, wparam, lparam);
  }
  return 0;
} 
