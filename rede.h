#ifndef REDE_H
#define REDE_H

#include <stdint.h>

#define MIN_MENSAGE_SIZE 4
#define MARCA_INICIO 0b01111110

typedef struct
{
    uint8_t tamanho : 5;
    uint8_t num_sequencia : 6;
    uint8_t tipo : 5;
    uint8_t *dados;
    uint8_t mensagemDoServidor; // boleano que indica quando o a mensagem vem do servidor ou do cliente
} Mensagem;

char *montaMensagem(Mensagem *mensagem);
int desmontaMensagem(const char *mensagem, Mensagem *protocolo);
int cria_raw_socket(char *nome_interface_rede);
void enviaMensagem(Mensagem *mensagem, int soquete);
void enviarAK(Mensagem *mensagem, int soquete);
void enviarNAK(Mensagem *mensagem, int soquete);
Mensagem *criaMensagemDoCliente();
Mensagem *criaMensagemDoServidor()

#endif
