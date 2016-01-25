
# non-debug compiling command 
#gcc -c -g -O2 -I/usr/local/include u2u_transform.c 

# debug compiling command 
gcc -DDEBUG -c -g -O2 -I/usr/local/include u2u_transform.c 

gcc -static -g -O2 -I/usr/local/include -o u2u_transform-debug u2u_transform.o /usr/local/lib/libnetfilter_queue.a /usr/local/lib/libnfnetlink.a


gcc  -c -g -O2 -I/usr/local/include u2u_transform.c 
gcc -static -g -O2 -I/usr/local/include -o u2u_transform u2u_transform.o /usr/local/lib/libnetfilter_queue.a /usr/local/lib/libnfnetlink.a

exit 

