#NAME: Sean Langley
#EMAIL: sean.langley22@gmail.com
#ID: 504 661 838

CC=gcc
CFLAGS=-pthread -lmcrypt -g

all:
	$(CC) $(CFLAGS) -o lab1b-client lab1b-client.c
	$(CC) $(CFLAGS) -o lab1b-server lab1b-server.c 
client:
	$(CC) $(CFLAGS) -o lab1b-client lab1b-clinet.c
server:
	$(CC) $(CFLAGS) -o lab1b-server lab1b-server.c
clean:
	rm -rf lab1b-client lab1b-server
dist:
	tar -czf lab1b-504661838.tar.gz *.c README Makefile my.key
