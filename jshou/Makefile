CC=gcc
CFLAGS=-std=c99 -Wall -pedantic -g -O0
LDLIBS = -lreadline -lm

all: \
	s_expressions \
	q_expressions \
	variables \
	functions \
	$(NULL)

%: %.c mpc.c
	$(CC) $(CFLAGS) -o $@ $*.c mpc.c $(LDLIBS)

.PHONY: clean realclean
clean:
	rm -rf *.dSYM *~

realclean: clean
	rm -f ch?? *.o
