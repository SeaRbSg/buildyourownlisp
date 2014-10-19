CFLAGS=-std=c99 -Wall -ledit -g
all: \
	conditionals \
	run \
	$(NULL)

conditionals: conditionals.c mpc.c

run: conditionals
	./conditionals

recursive: conditionals
	./conditionals < recursive.lispy

bonus: conditionals
	./conditionals < bonus.lispy

clean:
	rm -Rf conditionals
