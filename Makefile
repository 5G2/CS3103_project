CC=gcc
CFLAGS=-g -lm

problem3 : problem3.o helpers.o
	$(CC) -o problem3 helpers.o problem3.o -pthread

.PHONY : clean

clean : 
	rm *.o $(objects) problem3