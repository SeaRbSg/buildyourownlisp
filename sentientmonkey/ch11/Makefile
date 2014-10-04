CFLAGS=-std=c99 -Wall -ledit -g
all: \
	variables \
	run \
	$(NULL)

variables: variables.c mpc.c

run: variables
	./variables

clean:
	rm -Rf variables
