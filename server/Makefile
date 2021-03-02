# Makefile for 'server' module
# William Dinauer, Dartmouth CS50 Winter 2021

L = ../support

PROG = server
LIBS = -lm
LLIBS = $L/support.a

OBJS = server.o ../map/map.o

CFLAGS = -Wall -pedantic -std=c11 -ggdb -I$L -I../map
CC = gcc

$(PROG): $(OBJS)
	$(CC) $(CFLAGS) $^ $(LLIBS) $(LIBS) -o $(PROG)

server.o: $L/hashtable.h $L/set.h $L/counters.h $L/message.h $L/log.h ../map/map.h
map.o: ../map/map.h

.PHONY: clean valgrind test

clean:
	rm -rf *.dSYM  # MacOS debugger info
	rm -f *~ *.o
	rm -f $(PROG)
