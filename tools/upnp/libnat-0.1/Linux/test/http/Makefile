# $Id: Makefile,v 1.1.1.1 2006/03/13 15:54:53 awarring Exp $ #

CC = gcc
CFLAGS = -ansi -pedantic -Wall

OBJS = ../../libnat/http.o ../../libnat/os_unix.o ../../libnat/os_common.o  \
       ../../libnat/utility.o ../../libnat/error.o http_test.o
PROG = http_test.exe

all: $(PROG)

$(PROG): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS)

http_test.o: ../../libnat/http.h ../../libnat/error.h
	$(CC) $(CFLAGS) -c http_test.c

clean:
	rm -rf http_test.o $(PROG)
