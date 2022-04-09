all: build

build: main.c
	gcc -g main.c -o snake

run:
	./snake

clean:
	rm snake
