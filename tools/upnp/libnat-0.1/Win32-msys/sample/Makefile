
CC = gcc
CFLAGS = -std=c99 -pedantic -Wall -I../libnat

SRCS = forward.c
OBJS = forward.o
LIB = ../libnat/libnat.a ../expat/xmlparse/libexpat.a
PROG = forward.exe

$(PROG): $(SRCS) $(OBJS) $(LIB)
	$(CC) $(CFLAGS) -o $@ $(OBJS) $(LIB) -lws2_32

forward.o: $(SRCS)
	$(CC) $(CFLAGS) -c forward.c

clean:
	rm -rf $(OBJS) $(PROG)
