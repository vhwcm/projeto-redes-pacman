#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define MAP_SIZE 40

const char LABIRINTO[MAP_SIZE][MAP_SIZE + 1] = {
    "0000000000000000000000000000000000000000",
    "0000000000000000000000000000000000000000",
    "0000000000000000000000000000000000000000",
    "0000000000000000000000000000000000000000",
    "0000000000000000000000000000000000000000",
    "0000000000000000000000000000000000000000",
    "0000000000000000000000000000000000000000",
    "0000000000000000000000000000000000000000",
    "0000000000000000000000000000000000000000",
    "0000000000000000000000000000000000000000",
    "0000000000000000000000000000000000000000",
    "0000000000000000000000000000000000000000",
    "0000000000000000000000000000000000000000",
    "0000000000000000000000000000000000000000",
    "0000000000000000000000000000000000000000",
    "0000000000000000000000000000000000000000",
    "0000000X000X00XXXXX00XXXX000XXXX0000000",
    "0000000X000X00X0000000X000X00X000X0000000",
    "0000000X000X00X0000000XXXX000XXXX0000000",
    "0000000X000X00XXXX000X000000X0X0000000",
    "00000000XXX000X0000000X000000X00X0000000",
    "0000000000000000000000000000000000000000",
    "0000000000000000000000000000000000000000",
    "0000000000000000000000000000000000000000",
    "0000000000000000000000000000000000000000",
    "0000000000000000000000000000000000000000",
    "0000000000000000000000000000000000000000",
    "0000000000000000000000000000000000000000",
    "0000000000000000000000000000000000000000",
    "0000000000000000000000000000000000000000",
    "0000000000000000000000000000000000000000",
    "0000000000000000000000000000000000000000",
    "0000000000000000000000000000000000000000",
    "0000000000000000000000000000000000000000",
    "0000000000000000000000000000000000000000",
    "0000000000000000000000000000000000000000",
    "0000000000000000000000000000000000000000",
    "0000000000000000000000000000000000000000",
    "0000000000000000000000000000000000000000",
    "0000000000000000000000000000000000000000",
};

const char artefatos[] = {'P', '1', '2', '3', '4', '5', '6', 'R', 'B', 'G', 'Y', '\0'};
typedef struct
{
    int artefatosPosX[12]; // '/0' representa que já foi coletado
    int artefatosPosY[12]; // '/0' representa que já foi coletado
    int round;
} GameState;

int main(int argc, char *argv[])
{
    GameState *gameState = malloc(sizeof(GameState));
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
    }
    else
    {
        printf("Uso: %s <arquivo_labirinto>\n", argv[0]);
        return 1;
    }

    iniciaLabirinto(labirinto);
}

void iniciaLabirinto(int labirinto[MAP_SIZE][MAP_SIZE])
{
    for (int i = 0; i < MAP_SIZE; i++)
    {
        for (int j = 0; j < MAP_SIZE; j++)
        {
            labirinto[i][j] = '0';
        }
    }
}

void carregaLabirinto(FILE *arquivo, int labirinto[MAP_SIZE][MAP_SIZE], GameState *gameState)
{
    char buffer[5000];
    int ponteiroBuffer = 0;
    if (fgets(buffer, sizeof(buffer), arquivo) != NULL)
    {
        for (int i = 0; i < MAP_SIZE; i++)
        {
            for (int j = 0; j < MAP_SIZE; j++)
            {
                labirinto[i][j] = buffer[ponteiroBuffer];
                posicionaAretefatoNoGameState(i, j, buffer[ponteiroBuffer], gameState);
                ponteiroBuffer += 2;
            }
        }
    }
}

void carregaPosicoesLabirinto(int labirinto[MAP_SIZE][MAP_SIZE], GameState *gameState)
{
    int pos = 0;

    while (artefatos[pos] != '\0')
    {
        int x, y;
        do
        {
            x = rand() % MAP_SIZE;
            y = rand() % MAP_SIZE;
        } while (labirinto[x][y] == 0);
        labirinto[x][y] = artefatos[pos];
        posicionaAretefatoNoGameState(x, y, artefatos[pos], gameState);
        pos++;
    }
}

void posicionaAretefatoNoGameState(int x, int y, char a, GameState *gameState)
{
    switch (a)
    {
    case 'P':
        gameState->artefatosPosX[0] = x;
        gameState->artefatosPosY[0] = y;
        break;
    case '1':
        gameState->artefatosPosX[1] = x;
        gameState->artefatosPosY[1] = y;
        break;
    case '2':
        gameState->artefatosPosX[2] = x;
        gameState->artefatosPosY[2] = y;
        break;
    case '3':
        gameState->artefatosPosX[3] = x;
        gameState->artefatosPosY[3] = y;
        break;
    case '4':
        gameState->artefatosPosX[4] = x;
        gameState->artefatosPosY[4] = y;
        break;
    case '5':
        gameState->artefatosPosX[5] = x;
        gameState->artefatosPosY[5] = y;
        break;
    case '6':
        gameState->artefatosPosX[6] = x;
        gameState->artefatosPosY[6] = y;
        break;
    case 'R':
        gameState->artefatosPosX[7] = x;
        gameState->artefatosPosY[7] = y;
        break;
    case 'G':
        gameState->artefatosPosX[8] = x;
        gameState->artefatosPosY[8] = y;
        break;
    case 'B':
        gameState->artefatosPosX[9] = x;
        gameState->artefatosPosY[9] = y;
        break;
    case 'Y':
        gameState->artefatosPosX[10] = x;
        gameState->artefatosPosY[10] = y;
        break;
    default:
        break;
    }
}