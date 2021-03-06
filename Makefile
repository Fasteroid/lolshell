# choose your compiler
CC=gcc
#CC=gcc -Wall

lolshell: sh.o get_path.o main.c 
	$(CC) -g main.c build/sh.o build/get_path.o -o lolshell
#	$(CC) -g main.c build/sh.o build/get_path.o -o lolshell

sh.o: sh.c sh.h
	$(CC) -g -c sh.c -o build/sh.o

get_path.o: get_path.c get_path.h
	$(CC) -g -c get_path.c -o build/get_path.o

clean:
	rm -rf build/* lolshell
