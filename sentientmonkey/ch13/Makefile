CFLAGS=-std=c99 -Wall -ledit -g
all: \
	conditionals \
	run \
	$(NULL)

conditionals: conditionals.c mpc.c

run: conditionals
	./conditionals

clean:
	rm -Rf conditionals
