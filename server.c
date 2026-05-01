#include <stdio.h>
#include <stdlib.h>

#include "labirinto.h"
#include "rede.h"

void enviarVisualizacao(int soquete, unsigned int labirinto[MAP_SIZE][MAP_SIZE]);
int leProtocoloMontaMensagem(Mensagem *mensagem, unsigned char bytes[2048], unsigned int *i, int soquete);

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        printf("Uso: %s <nome_rede> [arquivo_labirinto]\n", argv[0]);
        return 1;
    }

    char *nome_rede = argv[1];

    GameState *gameState = malloc(sizeof(GameState));
    unsigned int labirinto[MAP_SIZE][MAP_SIZE];

    if (argc == 3)
    {
        FILE *arquivoCSV = fopen(argv[2], "r");
        if (arquivoCSV == NULL)
        {
            printf("Erro ao abrir arquivo\n");
            return 1;
        }
        carregaLabirinto(arquivoCSV, labirinto, gameState);
        fclose(arquivoCSV);
    }
    else if (argc == 2)
    {
        iniciaLabirinto(labirinto);
        carregaPosicoesLabirinto(labirinto, gameState);
        printa_labirinto(labirinto);
    }
    else
    {
        printf("Uso: %s <nome_rede> [arquivo_labirinto]\n", argv[0]);
        return 1;
    }

    iniciaLabirinto(labirinto);
    unsigned char buffer[2048];
    unsigned int soquete = cria_raw_socket(nome_rede);
    unsigned int bytes;

    while (1)
    {
        bytes = recv(soquete, buffer, sizeof(buffer), 0);
        if (bytes <= 0)
            continue;

        for (int i = 0; i < bytes; i++)
        {
            if (buffer[i] == MARCA_INICIO)
            {
                i++;
                Mensagem *mensagem = criaMensagemDoServidor();
                unsigned int tipo = leProtocoloMontaMensagem(mensagem, bytes, &i, soquete);
                switch (tipo)
                {
                case 2:
                    enviarVisualizacao(soquete, gameState->labirinto);
                    break;
                case 10:
                case 11:
                case 12:
                case 13:
                    movimentaPacMan(soquete);
                    break;

                default:
                    break;
                }
            }
        }
    }
    return 0;
}

void enviarVisualizacao(int soquete, unsigned int labirinto[MAP_SIZE][MAP_SIZE])
{
    Mensagem *mensagem = criaMensagemDoServidor();
    mensagem->num_sequencia = 0;
    mensagem->tamanho = LABIRINTO_SIZE;
    mensagem->tipo = 2;
    mensagem->dados = labirinto;

    enviaMensagem(mensagem, soquete);
}

// retorna o tipo da mensagem e monta a mensagem. retorna -1 em caso de erro
int leProtocoloMontaMensagem(Mensagem *mensagem, unsigned char bytes[2048], unsigned int *i, int soquete)
{
    (*i)++;
    Mensagem *mensagem = criaMensagemDoServidor();
    uint8_t tamanho = buffer[*i + 1] >> 3;
    uint8_t numSequencia = (buffer[*i + 1] << 5) & (buffer[*i + 2] >> 5);
    uint8_t tipo = ((buffer[*i + 2] << 3) >> 3);
    Mensagem *mensagem = criaMensagemDoServidor();
    mensagem->tamanho = tamanho;
    mensagem->num_sequencia = numSequencia;
    mensagem->tipo = tipo;
    if (tamanho > 0)
    {
        mensagem->dados = malloc(sizeof(tamanho));
        if (verifica_crc8(mensagem->dados, mensagem->tamanho, mensagem->crc))
        {
            if (mensagem->num_sequencia % 4 == 0)
            {
                enviarAK(mensagem, soquete);
            }
        }
        else
        {
            enviarNAK(mensagem, soquete);
            return -1;
        }
    }
    (*i) += 3;
    return tipo;
}
