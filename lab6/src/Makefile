СС=gcc
CFLAF=-std=c99

all: server client

server:
	$(CC) -o server -lpthread $(CFLAF) server.c

client:
	$(CC) -o client -lpthread $(CFLAF) client.c

clean:
	rm server client