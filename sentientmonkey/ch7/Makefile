all: evaluation
clean:
	rm evaluation
evaluation: evaluation.c
	cc -std=c99 -Wall evaluation.c mpc.c -lm -ledit -o evaluation
	./evaluation
