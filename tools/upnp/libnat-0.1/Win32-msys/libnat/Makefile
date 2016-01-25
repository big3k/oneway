# $Id: Makefile,v 1.1.1.1 2006/03/13 15:54:53 awarring Exp $ #

CC = gcc
AR = ar
#CFLAGS = -ansi -pedantic -Wall -c
CFLAGS = -std=c99 -pedantic -Wall -DOS_WIN -DWIN32 -c

OBJS = libnat.o upnp.o ssdp.o http.o os_win.o os_common.o utility.o error.o
PROG = libnat.a

all: $(PROG)

$(PROG): $(OBJS)
	$(AR) rc $(PROG) $(OBJS)

libnat.o: libnat.h utility.h error.h
	$(CC) $(CFLAGS) libnat.c

upnp.o: libnat.h http.h ssdp.h utility.h error.h
	$(CC) $(CFLAGS) upnp.c

ssdp.o: ssdp.h os.h utility.h error.h
	$(CC) $(CFLAGS) ssdp.c

http.o: http.h os.h utility.h error.h
	$(CC) $(CFLAGS) http.c

os_common.o: os_common.h os.h utility.h error.h
	$(CC) $(CFLAGS) os_common.c

os_win.o: os_common.h utility.h error.h
	$(CC) $(CFLAGS) os_win.c

utility.o: utility.h error.h
	$(CC) $(CFLAGS) utility.c

error.o: error.h
	$(CC) $(CFLAGS) error.c

clean:
	rm -rf $(OBJS) $(PROG)

