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

hash_table_test: hash_table.c hash_table_test.c
	$(CC) $(CFLAGS) hash_table.c hash_table_test.c -o hash_table_test

run: lispy
	./lispy

test: lispy
	./lispy tests.lispy

debug: lispy
	lldb lispy

clean:
	rm -Rf lispy
	rm -Rf prototypes.c
