CFLAGS=-std=c99 -Wall -pedantic -g -O0 -lreadline

all: \
	s_expressions \
	q_expressions \
	$(NULL)

s_expressions: s_expressions.c mpc.c
q_expressions: q_expressions.c mpc.c

.PHONY: clean realclean
clean:
	rm -rf *.dSYM *~

realclean: clean
	rm -f ch?? *.o
