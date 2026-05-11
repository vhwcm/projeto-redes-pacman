#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <arpa/inet.h>
#include <net/ethernet.h>
#include <linux/if_packet.h>
#include <net/if.h>

#include <sys/socket.h>

#include "Client_socket.h"
#include "Pacman.h"

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

    // Abrir jogo
        // criar os dados básicos do jogo(tela,teclado,etc)
        printf("Criando a base do jogo\n");
    char* table;
    if(!(table = game_table())){
        printf("ERROR: game table");
        return 0;
    }
    // recebe os parametros do servidor
    character* list;
    if(!(list = create_character(socket))){
        printf("ERROR: create ch\n");
        return 0;
    }
    // Iniciar jogo
    printf("iniciar o jogo\n");
    pacman(table, list);
    
    // Encerra o socket
    printf("encerrar socket\n");
    close(socket);
    return 0;
}