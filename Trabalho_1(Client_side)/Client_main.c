#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <arpa/inet.h>
#include <net/ethernet.h>
#include <linux/if_packet.h>
#include <net/if.h>

#include <sys/socket.h>
#include <sys/time.h>

#include <ncurses.h>

#include "Client_socket.h"
#include "Pacman.h"

const int timeoutMillis = 200; // 200 milisegundos de timeout

int main(int argc, char const *argv[]){
    // Verificação de argumentos
    if(argc < 2){
        printf("Uso: %s <Nome_Cliente> não definido\n", argv[0]);
        return 0;
    }
    
    const char *Nome_Client = argv[1];

    // Criar socket
    unsigned int socket;
    if(!(socket = cria_raw_socket(Nome_Client))){
        printf("ERRO: cria_raw_socket\n");
        return 0;
    }

    //definir timeout
    configurar_timeout(socket, timeoutMillis);

    // Abrir jogo
    initscr();
    curs_set(0);

    // testa se pode exibir outras cores no terminal
    if(has_colors() == FALSE){ 
        endwin();
        printf("Seu terminal não suporta cores.\n");
        return 1;
    }
  
    start_color();

    // tela: digite algo para iniciar
    print_title(LINES, COLS);

    getch();
    clear();

    // iniciar a tela do jogo
    pacman_game(LINES, COLS, socket);

    endwin();
    return 0;
}