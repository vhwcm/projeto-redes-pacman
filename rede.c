#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <linux/if_packet.h>
#include <linux/if_ether.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <unistd.h>
#include <sys/time.h>

#include "rede.h"

int modo_loopback = 0;
static uint8_t seq_global_send = 0;

long long timestamp_ms(void)
{
    struct timeval tp;
    gettimeofday(&tp, NULL);
    return (long long)tp.tv_sec * 1000 + tp.tv_usec / 1000;
}

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
    unsigned int crc = (unsigned char)mensagem[tamanhoMensagem + 3];

    if (!verifica_crc8((uint8_t *)&mensagem[3], tamanhoMensagem, crc))
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
    int totalDados = mensagem->tamanho;
    int offset = 0;

    while (offset < totalDados) {
        int chunkSize = totalDados - offset;
        if (chunkSize > TAM_MAXIMO_MENSAGEM)
            chunkSize = TAM_MAXIMO_MENSAGEM;

        Mensagem *frame = criaMensagem();
        frame->tipo          = mensagem->tipo;
        frame->num_sequencia = seq_global_send;
        seq_global_send      = (seq_global_send + 1) % 64;
        frame->tamanho       = chunkSize;
        frame->dados         = mensagem->dados + offset;

        char *raw = montaMensagem(frame);
        int frameTam = chunkSize + MIN_MENSAGE_SIZE;
        ssize_t sent = send(soquete, raw, frameTam, 0);
        if (sent < 0)
            perror("ERROR: enviaMensagem send");

        free(raw);
        free(frame);
        offset += chunkSize;
    }

    Mensagem *fim = criaMensagem();
    fim->tipo          = 16;
    fim->num_sequencia = seq_global_send;
    seq_global_send    = (seq_global_send + 1) % 64;
    fim->tamanho       = 0;
    fim->dados         = NULL;
    char *rawFim = montaMensagem(fim);
    send(soquete, rawFim, MIN_MENSAGE_SIZE, 0);
    free(rawFim);
    free(fim);
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
    Mensagem ack;
    ack.tipo          = 1;
    ack.num_sequencia = seq;
    ack.tamanho       = 0;
    ack.dados         = NULL;
    char *raw = montaMensagem(&ack);
    send(soquete, raw, MIN_MENSAGE_SIZE, 0);
    free(raw);
}

void enviarNAK(uint8_t seq, int soquete)
{
    Mensagem nak;
    nak.tipo          = 15;
    nak.num_sequencia = seq;
    nak.tamanho       = 0;
    nak.dados         = NULL;
    char *raw = montaMensagem(&nak);
    send(soquete, raw, MIN_MENSAGE_SIZE, 0);
    free(raw);
}

Mensagem *criaMensagem()
{
    Mensagem *mensagem = malloc(sizeof(Mensagem));
    mensagem->num_sequencia = 0;
    mensagem->tipo          = 0;
    mensagem->tamanho       = 0;
    mensagem->dados         = NULL;
    return mensagem;
}
