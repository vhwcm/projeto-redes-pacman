#include <stdio.h>
#include <stdlib.h>

#include "labirinto.h"

int main(int argc, char *argv[])
{
    GameState *gameState = malloc(sizeof(GameState));
    gameState->round = 0;
    int labirinto[MAP_SIZE][MAP_SIZE];

    if (argc == 2)
    {
        FILE *arquivoCSV = fopen(argv[1], "r");
        if (arquivoCSV == NULL)
        {
            printf("Erro ao abrir arquivo\n");
            return 1;
        }
        carregaLabirinto(arquivoCSV, labirinto, gameState);
        fclose(arquivoCSV);
    }
    else if (argc == 1)
    {
        iniciaLabirinto(labirinto);
        carregaPosicoesLabirinto(labirinto, gameState);
        printa_labirinto(labirinto);
    }
    else
    {
        printf("Uso: %s <arquivo_labirinto>\n", argv[0]);
        return 1;
    }

    iniciaLabirinto(labirinto);
    return 0;
}