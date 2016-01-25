# $Id: Makefile,v 1.1.1.1 2006/03/13 15:54:53 awarring Exp $ #

CC = gcc
CFLAGS = -ansi -pedantic -Wall -I ../libnat

SRCS = forward.c
OBJS = forward.o
LIB = ../libnat/libnat.a ../expat/xmlparse/libexpat.a
PROG = forward

$(PROG): $(OBJS) $(LIB)
	$(CC) $(CFLAGS) -o $@ $(OBJS) $(LIB)

forward.o: $(SRCS) ../libnat/libnat.h
	$(CC) $(CFLAGS) -c forward.c

clean:
	rm -rf $(OBJS) $(PROG)
