#ifndef REDE_H
#define REDE_H

#include <stdint.h>

#define MIN_MENSAGE_SIZE 4
#define MARCA_INICIO 0b01111110
#define ETHERNET_HEADER_SIZE 14
#define ETHERNET_MIN_FRAME_SIZE 60
#define TAM_MAXIMO_MENSAGEM 31

typedef struct
{
    uint16_t tamanho;
    uint8_t num_sequencia : 6;
    uint8_t tipo : 5;
    uint8_t *dados;
} Mensagem;

char *montaMensagem(Mensagem *mensagem);
int desmontaMensagem(const char *mensagem, Mensagem *protocolo);
void enviaMensagem(Mensagem *mensagem, int soquete);
void enviarAK(uint8_t seq, int soquete);
void enviarNAK(uint8_t seq, int soquete);
Mensagem *criaMensagemDoCliente();
Mensagem *criaMensagemDoServidor();
int cria_raw_socket(char *nome_interface_rede);
uint8_t calcula_crc8(const uint8_t *dados, int tamanho);
int verifica_crc8(const uint8_t *dados, int tamanho, uint8_t crc_recebido);
void printaMensagem(Mensagem *mensagem);

extern int modo_loopback;

#endif
