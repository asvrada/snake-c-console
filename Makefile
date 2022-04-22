all: build

build: snake.c
	gcc -g snake.c -o snake

run:
	./snake

clean:
	rm snake
