# Makefile for 'map' module
# Angus Emmett - Modified from class notes

S = ../support

CFLAGS = -Wall -pedantic -std=c11 -ggdb -I$S
CC = gcc
PROG = mapTest
OBJS = mapTest.o map.o
LIBS =

.PHONY: all clean test

all: mapTest

# executable depends on object files
$(PROG): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) $(LIBS) -o $(PROG)

# object files depend on include files
mapTest.o: map.h
map.o: map.h


test: $(PROG)
	./$(PROG)


clean:
	rm -f $(PROG)
	rm -f *~ *.o
	rm -rf *.dSYM