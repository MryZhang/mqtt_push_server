main : server.o net.o
	gcc server.o net.o -o server -lpthread
server.o : ./src/server.c
	gcc ./src/server.c -c
net.o : ./src/net.c
	gcc ./src/net.c -c
command_send.o : ./src/command_send.c
	gcc ./src/command_send.c -c
clear :
	rm *.o -rf
