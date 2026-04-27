#include "labirinto.h"

#include <stdlib.h>

static const char LABIRINTO[MAP_SIZE][MAP_SIZE + 1] = {
    "0000000000000000000000000000000000000000",
    "0000000000000000000000000000000000000000",
    "0000000000000000000000000000000000000000",
    "0000000000000000000000000000000000000000",
    "0000000000000000000000000000000000000000",
    "0000000000000000000000000000000000000000",
    "0000000000000000000000000000000000000000",
    "000X0000X0XXXXXX0XXXXX000XXXXXX0000000000",
    "000X0000X0X000000X0000X00X00000X00000000",
    "000X0000X0X000000X0000X00X00000X00000000",
    "000X0000X0XXXXXX0XXXXX000XXXXXX000000000",
    "000X0000X0X000000X0000000XXXX00000000000",
    "000X0000X0X000000X0000000X0XXXX000000000",
    "0000XXXX00X000000X0000000X000XXX00000000",
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
    "0000000000000000000000000000000000000000",
    "0000000000000000000000000000000000000000",
    "0000000000000000000000000000000000000000",
    "0000000000000000000000000000000000000000",
    "0000000000000000000000000000000000000000",
    "0000000000000000000000000000000000000000",
    "0000000000000000000000000000000000000000",
};

static const char artefatos[] = {'P', '1', '2', '3', '4', '5', '6', 'R', 'B', 'G', 'Y', '\0'};

void iniciaLabirinto(int labirinto[MAP_SIZE][MAP_SIZE])
{
    for (int i = 0; i < MAP_SIZE; i++)
    {
        for (int j = 0; j < MAP_SIZE; j++)
        {
            labirinto[i][j] = LABIRINTO[i][j];
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

void printa_labirinto(int labirinto[MAP_SIZE][MAP_SIZE])
{
    for (int i = 0; i < MAP_SIZE; i++)
    {
        for (int j = 0; j < MAP_SIZE; j++)
        {
            int cell = labirinto[i][j];
            if (cell == 0)
            {
                putchar(' ');
            }
            else
            {
                putchar((char)cell);
            }
        }
        putchar('\n');
    }
}
