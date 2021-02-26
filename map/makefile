# Makefile for 'map' module
# Angus Emmett - Modified from class notes

CFLAGS = -Wall -pedantic -std=c11 -ggdb -I../lib -I../set
CC = gcc
PROG = mapTest
OBJS = mapTest.o
LIBS =

.PHONY: all clean

all: mapTest

# executable depends on object files
$(PROG): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) $(LIBS) -o $(PROG)

# object files depend on include files
mapTest.o: map.h
map.o: map.h 


clean:
	rm -f $(PROG)
	rm -f *~ *.o
	rm -rf *.dSYM