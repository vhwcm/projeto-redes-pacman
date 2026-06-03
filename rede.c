#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <linux/if_packet.h>
#include <linux/if_ether.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <unistd.h>


#include "rede.h"

int modo_loopback = 0;

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

char *montaMensagem(Mensagem *mensagem)
{
    int tamanhoDados = mensagem->tamanho;
    int tamanho_protocolo = tamanhoDados + MIN_MENSAGE_SIZE;

    char *protocolo = malloc(tamanho_protocolo);
    protocolo[0] = MARCA_INICIO;
    protocolo[1] = ((uint8_t)mensagem->tamanho << 3) | (mensagem->num_sequencia >> 3);
    protocolo[2] = ((mensagem->num_sequencia & 0x07) << 5) | (mensagem->tipo & 0x1F);
    for (int i = 0; i < tamanhoDados; i++)
    {
        protocolo[i + 3] = mensagem->dados[i];
    }
    uint8_t crc = calcula_crc8(mensagem->dados, tamanhoDados);
    protocolo[tamanhoDados + 3] = crc;

    return protocolo;
}

int desmontaMensagem(const char *mensagem, Mensagem *protocolo)
{
    if (mensagem[0] != MARCA_INICIO) {
        return 0;
    }

    protocolo->tamanho = (mensagem[1] >> 3) & 0x1F;
    protocolo->num_sequencia = ((mensagem[1] & 0x07) << 3) | ((mensagem[2] >> 5) & 0x07);
    protocolo->tipo = mensagem[2] & 0x1F;

    int tamanhoMensagem = protocolo->tamanho;
    protocolo->dados = malloc(tamanhoMensagem);
    for (int i = 0; i < tamanhoMensagem; i++)
    {
        protocolo->dados[i] = mensagem[i + 3];
    }
    unsigned int crc = mensagem[tamanhoMensagem + 3];

    if (!verifica_crc8((uint8_t *)&mensagem[1], tamanhoMensagem + 2, crc))
    {
        fprintf(stderr, "CRC inválido! Mensagem corrompida.\n");
        return 0;
    }

    return 1;
}

int cria_raw_socket(char *nome_interface_rede)
{
    // Cria arquivo para o socket sem qualquer protocolo
    int soquete = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
    if (soquete == -1)
    {
        fprintf(stderr, "Erro ao criar socket: Verifique se você é root!\n");
        exit(-1);
    }

    int ifindex = if_nametoindex(nome_interface_rede);

    struct sockaddr_ll endereco = {0};
    endereco.sll_family = AF_PACKET;
    endereco.sll_protocol = htons(ETH_P_ALL);
    endereco.sll_ifindex = ifindex;
    // Inicializa socket
    if (bind(soquete, (struct sockaddr *)&endereco, sizeof(endereco)) == -1)
    {
        fprintf(stderr, "Erro ao fazer bind no socket\n");
        exit(-1);
    }

    struct packet_mreq mr = {0};
    mr.mr_ifindex = ifindex;
    mr.mr_type = PACKET_MR_PROMISC;
    // Não joga fora o que identifica como lixo: Modo promíscuo
    if (setsockopt(soquete, SOL_PACKET, PACKET_ADD_MEMBERSHIP, &mr, sizeof(mr)) == -1)
    {
        fprintf(stderr, "Erro ao fazer setsockopt: "
                        "Verifique se a interface de rede foi especificada corretamente.\n");
        exit(-1);
    }

    int ignore = 1;
    setsockopt(soquete, SOL_PACKET, PACKET_IGNORE_OUTGOING, &ignore, sizeof(ignore));

    return soquete;
}

void enviaMensagem(Mensagem *mensagem, int soquete)
{
    Mensagem *frameMensagem = criaMensagemDoServidor();

    int quantFrames = mensagem->tamanho / TAM_MAXIMO_MENSAGEM;
    int tamUltimoFrame = mensagem->tamanho % TAM_MAXIMO_MENSAGEM;
    frameMensagem->tipo = mensagem->tipo;
    int tamanho = mensagem->tamanho + MIN_MENSAGE_SIZE;
    for (int i = 0; i < quantFrames; i++) {
        frameMensagem->dados = &mensagem->dados[i*TAM_MAXIMO_MENSAGEM];
        frameMensagem->tamanho = TAM_MAXIMO_MENSAGEM;
        if (i == quantFrames -1 ) {
            frameMensagem->tamanho = tamUltimoFrame;
        }
        frameMensagem->num_sequencia = i;
        char* frame = montaMensagem(frameMensagem);
        ssize_t sent = send(soquete, frame, tamanho, 0);
        if (sent < 0) {
            perror("ERROR: send");
        } else {
            printf("Enviados %zd de %d bytes\n", sent, tamanho);
        }
    }
    frameMensagem->tipo = 16;
    frameMensagem->tamanho = 0;
    frameMensagem->num_sequencia = quantFrames;
    char* protocoloFinalização = montaMensagem(frameMensagem);
    send(soquete, protocoloFinalização, tamanho, 0);

    free(frameMensagem);
}



void printaMensagem(Mensagem *mensagem)
{
    printf("=== MENSAGEM ===\n");
    printf("Tipo: %d\n", mensagem->tipo);
    printf("Num num_sequencia: %d\n", mensagem->num_sequencia);
    printf("Tamanho: %d\n", mensagem->tamanho);

    if (mensagem->tamanho > 0 && mensagem->dados != NULL) {
        printf("Dados: ");
        for (int i = 0; i < mensagem->tamanho; i++) {
            printf("%02X ", mensagem->dados[i]);
        }
        printf("\n");
    } else {
        printf("Dados: (vazio)\n");
    }
    printf("================\n");
}


void enviarAK(uint8_t seq, int soquete)
{
    Mensagem *ack = criaMensagemDoServidor();
    ack->tipo = 1;
    ack->num_sequencia = seq;
    ack->tamanho = 1;
    ack->dados = malloc(1);
    ack->dados[0] = 0;
    enviaMensagem(ack, soquete);
    free(ack->dados);
    free(ack);
}

void enviarNAK(uint8_t seq, int soquete)
{
    Mensagem *nak = criaMensagemDoServidor();
    nak->tipo = 15;
    nak->num_sequencia = seq;
    nak->tamanho = 1;
    nak->dados = malloc(1);
    nak->dados[0] = 0;
    enviaMensagem(nak, soquete);
    free(nak->dados);
    free(nak);
}

Mensagem *criaMensagemDoServidor()
{
    Mensagem *mensagem = malloc(sizeof(Mensagem));
    mensagem->num_sequencia = 0;
    mensagem->tipo = 0;
    mensagem->tamanho = 0;
    mensagem->dados = NULL;
    return mensagem;
}

Mensagem *criaMensagemDoCliente()
{
    Mensagem *mensagem = malloc(sizeof(Mensagem));
    return mensagem;
}
