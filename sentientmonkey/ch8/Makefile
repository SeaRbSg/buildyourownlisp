all: error_handling
clean:
	rm error_handling
error_handling: error_handling.c
	cc -std=c99 -Wall error_handling.c mpc.c -lm -ledit -o error_handling
	./error_handling
