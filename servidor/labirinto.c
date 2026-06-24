#include "labirinto.h"

#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <stdio.h>

static const char LABIRINTO[MAP_SIZE][MAP_SIZE] = {
    "0000000000000000000000000000000000000X00",
    "0000000000000000000000000000000000000000",
    "0000000000000000000000000000000000000000",
    "0XXXXXXX00000000XXXXXXXXXXXXXXXXXXXXXXXX",
    "0000000000000000000000000000000000000000",
    "0000000000000000000000000000000000000000",
    "00XX0000XX00XXXXXXXX00000000000000000000",
    "00XX0000XX00XXXXXXXX0XXXXX000XXXXXX00000",
    "00XX0000XX00XX00000X0000X00X00000X000000",
    "00XX0000XX00XX00000X0000X00X00000X000000",
    "00XX0000XX00XXXXXXXXXXXXX000XXXXXX000000",
    "00XX0000XX00XX00000X0000000XXXX000000000",
    "00XX0000XX00XX00000X0000000X0XXXX0000000",
    "000XXXXXX000XX00000X0000000X000XXX000000",
    "0000XXX00000X000000000000000000000000000",
    "000000000000X0000000000000000000X0000000",
    "000000000000X0000000000000000000X0000000",
    "00000000000000000000000000000000X0000000",
    "00000000000000000000000000000000X0000000",
    "00000000000000000000000000000000X0000000",
    "00000000000000000000000000000000X0000000",
    "00000X00000000000000000000000000X0000000",
    "00000000000000000000000000000000X0000000",
    "00000000000000000000000000000000X0000000",
    "00000000000000000000000000000000X0000000",
    "00000X00000000000000000000000000X0000000",
    "00000X00000000000000000000000000X0000000",
    "00000X00000000000000000000000000X0000000",
    "00000000000000000000000000000000X0000000",
    "00000000000000000000000000000000X0000000",
    "0000000000000000000000000000000XX0000000",
    "0000000000000000000000000000000000000000",
    "0000000000000000000000000000000000000000",
    "000000000XXXX000000000000000000000000000",
    "0000000000000000000000000000000000000000",
    "0000000000000000000000000000000000000000",
    "0000000000000000000000000000000000000000",
    "0000000000000000000000000000000000000000",
    "0000000000000000000000000000000000000000",
    "0000000000000000000000000000000000000000",
};

static const char artefatos[] = {'P', '1', '2', '3', '4', '5', '6', 'R', 'B', 'G', 'Y', '\0'};

GameState* criaGameState()
{
    GameState *gameState = malloc(sizeof(GameState));
    if (gameState == NULL) {
        return NULL;
    }
    
    gameState->vidas = 5;
    gameState->movimentos_totais = 0;
    for (int i = 0; i < 12; i++) {
        gameState->startPosX[i] = -1;
        gameState->startPosY[i] = -1;
    }

    iniciaLabirinto(gameState->labirinto);
    carregaPosicoesLabirinto(gameState->labirinto, gameState);
    
    printf("GameState criado com sucesso!\n");
    printf("Posição inicial do Pac-Man: (%d, %d)\n", gameState->artefatosPosX[0], gameState->artefatosPosY[0]);
    
    return gameState;
}

void iniciaLabirinto(char labirinto[MAP_SIZE][MAP_SIZE])
{
    for (int i = 0; i < MAP_SIZE; i++)
    {
        for (int j = 0; j < MAP_SIZE; j++)
        {
            labirinto[i][j] = LABIRINTO[i][j];
        }
    }
}

void carregaLabirinto(FILE *arquivo, char labirinto[MAP_SIZE][MAP_SIZE], GameState *gameState)
{
    char line[1024];
    int i = 0;
    while (i < MAP_SIZE && fgets(line, sizeof(line), arquivo) != NULL)
    {
        int j = 0;
        char *token = strtok(line, ";\n\r");
        while (token != NULL && j < MAP_SIZE) {
            labirinto[i][j] = token[0];
            posicionaAretefatoNoGameState(i, j, token[0], gameState);
            j++;
            token = strtok(NULL, ";\n\r");
        }
        i++;
    }
}

void carregaPosicoesLabirinto(char labirinto[MAP_SIZE][MAP_SIZE], GameState *gameState)
{
    srand(time(NULL));
    int pos = 0;

    while (artefatos[pos] != '\0')
    {
        int x, y;
        do
        {
            x = rand() % MAP_SIZE;
            y = rand() % MAP_SIZE;
        } while (labirinto[x][y] != '0');
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
        gameState->artefatosPosX[0] = x; gameState->artefatosPosY[0] = y;
        gameState->startPosX[0] = x; gameState->startPosY[0] = y;
        break;
    case '1':
        gameState->artefatosPosX[1] = x; gameState->artefatosPosY[1] = y;
        gameState->startPosX[1] = x; gameState->startPosY[1] = y;
        break;
    case '2':
        gameState->artefatosPosX[2] = x; gameState->artefatosPosY[2] = y;
        gameState->startPosX[2] = x; gameState->startPosY[2] = y;
        break;
    case '3':
        gameState->artefatosPosX[3] = x; gameState->artefatosPosY[3] = y;
        gameState->startPosX[3] = x; gameState->startPosY[3] = y;
        break;
    case '4':
        gameState->artefatosPosX[4] = x; gameState->artefatosPosY[4] = y;
        gameState->startPosX[4] = x; gameState->startPosY[4] = y;
        break;
    case '5':
        gameState->artefatosPosX[5] = x; gameState->artefatosPosY[5] = y;
        gameState->startPosX[5] = x; gameState->startPosY[5] = y;
        break;
    case '6':
        gameState->artefatosPosX[6] = x; gameState->artefatosPosY[6] = y;
        gameState->startPosX[6] = x; gameState->startPosY[6] = y;
        break;
    case 'R':
        gameState->artefatosPosX[7] = x; gameState->artefatosPosY[7] = y;
        gameState->startPosX[7] = x; gameState->startPosY[7] = y;
        break;
    case 'G':
        gameState->artefatosPosX[8] = x; gameState->artefatosPosY[8] = y;
        gameState->startPosX[8] = x; gameState->startPosY[8] = y;
        break;
    case 'B':
        gameState->artefatosPosX[9] = x; gameState->artefatosPosY[9] = y;
        gameState->startPosX[9] = x; gameState->startPosY[9] = y;
        break;
    case 'Y':
        gameState->artefatosPosX[10] = x; gameState->artefatosPosY[10] = y;
        gameState->startPosX[10] = x; gameState->startPosY[10] = y;
        break;
    default:
        break;
    }
}

void resetaPosicoes(char labirinto[MAP_SIZE][MAP_SIZE], GameState *gameState) {
    int ids[] = {0, 7, 8, 9, 10}; // PacMan e Fantasmas
    char symbols[] = {'P', 'R', 'G', 'B', 'Y'};
    
    for (int k = 0; k < 1; k++) {
        int idx = ids[k];
        int cx = gameState->artefatosPosX[idx];
        int cy = gameState->artefatosPosY[idx];
        if (cx >= 0 && cy >= 0 && cx < MAP_SIZE && cy < MAP_SIZE) {
            if (labirinto[cx][cy] == symbols[k]) {
                labirinto[cx][cy] = '0'; // limpa
            }
        }
    }

    for (int k = 0; k < 1; k++) {
        int idx = ids[k];
        int sx = gameState->startPosX[idx];
        int sy = gameState->startPosY[idx];
        if (sx >= 0 && sy >= 0 && sx < MAP_SIZE && sy < MAP_SIZE) {
            labirinto[sx][sy] = symbols[k];
            gameState->artefatosPosX[idx] = sx;
            gameState->artefatosPosY[idx] = sy;
        }
    }
}

void printa_labirinto(char labirinto[MAP_SIZE][MAP_SIZE])
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
