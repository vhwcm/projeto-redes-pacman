#include <sys/ioctl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

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
    init_log("client.log");
    if(argc < 2){
        log_print("Uso: %s <Nome_Cliente> [--debug]\n", argv[0]);
        return 0;
    }
    
    if(argc >= 3 && strcmp(argv[argc-1], "--debug") == 0) {
        debug_mode = 1;
    }
    
    const char *Nome_Client = argv[1];

    unsigned int socket;
    if(!(socket = cria_raw_socket(Nome_Client))){
        log_print("ERRO: cria_raw_socket\n");
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