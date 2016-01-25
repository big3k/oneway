
# sj-enc functions 
g++ -c sj-enc.cpp
# non-debug compiling command 
#g++ -c -g -O2 -I/usr/local/include u2t_transform.c 

# debug compiling command 
g++ -DDEBUG -c -g -O2 -I/usr/local/include u2t_transform.c 

g++ -static -g -O2 -I/usr/local/include -o u2t_transform-debug u2t_transform.o sj-enc.o /usr/local/lib/libnetfilter_queue.a /usr/local/lib/libnfnetlink.a


g++  -c -g -O2 -I/usr/local/include u2t_transform.c 
g++ -static -g -O2 -I/usr/local/include -o u2t_transform u2t_transform.o sj-enc.o /usr/local/lib/libnetfilter_queue.a /usr/local/lib/libnfnetlink.a

exit 

