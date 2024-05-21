# Makefile
main: main.o
	gcc -g -o main main.o -lrt -lpthread -Wall
main.o:
	gcc -c main.c
clean:
	rm -rf main main.o