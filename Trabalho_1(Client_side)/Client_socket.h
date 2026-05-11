#ifndef CLIENT_SOCKET
#define CLIENT_SOCKET

#define MARCA_INICIO 0b01111110

// tipos
#define ACK 0b00000
#define NACK 0b00001
#define INICIALIZACAO 0b00011
#define DIREITA 0b01010
#define ESQUERDA 0b01011
#define CIMA 0b01100
#define BAIXO 0b01101
#define ERROS 0b01111
#define FIM_TRANSMISSAO 0b10000

typedef struct
{
    uint8_t m_inicio : 8;
    uint8_t tamanho : 5;
    uint8_t sequencia : 6;
    uint8_t tipo : 5;
    uint8_t *dados;
    uint8_t CRC : 8;
} Mensagem;

int cria_raw_socket(const char* nome_interface_rede);

Mensagem* cria_msg();

void Enviar_p_servidor();

void Receber_d_servidor();

#endif