object = net.o util.o mqtt_packet.o mqtt_handler.o \
		 command_send.o server.o mqtt_timer.o redis_com.o
gcc = gcc
CFLAG = -c

VPATH = ./src
INCLUDEPATH = -I /home/aside99/Documents/hiredis-master/

all : $(object)
	$(gcc) $(object) -o server -lpthread -lhiredis
net.o : net.h net.c
	$(gcc) $(CFLAG) ./src/net.c $(INCLUDEPATH)
util.o : util.h util.c
	$(gcc) $(CFLAG) ./src/util.c $(INCLUDEPATH)
mqtt_packet.o : mqtt_packet.c mqtt_packet.h net.h server.h
	$(gcc) $(CFLAG) ./src/mqtt_packet.c $(INCLUDEPATH)
mqtt_handler.o : mqtt_handler.c mqtt_handler.h mqtt_packet.h server.h
	$(gcc) $(CFLAG) ./src/mqtt_handler.c $(INCLUDEPATH)
server.o : server.c server.h
	$(gcc) $(CFLAG) ./src/server.c $(INCLUDEPATH)
command_send.o : command_send.c command_send.h
	$(gcc) $(CFLAG) ./src/command_send.c $(INCLUDEPATH)
mqtt_timer.o : ./src/mqtt_timer.c ./src/mqtt_timer.h
	$(gcc) $(CFLAG) ./src/mqtt_timer.c $(INCLUDEPATH)
redis_com.o : ./src/redis_com.c ./src/redis_com.h
	$(gcc) $(CFLAG) ./src/redis_com.c $(INCLUDEPATH)
clear : 
	rm *.o -rf
