# Check the following :

CC = gcc

#CFLAGS = -O1 -g -DDEBUG2
CFLAGS = -O1
CFLAGS += -Wall
CFLAGS += -W -Wstrict-prototypes
#CFLAGS += -D_BSD_SOURCE -D_POSIX_C_SOURCE=200112L
#CFLAGS += -DLINUX

LIBS    = -lpthread
INCLUDE = 

LDFLAGS= $(CFLAGS) -L.

LIBOBJS = kam.o vaap264.o http_lib.o

TARGETS = rec

all: $(TARGETS)

.c.o:
	$(CC) -c $(CFLAGS) $(INCLUDE) $<

$(TARGETS): $(LIBOBJS)
	$(CC) $(LDFLAGS) $(LIBOBJS) $(LIBS) -o $@

clean: 
	rm -f $(TARGETS)
	rm -f *.tgz
	rm -f *.o
	rm -f *~
	rm -f #*
	rm -f core
