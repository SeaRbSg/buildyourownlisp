CFLAGS=-std=c99 -Wall -ledit -g
all: \
	functions \
	run \
	$(NULL)

functions: functions.c mpc.c

run: functions
	./functions

clean:
	rm -Rf functions
