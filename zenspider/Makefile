
CFLAGS=-std=c99 -Wall -pedantic -g -O0 -ledit

all: ch04 ch05

ch04: ch04.c

ch05: ch05.c mpc.c

.PHONY: clean realclean
clean:
	rm -rf *.dSYM *~

realclean: clean
	rm ch04
