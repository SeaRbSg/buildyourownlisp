CFLAGS=-std=c99 -Wall -ledit -g
all: \
	s_expressions \
	run \
	$(NULL)

s_expressions: s_expressions.c mpc.c

run: s_expressions
	./s_expressions

clean:
	rm -Rf s_expressions
