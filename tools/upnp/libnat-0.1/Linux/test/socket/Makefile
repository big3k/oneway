# $Id: Makefile,v 1.1.1.1 2006/03/13 15:54:53 awarring Exp $ #

CC = gcc
CFLAGS = -ansi -pedantic -Wall

OBJS = ../../libnat/os_unix.o ../../libnat/os_common.o ../../libnat/error.o socket_test.o
PROG = socket_test.exe

all: $(PROG)

$(PROG): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS)

socket_test.o: ../../libnat/os.h ../../libnat/error.h
	$(CC) $(CFLAGS) -c socket_test.c

clean:
	rm -rf socket_test.o $(PROG)
