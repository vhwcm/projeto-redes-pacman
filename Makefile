CC ?= gcc
CFLAGS ?= -Wall -Wextra -O2

BIN ?= server
PROJECT ?= projeto-redes-pacman
TAR_NAME ?= $(PROJECT).tar.gz

SRC = server.c labirinto.c rede.c
OBJ = $(SRC:.c=.o)

.PHONY: all clean package

all: $(BIN)

$(BIN): $(OBJ)
	$(CC) $(CFLAGS) $(OBJ) -o $(BIN)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) $(BIN)

package: clean
	tar --exclude='*.o' --exclude='$(BIN)' -czf $(TAR_NAME) .
