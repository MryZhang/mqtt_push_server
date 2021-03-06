object = net.o util.o mqtt_packet.o mqtt_handler.o \
		 command_send.o server.o mqtt_timer.o redis_com.o \
		 mqtt_hash.o mqtt_string.o client_ds.o mqtt_message.o log.o
gcc = gcc
CFLAG = -c

VPATH = ./src
INCLUDEPATH = -I /home/aside99/Documents/hiredis-master/

all : $(object)
	$(gcc) $(object) -o server -lpthread -lhiredis
log.o : log.h log.c
	$(gcc) $(CFLAG) ./src/log.c
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
mqtt_hash.o : ./src/mqtt_hash.c ./src/mqtt_hash.h
	$(gcc) $(CFLAG) ./src/mqtt_hash.c $(INCLUDEPATH)
mqtt_string.o : ./src/mqtt_string.c ./src/mqtt_string.h
	$(gcc) $(CFLAG) ./src/mqtt_string.c $(INCLUDEPATH)
client_ds.o : ./src/client_ds.c ./src/client_ds.h
	$(gcc) $(CFLAG) ./src/client_ds.c $(INCLUDEPATH)
mqtt_message.o : ./src/mqtt_message.c
	$(gcc) $(CFLAG) ./src/mqtt_message.c $(INCLUDEPATH)
clear : 
	rm *.o -rf
