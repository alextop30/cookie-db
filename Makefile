CC = g++
CFLAGS = -Wall -g -std=c++11

all : load.o query.o load.out query.out

clean:
	rm -rf *o
	rm load
	rm query

.PHONY : load.o
load.o :
	${CC} ${CFLAGS} -c load.cc 

load.out : load.o
	${CC} ${CFLAGS} load.o -o load -L/usr/include/postgresql -lpq

.PHONY: query.o	
query.o : 
	${CC} ${CFLAGS} -c query.cc 
	
query.out : query.o
	${CC} ${CFLAGS} query.o -o query -L/usr/include/postgresql -lpq

