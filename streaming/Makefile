
CC      = gcc

CFLAGS = -O1
CFLAGS += -Wall
CFLAGS += -W -Wstrict-prototypes
#CFLAGS += -D_BSD_SOURCE -D_POSIX_C_SOURCE=200112L

LDFLAGS= $(CFLAGS) -L.

INCLUDE = 
LIBS    =
OBJS    = streaming.o passivesock.o passiveTCP.o

TARGETS = streaming

all:  $(TARGETS)

.c.o:
	$(CC) -c $(CFLAGS) $(INCLUDE) $<

$(TARGETS): $(OBJS)
	$(CC) $(LDFLAGS) $(OBJS) $(LIBS) -o $@

install:

clean:
	rm -f *.o core a.out *~ $(TARGETS) *.exe *.core *.log






