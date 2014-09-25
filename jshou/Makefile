CC=gcc
CFLAGS=-std=c99 -Wall -pedantic -g -O0
LDFLAGS = -lreadline -lm

all: \
	s_expressions \
	q_expressions \
	variables \
	functions

%: %.o mpc.o
	$(CC) $(CFLAGS) -o $@ $*.o mpc.o $(LDFLAGS)

.PHONY: clean realclean
clean:
	rm -rf *.dSYM *~

realclean: clean
	rm -f ch?? *.o
