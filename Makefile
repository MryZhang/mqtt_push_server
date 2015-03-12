main : server.o
	gcc server.o -o server -lpthread
server.o : ./src/server.c
	gcc ./src/server.c -c

clear :
	rm *.o -rf
