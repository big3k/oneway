# $Id: Makefile,v 1.2 2006/03/13 23:47:25 awarring Exp $ #

CC = gcc
AR = ar
CFLAGS = -ansi -pedantic -Wall -W

#expat directory#
ED = expat/
EXPAT-LIB = $(ED)xmlparse/libexpat.a

#libnat directory, objects, and output#
LD = libnat/
LIBNAT-OBJS = $(LD)libnat.o $(LD)upnp.o $(LD)ssdp.o $(LD)http.o   \
              $(LD)os_unix.o $(LD)os_common.o $(LD)utility.o $(LD)error.o
LIBNAT-OUT = $(LD)libnat.a

#sample directory, objects, and outputs#
SD = sample/
SAMPLE-OBJS = $(SD)sample.o
SAMPLE-LIBS = $(LIBNAT-OUT) $(EXPAT-LIB)
SAMPLE-OUT = $(SD)sample.exe

#test directory, objects, and outputs
TD = test/

SOCKET-OBJS = $(TD)socket/socket_test.o $(LD)os_unix.o $(LD)os_common.o \
              $(LD)utility.o $(LD)error.o
SOCKET-OUT = $(TD)socket/socket_test.exe

SSDP-OBJS = $(TD)ssdp/ssdp_test.o $(LD)os_unix.o $(LD)os_common.o \
            $(LD)utility.o $(LD)error.o $(LD)ssdp.o
SSDP-OUT = $(TD)ssdp/ssdp_test.exe

HTTP-OBJS = $(TD)http/http_test.o $(LD)os_unix.o $(LD)os_common.o \
            $(LD)utility.o $(LD)error.o $(LD)http.o
HTTP-OUT = $(TD)http/http_test.exe

all: $(LIBNAT-OUT) $(SAMPLE-OUT) $(SOCKET-OUT) $(SSDP-OUT) $(HTTP-OUT)

#libnat build#
$(LIBNAT-OUT): $(LIBNAT-OBJS) $(EXPAT-LIB)
	$(AR) rc $(LIBNAT-OUT) $(LIBNAT-OBJS)
	
$(EXPAT-LIB):
	cd $(ED); $(MAKE); cd ..;
	
$(LD)libnat.o: $(LD)libnat.h $(LD)utility.h $(LD)error.h
	$(CC) $(CFLAGS) $(LD)libnat.c -o $@ -c
	
$(LD)upnp.o: $(LD)libnat.h $(LD)http.h $(LD)ssdp.h $(LD)utility.h $(LD)error.h
	$(CC) $(CFLAGS) $(LD)upnp.c -o $@ -c
	
$(LD)ssdp.o: $(LD)ssdp.h $(LD)os.h $(LD)utility.h $(LD)error.h
	$(CC) $(CFLAGS) $(LD)ssdp.c -o $@ -c
	
$(LD)http.o: $(LD)http.h $(LD)os.h $(LD)utility.h $(LD)error.h
	$(CC) $(CFLAGS) $(LD)http.c -o $@ -c
	
$(LD)os_common.o: $(LD)os_common.h $(LD)os.h $(LD)utility.h $(LD)error.h
	$(CC) $(CFLAGS) $(LD)os_common.c -o $@ -c
	
$(LD)os_unix.o: $(LD)os_common.h $(LD)utility.h $(LD)error.h
	$(CC) $(CFLAGS) $(LD)os_unix.c -o $@ -c

$(LD)utility.o: $(LD)utility.h $(LD)error.h
	$(CC) $(CFLAGS) $(LD)utility.c -o $@ -c

$(LD)error.o: $(LD)error.h
	$(CC) $(CFLAGS) $(LD)error.c -o $@ -c
	
#sample build#
$(SAMPLE-OUT): $(SAMPLE-OBJS) $(SAMPLE-LIBS)
	$(CC) $(CFLAGS) -o $(SAMPLE-OUT) $(SAMPLE-OBJS) $(SAMPLE-LIBS)
	
$(SD)sample.o: $(LD)libnat.h
	$(CC) $(CFLAGS) -c $(SD)sample.c -o $@
	
#build the test cases#
$(SOCKET-OUT): $(SOCKET-OBJS)
	$(CC) $(CFLAGS) -o $(SOCKET-OUT) $(SOCKET-OBJS)
	
$(TD)/socket/socket_test.o: $(LD)os.h $(LD)error.h
	$(CC) $(CFLAGS) -c $(TD)socket/socket_test.c -o $@
	
$(SSDP-OUT): $(SSDP-OBJS)
	$(CC) $(CFLAGS) -o $(SSDP-OUT) $(SSDP-OBJS)
	
$(TD)/ssdp/ssdp_test.o: $(LD)ssdp.h $(LD)error.h
	$(CC) $(CFLAGS) -c $(TD)ssdp/ssdp_test.c -o $@
	
$(HTTP-OUT): $(HTTP-OBJS)
	$(CC) $(CFLAGS) -o $(HTTP-OUT) $(HTTP-OBJS)
	
$(TD)/http/http_test.o: $(LD)http.h $(LD)error.h
	$(CC) $(CFLAGS) -c $(TD)http/http_test.c -o $@
	

#remove object and exe files from all the build dirs#
clean:
	rm -rf $(LIBNAT-OBJS) $(LIBNAT-OUT); \
	rm -rf $(SAMPLE-OBJS) $(SAMPLE-OUT); \
	rm -rf $(SOCKET-OBJS) $(SOCKET-OUT); \
	rm -rf $(SSDP-OBJS) $(SSDP-OUT); \
	rm -rf $(HTTP-OBJS) $(HTTP-OUT); \
	cd expat; make clean; cd ..;
	

