#include "rede.h"

#include <arpa/inet.h>
#include <net/ethernet.h>
#include <linux/if_packet.h>
#include <net/if.h>
#include <stdio.h>
#include <stdlib.h>

#include <stdint.h>
#include <sys/socket.h>

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

char *montaMensagem(Mensagem *protocolo)
{
    int tamanhoMensagem = (MIN_MENSAGE_SIZE) + (protocolo->tamanho >> 3);
    char *mensagem = malloc(tamanhoMensagem + MIN_MENSAGE_SIZE);
    mensagem[0] = MARCA_INICIO;
    mensagem[1] = (protocolo->tamanho << 3) | (protocolo->num_sequencia >> 3);
    mensagem[2] = ((protocolo->num_sequencia & 0x07) << 5) | (protocolo->tipo & 0x1F);
    for (int i = 0; i < tamanhoMensagem; i++)
    {
        mensagem[i + 3] = protocolo->dados[i];
    }

    unsigned int crc = calcula_crc8((uint8_t *)&mensagem[1], tamanhoMensagem + 2);
    mensagem[tamanhoMensagem + 3] = crc;
    return mensagem;
}

int desmontaMensagem(const char *mensagem, Mensagem *protocolo)
{
    if (mensagem[0] != MARCA_INICIO)
    {
        return;
    }

    protocolo->tamanho = (mensagem[1] >> 3) & 0x1F;
    protocolo->num_sequencia = ((mensagem[1] & 0x07) << 3) | ((mensagem[2] >> 5) & 0x07);
    protocolo->tipo = mensagem[2] & 0x1F;

    int tamanhoMensagem = (MIN_MENSAGE_SIZE + protocolo->tamanho) >> 3;
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

    return soquete;
}

void enviaMensagem(Mensagem *mensagem, int soquete)
{
    char *buffer = montaMensagem(mensagem);
    int tamanho = (MIN_MENSAGE_SIZE) + (mensagem->tamanho >> 3);

    if (send(soquete, buffer, tamanho, 0) == -1)
    {
        printf("Erro ao enviar mensagem\n");
    }

    free(buffer);
}

int mensagemVemDoServidor(unsigned valor)
{
    switch (valor)
    {
    case 2:  // vizualização
    case 4:  // dados dos arquivos
    case 5:  // txt
    case 6:  // jpg
    case 7:  // mp4
    case 15: // erros
    case 16: // fim da transmissão
        return 1;
        break;

    default:
        return 0;
        break;
    }
}