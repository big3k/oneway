
# Compiler/Linker
CC = gcc
LD = gcc

# compiler/linker flags
#CFLAGS = -g -Wall -I/usr/local/include -DDEBUG
CFLAGS = -g -Wall -I/usr/local/include
LDFLAGS = -static -g -O2

# files removal
RM = /bin/rm -f

# library to use when linking the main program
LIBS = /usr/local/lib/libnetfilter_queue.a /usr/local/lib/libnfnetlink.a -lpthread

# program's object files
PROG_OBJS = delayer.o sender_threads.o queue.o

PROG_INCS = queue.h  sender_threads.h

# program's executable
PROG = delayer

# top-level rule
#all: $(PROG)

$(PROG): $(PROG_OBJS)
	$(LD) $(LDFLAGS) $(PROG_OBJS) $(LIBS) -o $(PROG)

# compile C source files into object files.
%.o: %.c $(PROG_INCS) 
	$(CC) $(CFLAGS) -c $<

# clean everything
clean:
	$(RM) $(PROG_OBJS) $(PROG)


