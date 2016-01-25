
# sj-enc functions 
g++ -Wall -c sj-enc.cpp
# non-debug compiling command 
#g++ -c -g -O2 -I/usr/local/include u2u_transform.c 

# debug compiling command 
g++ -Wall -DDEBUG -c -g -O2 -I/usr/local/include u2u_transform.c 
g++ -Wall -DDEBUG -c -g -O2 -I/usr/local/include u2u_transform_raw.c 

g++ -Wall -static -g -O2 -I/usr/local/include -o u2u_transform-debug u2u_transform.o sj-enc.o /usr/local/lib/libnetfilter_queue.a /usr/local/lib/libnfnetlink.a /usr/lib64/libcrypto.a -ldl

g++ -static -g -O2 -I/usr/local/include -o u2u_transform_raw-debug u2u_transform_raw.o sj-enc.o /usr/local/lib/libnetfilter_queue.a /usr/local/lib/libnfnetlink.a /usr/lib64/libcrypto.a -ldl

g++  -Wall -c -g -O2 -I/usr/local/include u2u_transform_raw.c 
g++ -static -g -O2 -I/usr/local/include -o u2u_transform_raw u2u_transform_raw.o sj-enc.o /usr/local/lib/libnetfilter_queue.a /usr/local/lib/libnfnetlink.a /usr/lib64/libcrypto.a -ldl

exit 

