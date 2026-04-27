#include "rede.h"

#include <arpa/inet.h>
#include <net/ethernet.h>
#include <linux/if_packet.h>
#include <net/if.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>

char *montaMensagem(Mensagem *protocolo)
{
    int tamanhoMensagem = (MIN_MENSAGE_SIZE) + (protocolo->tamanho >> 3);
    char *mensagem = malloc(tamanhoMensagem);
    mensagem[0] = MARCA_INICIO;
    mensagem[1] = (protocolo->tamanho << 3) | (protocolo->num_sequencia >> 3);
    mensagem[2] = ((protocolo->num_sequencia & 0x07) << 5) | (protocolo->tipo & 0x1F);
    for (int i = 0; i < tamanhoMensagem; i++)
    {
        mensagem[i + 3] = protocolo->dados[i];
    }
    mensagem[tamanhoMensagem + 3] = protocolo->crc;
    return mensagem;
}

void desmontaMensagem(const char *mensagem, Mensagem *protocolo)
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
    protocolo->crc = mensagem[tamanhoMensagem + 3];
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
