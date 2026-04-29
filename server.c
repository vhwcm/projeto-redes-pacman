#include <stdio.h>
#include <stdlib.h>

#include "labirinto.h"
#include "rede.h"

void enviarVisualizacao(int soquete, int labirinto[MAP_SIZE][MAP_SIZE]);
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
                uint8_t tamanho = buffer[i + 1] >> 3;
                uint8_t num_sequencia = (buffer[i + 1] << 5) & (buffer[i + 2] >> 5);
                uint8_t tipo = ((buffer[i + 2] << 3) >> 3);
                switch (tipo)
                {
                case 2:
                    enviarLabirinto(soquete, gameState->labirinto);
                    break;

                default:
                    break;
                }
            }
        }
    }
    return 0;
}

void enviarVisualizacao(int soquete, int labirinto[MAP_SIZE][MAP_SIZE])
{
    Mensagem *mensagem = malloc(sizeof(Mensagem));
    mensagem->num_sequencia = 0;
    mensagem->tamanho = LABIRINTO_SIZE;
    mensagem->tipo = 2;
    mensagem->dados = labirinto;

    enviaMensagem(mensagem, soquete);
}
