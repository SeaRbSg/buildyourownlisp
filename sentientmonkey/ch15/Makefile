CFLAGS=-std=c99 -Wall -ledit -g
all: \
	lispy \
	run \
	$(NULL)

prototypes.c: lispy.c
	cproto -o prototypes.c lispy.c > /dev/null 2>&1

tags: lispy.c
	ctags lispy.c

lispy: prototypes.c lispy.c mpc.c
	$(CC) $(CFLAGS) lispy.c mpc.c -o lispy

run: lispy
	./lispy

clean:
	rm -Rf lispy
	rm -Rf prototypes.c
