#ifndef CLIENT_SOCKET
#define CLIENT_SOCKET
#include <stdint.h>

#include "Pacman.h"

#define MARCA_INICIO 0b01111110

// tipos
#define ACK 0b00000             // 0
#define NACK 0b00001            // 1
#define VISUALIZACAO 0b00010    // 2
#define INICIALIZACAO 0b00011   // 3
#define DADOS 0b00100           // 4
#define TXT 0b00101             // 5
#define JPG 0b00110             // 6
#define MP4 0b00111             // 7
#define GAME_CLEAR 0b01001      // 9
#define DIREITA 0b01010         // 10
#define ESQUERDA 0b01011        // 11
#define CIMA 0b01100            // 12
#define BAIXO 0b01101           // 13
#define GAME_OVER 0b01110       // 14
#define ERROS 0b01111           // 15
#define FIM_TRANSMISSAO 0b10000 // 16

#define TAM_MAX 31

typedef struct
{
    uint8_t m_inicio : 8;
    uint8_t tamanho : 5;
    uint8_t sequencia : 6;
    uint8_t tipo : 5;
    char *dados;
    uint8_t CRC : 8;
} Mensagem;

void configurar_timeout(int soquete, int timeoutMillis);

int cria_raw_socket(const char* nome_interface_rede);

Mensagem* cria_msg(uint8_t tipo, uint8_t sequencia);

void Enviar_p_servidor(int socket, uint8_t tipo, uint8_t sequencia);

int Receber_d_servidor(int socket, char game_map[MAP_SIZE * MAP_SIZE]);

void init_log(const char *filename);
void log_print(const char *format, ...);

#endif