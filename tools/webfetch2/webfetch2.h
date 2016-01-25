
#define MAXTHREADS 2000
#define MAXLINES 1000
#define MAXTEXT  256
#define RELOAD_PERIOD   7200    // how often to reload config file, in sec

// global data for threads to access

  char proto[MAXLINES][MAXTEXT];
  char ipstr[MAXLINES][MAXTEXT];
  int  port[MAXLINES];
  int  intensity[MAXLINES];
  int  Nconfig;  


struct MemoryStruct {
  char *memory;
  size_t size;
};

static size_t WriteMemoryCallback(void *ptr, size_t size, size_t nmemb, void *data); 

int get_config(void);  
int dec_config(char *buf, int len);   // same for both decoding and encoding

LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);


