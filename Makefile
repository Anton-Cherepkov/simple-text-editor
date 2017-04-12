CC=gcc
FLAGS=-std=c89 -Wall -ansi -pedantic

.PHONY: all
all: editor

editor: main.c parser.o line.o instance.o handlers.o help.o
	$(CC) $(FLAGS) main.c parser.o line.o instance.o handlers.o help.o -o editor

handlers.o: handlers.c handlers.h
	$(CC) -c $(FLAGS) handlers.c

help.o: help.c help.h
	$(CC) -c $(FLAGS) help.c

instance.o: instance.c instance.h
	$(CC) -c $(FLAGS) instance.c

line.o: line.c line.h
	$(CC) -c $(FLAGS) line.c

parser.o: parser.c parser.h
	$(CC) -c $(FLAGS) parser.c

.PHONY: clean cleanobj
clean: cleanobj
	rm -f editor

cleanobj:
	rm -f *.o
