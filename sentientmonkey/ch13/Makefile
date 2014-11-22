CFLAGS=-std=c99 -Wall -ledit -g
all: \
	conditionals \
	run \
	$(NULL)

prototypes.c: conditionals.c
	cproto -o prototypes.c conditionals.c > /dev/null 2>&1

tags: conditionals.c
	ctags conditionals.c

conditionals: prototypes.c conditionals.c mpc.c
	$(CC) $(CFLAGS) conditionals.c mpc.c -o conditionals

run: conditionals
	./conditionals

recursive: conditionals
	./conditionals < recursive.lispy

bonus: conditionals
	./conditionals < bonus.lispy

clean:
	rm -Rf conditionals
	rm -Rf prototypes.c
