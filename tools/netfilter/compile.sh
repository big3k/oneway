
# non-debug compiling command 
#gcc -c -g -O2 -I/usr/local/include tcp_transform.c 

# debug compiling command 
gcc -DDEBUG -c -g -O2 -I/usr/local/include tcp_transform.c 

gcc -static -g -O2 -I/usr/local/include -o tcp_transform-debug tcp_transform.o /usr/local/lib/libnetfilter_queue.a /usr/local/lib/libnfnetlink.a


gcc  -c -g -O2 -I/usr/local/include tcp_transform.c 
gcc -static -g -O2 -I/usr/local/include -o tcp_transform tcp_transform.o /usr/local/lib/libnetfilter_queue.a /usr/local/lib/libnfnetlink.a

exit 

