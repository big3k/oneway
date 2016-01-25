

gcc -g -DCURL_STATICLIB -static -o webfetch2 -I../curl-7.21.3/include webfetch2.c get_config.c threads.c ../curl-7.21.3/lib/libcurl.a -lws2_32 -lwinmm -mwindows

# 
gcc -DCURL_STATICLIB -static -o getinmem -I../curl-7.21.3/include getinmem.c ../curl-7.21.3/lib/libcurl.a -lws2_32 -lwinmm

strip webfetch2.exe
