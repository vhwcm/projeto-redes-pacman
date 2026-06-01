CC ?= gcc
CFLAGS ?= -Wall -Wextra -O2 -g

BIN ?= server
TEST_BIN = servidor_teste
PROJECT ?= projeto-redes-pacman
TAR_NAME ?= $(PROJECT).tar.gz

SRC = server.c labirinto.c rede.c
OBJ = $(SRC:.c=.o)
TEST_SRC = servidor_teste.c rede.c
TEST_OBJ = $(TEST_SRC:.c=.o)

.PHONY: all clean package

all: $(BIN) $(TEST_BIN)

$(BIN): $(OBJ)
	$(CC) $(CFLAGS) $(OBJ) -o $(BIN)

$(TEST_BIN): $(TEST_OBJ)
	$(CC) $(CFLAGS) $(TEST_OBJ) -o $(TEST_BIN) -lncurses -lpthread

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) $(TEST_OBJ) $(BIN) $(TEST_BIN)

package: clean
	tar --exclude='*.o' --exclude='$(BIN)' -czf $(TAR_NAME) .
