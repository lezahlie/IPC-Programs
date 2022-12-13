CC=gcc	
CFLAGS=-Wall -g	
ALL_CFLAGS= -pthread $(CFLAGS) # enables threading support 

all: war_pipes war_sockets

war_pipes: war_pipes.c 	
	$(CC) $(CFLAGS) war_pipes.c -o war_pipes
war_sockets: war_sockets.c 	
	$(CC) $(ALL_CFLAGS) war_sockets.c -o war_sockets
clean:
	rm -f *.o war_sockets war_pipes