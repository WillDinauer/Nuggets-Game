# Makefile for CS50 Nuggets Project
#
# Angus Emmett

MAKE = make
.PHONY: all valgrind clean

############## default: make all libs and programs ##########
all: 
	$(MAKE) -C map
	$(MAKE) -C server


############## clean  ##########
clean:
	rm -f *~
	rm -f TAGS
	$(MAKE) -C map clean
	$(MAKE) -C server clean
