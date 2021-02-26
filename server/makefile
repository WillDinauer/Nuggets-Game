# Makefile for 'map' module
# Angus Emmett - Modified from class notes

M = ../map

CFLAGS = -Wall -pedantic -std=c11 -ggdb -I../map
CC = gcc
PROG = server
OBJS = server.o
LIBS =

.PHONY: all clean

all: server

# executable depends on object files
$(PROG): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) $(LIBS) -o $(PROG)

# object files depend on include files
server.o: $M/map.h



clean:
	rm -f $(PROG)
	rm -f *~ *.o
	rm -rf *.dSYM