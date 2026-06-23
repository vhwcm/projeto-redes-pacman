CC = gcc
CFLAGS = -Wall -Wextra

PROG = Pacman_game

$(PROG): Client_main.o Client_socket.o Pacman.o
	$(CC) $(CFLAGS) -o $(PROG) Client_main.o Client_socket.o Pacman.o

Client_main.o: Client_main.c Client_socket.h Pacman.h 
	$(CC) $(CFLAGS) -c Client_main.c

Client_socket.o: Client_socket.c Client_socket.h
	$(CC) $(CFLAGS) -c Client_socket.c

Pacman.o: Pacman.c Pacman.h
	$(CC) $(CFLAGS) -c Pacman.c

clean:
	rm -f *.o $(PROG)