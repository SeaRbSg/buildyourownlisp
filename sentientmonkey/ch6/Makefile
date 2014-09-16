all: parsing
clean:
	rm parsing
parsing: parsing.c
	cc -std=c99 -Wall parsing.c mpc.c -lm -ledit -o parsing
	./parsing
