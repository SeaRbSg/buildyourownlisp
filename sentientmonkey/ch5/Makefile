all: doge
clean:
	rm doge json
doge: doge.c
	cc -std=c99 -Wall doge.c mpc.c -lm -ledit -o doge
	./doge
json: json.c
	cc -std=c99 -Wall json.c mpc.c -lm -ledit -o json
	./json
