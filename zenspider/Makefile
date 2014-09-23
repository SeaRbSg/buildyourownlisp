
CFLAGS=-std=c99 -Wall -pedantic -g -O0 -ledit

all: \
	ch04 \
	ch05 \
	ch06 \
	ch07 \
	ch08 \
	ch09 \
	$(NULL)

ch04: ch04.c

ch05: ch05.c mpc.c

ch06: ch06.c mpc.c

ch07: ch07.c mpc.c

ch08: ch08.c mpc.c

ch09: ch09.c mpc.c

.PHONY: clean realclean
clean:
	rm -rf *.dSYM *~

realclean: clean
	rm -f ch?? *.o
