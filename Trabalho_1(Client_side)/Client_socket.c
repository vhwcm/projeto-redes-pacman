#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <arpa/inet.h>
#include <net/ethernet.h>
#include <linux/if_packet.h>
#include <net/if.h>


#include "Client_socket.h"
 
int cria_raw_socket(const char* nome_interface_rede) {
    // Cria arquivo para o socket sem qualquer protocolo
    int soquete = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
    if (soquete == -1) {
        fprintf(stderr, "Erro ao criar socket: Verifique se você é root!\n");
        exit(-1);
    }
 
    int ifindex = if_nametoindex(nome_interface_rede);
 
    struct sockaddr_ll endereco = {0};
    endereco.sll_family = AF_PACKET;
    endereco.sll_protocol = htons(ETH_P_ALL);
    endereco.sll_ifindex = ifindex;
    // Inicializa socket
    if (bind(soquete, (struct sockaddr*) &endereco, sizeof(endereco)) == -1) {
        fprintf(stderr, "Erro ao fazer bind no socket\n");
        exit(-1);
    }
 
    struct packet_mreq mr = {0};
    mr.mr_ifindex = ifindex;
    mr.mr_type = PACKET_MR_PROMISC;
    // Não joga fora o que identifica como lixo: Modo promíscuo
    if (setsockopt(soquete, SOL_PACKET, PACKET_ADD_MEMBERSHIP, &mr, sizeof(mr)) == -1) {
        fprintf(stderr, "Erro ao fazer setsockopt: "
            "Verifique se a interface de rede foi especificada corretamente.\n");
        exit(-1);
    }
 
    return soquete;
}

// Calcula CRC
uint8_t calcula_crc8(const uint8_t *dados, int tamanho)
{
    uint8_t crc = 0x00;
    for (int i = 0; i < tamanho; i++)
    {
        crc ^= dados[i];
        for (int j = 0; j < 8; j++)
        {
            if (crc & 0x80)
                crc = (crc << 1) ^ 0x07;
            else
                crc <<= 1;
        }
    }
    return crc;
}

int verifica_crc8(const uint8_t *dados, int tamanho, uint8_t crc_recebido)
{
    uint8_t crc_calc = calcula_crc8(dados, tamanho);
    return crc_calc == crc_recebido;
}

// Montar a msg para enviar
Mensagem* cria_msg(int tipo){
    Mensagem* msg;
    if(!(msg = malloc(sizeof(Mensagem)))){
        printf("ERROR: malloc msg\n");
        return NULL;
    }

    msg->m_inicio = MARCA_INICIO;
    msg->tamanho = 0b00000;
    msg->sequencia = 0b000000;  // = 0

    switch (tipo){
        case 0:
            msg->tipo = ACK;
            break;
        case 1:
            msg->tipo = NACK;
            break;
        case 3:
            msg->tipo = INICIALIZACAO;
            break;
        case 10:
            msg->tipo = DIREITA;
            break;
        case 11:
            msg->tipo = ESQUERDA;
            break;
        case 12:
            msg->tipo = CIMA;
            break;
        case 13:
            msg->tipo = BAIXO;
            break;
        case 15:
            msg->tipo = ERROS;
            break;
        case 16:
            msg->tipo = FIM_TRANSMISSAO;
            break;
        default:
            printf("ERRO: Case inapropriado\n");
    }

    uint8_t* dados;
    if(!(dados = malloc(3))){
        printf("ERROR: dados = malloc\n");
        return NULL;
    }

    dados[0] = msg->tamanho;
    dados[1] = msg->sequencia;
    dados[2] = msg->tipo;

    msg->CRC = calcula_crc8(dados, 3);

    return msg;

}

// Montar protocolo msg para ser enviado
char* monta_protocolo(Mensagem* msg, int tipo){
    char* protocolo;
    if(!(protocolo = malloc(5))){
        printf("ERROR: protocolo = malloc\n");
        return NULL;
    }

    protocolo[0] = msg->m_inicio;
    protocolo[1] = msg->tamanho;
    protocolo[2] = msg->sequencia;
    protocolo[3] = msg->tipo;
    protocolo[4] = msg->CRC;

    return protocolo;
}

// Envia mensagem
void Enviar_p_servidor(int socket, int tipo){
    Mensagem* msg;
    if(!(msg = cria_msg(tipo))){
        printf("ERRO: cria_msg\n");
        return;
    }
    
    char* buffer;     
    if(!(buffer = monta_protocolo(msg, tipo))){
        printf("ERROR: buffer\n");
        return;
    }

    if(!(send(socket, buffer, strlen(buffer),0))){
        printf("ERROR: send\n");
        return;
    }
    printf("msg enviada\n");

    free(msg);
    free(buffer);
}

Mensagem *desmontar_msg(char* buffer){
    if(buffer[0] != MARCA_INICIO){
        printf("ERROR: (desmontar_msg) Marca_inicio diferente\n");
        return NULL;
    }

    if(!(verifica_crc8(buffer[4], strlen(buffer[4]), buffer[5]))){
        printf("ERROR: CRC8 diferente\n");
        return NULL;
    }

    // se não haver erro, atribuir os seus valores na struct
    Mensagem *msg;
    if(!(msg = malloc(sizeof(Mensagem)))){
        printf("ERROR: malloc msg\n");
        return NULL;
    }
    
    msg->tamanho = buffer[1];
    msg->sequencia = buffer[2];
    msg->tipo = buffer[3];

    if(msg->tipo != FIM_TRANSMISSAO)
        msg->dados = buffer[4];

    printf("TESTE dados:\n");
    printf("%s\n",buffer[4]);

    return msg;
}

// Recebe mensagem
void Receber_d_servidor(int socket){
    Mensagem *msg;
    char *buffer;
    recv(socket, buffer, strlen(buffer),0);

    // desmonta a msg e atribui na estrutura
    msg = desmontar_msg(buffer);

    // caso receber exceto ack e nack
    switch (msg->tipo){
        case 2: //visualização;

            break;
        case 4: //dados
            
            break;
        case 5: //txt
            
            break;
        case 6: //jpg
            
            break;
        case 7: //mp4
            
            break;
        case 15: //erros
            
            break;
        case 16: //fim_transmissao
            
            break;
        default: //outros
            printf("ERROR: tipo invalido\n");
    }
}

// Timestamp