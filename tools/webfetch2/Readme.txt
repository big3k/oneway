
2/11/2011

The programs under this directory have to be compiled under msys. 

1. Compile and install libcurl, with only http support, for simplicity. 
   (http://lenkite.blogspot.com/2007/11/quickstart-libcurl-and-mingw-plain-http.html)
   * download curl-7.21.3
   * Open LIBCURL_HOME/lib/setup.h. Add a "#define HTTP_ONLY" (minus quotes
   * ofcourse) just before the first line in the file. This disables all
   * protocols except http. Read LIBCURL_HOME/docs/INSTALL for further
   * information.
   * Open a command shell, change to this directory and use command 
     "mingw32-make mingw32". (Read LIBCURL_HOME/docs/INSTALL for more info on this)


2. To comiple, 

   cd webfetch2
   sh compile.sh

with:

$ cat compile.sh 


gcc -DCURL_STATICLIB -static -o webfetch2 -I../curl-7.21.3/include webfetch2.c
get_config.c threads.c ../curl-7.21.3/lib/libcurl.a -lws2_32 -lwinmm -mwindows

# 
gcc -DCURL_STATICLIB -static -o getinmem -I../curl-7.21.3/include getinmem.c
../curl-7.21.3/lib/libcurl.a -lws2_32 -lwinmm

strip webfetch2.exe

