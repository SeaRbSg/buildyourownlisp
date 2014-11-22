CFLAGS=-std=c99 -Wall -ledit -g
all: \
	strings \
	run \
	$(NULL)

prototypes.c: strings.c
	cproto -o prototypes.c strings.c > /dev/null 2>&1

tags: strings.c
	ctags strings.c

strings: prototypes.c strings.c mpc.c
	$(CC) $(CFLAGS) strings.c mpc.c -o strings

run: strings
	./strings

hello: strings
	./strings hello.lispy

clean:
	rm -Rf strings
	rm -Rf prototypes.c
