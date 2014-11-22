
CFLAGS=-std=c99 -Wall -pedantic -Werror -g -O0 -ledit

all: \
	ch04 \
	ch05 \
	ch06 \
	ch07 \
	ch08 \
	ch09 \
	ch10 \
	ch11 \
	ch12 \
	ch13 \
	ch14 \
	$(NULL)

ch04: ch04.c

ch05: ch05.c mpc.c

ch06: ch06.c mpc.c

ch07: ch07.c mpc.c

ch08: ch08.c mpc.c

ch09: ch09.c mpc.c

ch10: ch10.c mpc.c

ch11: ch11.c mpc.c

ch12: ch12.c mpc.c

ch13: ch13.c mpc.c

ch14: ch14.c mpc.c

.PHONY: clean realclean
clean:
	rm -rf *.dSYM *~

realclean: clean
	rm -f ch?? *.o

leaks: ch14
	valgrind --leak-check=full ./ch14 leaks.lisp < /dev/null

# depend:
# 	mkdep -p ${CFLAGS} ch12.c mpc.c
# 
# include .depend
