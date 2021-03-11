# Makefile for 'server' module
# William Dinauer, Dartmouth CS50 Winter 2021

L = ../support

PROG = server
LIBS = -lm
LLIBS = $L/support.a

OBJS = server.o ../map/map.o serverUtils.o

CFLAGS = -Wall -pedantic -std=c11 -ggdb -I$L -I../map
CC = gcc

$(PROG): $(OBJS)
	$(CC) $(CFLAGS) $^ $(LLIBS) $(LIBS) -o $(PROG)

server.o: $L/hashtable.h $L/set.h $L/counters.h $L/message.h $L/log.h ../map/map.h serverUtils.h
map.o: ../map/map.h
serverUtils.o: serverUtils.h

.PHONY: clean valgrind test

test: $(PROG)
	./$(PROG) 2>server.log ../maps/main.txt

valgrind: $(PROG)
	valgrind --leak-check=full --show-leak-kinds=all ./$(PROG) 2>server.log ../maps/small.txt

clean:
	rm -rf *.dSYM  # MacOS debugger info
	rm -f *~ *.o
	rm -f $(PROG)
