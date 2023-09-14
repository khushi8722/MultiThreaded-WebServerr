
CC=g++

all: MultiThreadServer.cpp
	$(CC) -pthread -o Webserver MultiThreadServer.cpp

clean:
	$(RM) Webserver
