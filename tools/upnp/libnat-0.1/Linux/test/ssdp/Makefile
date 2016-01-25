# $Id: Makefile,v 1.1.1.1 2006/03/13 15:54:53 awarring Exp $ #

CC = gcc
CFLAGS = -ansi -pedantic -Wall

OBJS = ../../libnat/ssdp.o ../../libnat/os_unix.o ../../libnat/os_common.o  \
       ../../libnat/utility.o ../../libnat/error.o ssdp_test.o
PROG = ssdp_test.exe

all: $(PROG)

$(PROG): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS)

ssdp_test.o: ../../libnat/ssdp.h ../../libnat/error.h
	$(CC) $(CFLAGS) -c ssdp_test.c

clean:
	rm -rf ssdp_test.o $(PROG)
