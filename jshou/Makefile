CC=gcc
CFLAGS=-std=c99 -Wall -pedantic -g -O0
LDFLAGS = -lreadline -lm

all: \
	s_expressions \
	q_expressions \
	variables \
	functions

s_expressions: s_expressions.o mpc.o
	$(CC) $(CFLAGS) -o s_expressions s_expressions.o mpc.o $(LDFLAGS)

q_expressions: q_expressions.o mpc.o
	$(CC) $(CFLAGS) -o q_expressions q_expressions.o mpc.o $(LDFLAGS)

variables: variables.o mpc.o
	$(CC) $(CFLAGS) -o variables variables.o mpc.o $(LDFLAGS)

functions: functions.o mpc.o
	$(CC) $(CFLAGS) -o functions functions.o mpc.o $(LDFLAGS)

.PHONY: clean realclean
clean:
	rm -rf *.dSYM *~

realclean: clean
	rm -f ch?? *.o
