#include <sys/ioctl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <arpa/inet.h>
#include <net/ethernet.h>
#include <linux/if_packet.h>
#include <net/if.h>

#include <sys/socket.h>
#include <sys/time.h>

#include "Client_socket.h"
#include "Pacman.h"

const int timeoutMillis = 200;

int main(int argc, char const *argv[]){
    if(argc < 2){
        printf("Uso: %s <Nome_Cliente> não definido\n", argv[0]);
        return 0;
    }
    
    const char *Nome_Client = argv[1];

    unsigned int socket;
    if(!(socket = cria_raw_socket(Nome_Client))){
        printf("ERRO: cria_raw_socket\n");
        return 0;
    }

    configurar_timeout(socket, timeoutMillis);

    struct winsize w;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
    int LINES = w.ws_row;
    int COLS = w.ws_col;

    clear_screen();

    print_title(LINES, COLS);
    
    my_getch();

    clear_screen();

    pacman_game(LINES, COLS, socket);

    return 0;
}