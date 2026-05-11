CFLAGS = -Wall 
LDFLAGS = 
BIN = Client
SOURCE = Client_main.c Client_socket.c Pacman.c  

CC = gcc

AS: 
	$(CC) $(SOURCE) $(CFLAGS) -o $(BIN) $$(pkg-config allegro_ttf-5 allegro-5 allegro_main-5 allegro_font-5 allegro_primitives-5 allegro_image-5 --libs)
