CFLAGS=-std=c99 -Wall -ledit -g
all: \
	q_expressions \
	run \
	$(NULL)

q_expressions: q_expressions.c mpc.c

run: q_expressions
	./q_expressions

clean:
	rm -Rf q_expressions
