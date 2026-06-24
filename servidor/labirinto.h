#ifndef LABIRINTO_H
#define LABIRINTO_H

#include <stdio.h>

#define MAP_SIZE 40
#define LABIRINTO_SIZE MAP_SIZE *MAP_SIZE

typedef struct
{
    char labirinto[MAP_SIZE][MAP_SIZE];
    int artefatosPosX[12]; // '-1' representa que já foi coletado
    int artefatosPosY[12]; // '-1' representa que já foi coletado
    int startPosX[12];
    int startPosY[12];
    int vidas;
    int movimentos_totais;
} GameState;

void iniciaLabirinto(char labirinto[MAP_SIZE][MAP_SIZE]);
void carregaLabirinto(FILE *arquivo, char labirinto[MAP_SIZE][MAP_SIZE], GameState *gameState);
void carregaPosicoesLabirinto(char labirinto[MAP_SIZE][MAP_SIZE], GameState *gameState);
void posicionaAretefatoNoGameState(int x, int y, char a, GameState *gameState);
void printa_labirinto(char labirinto[MAP_SIZE][MAP_SIZE]);
void resetaPosicoes(char labirinto[MAP_SIZE][MAP_SIZE], GameState *gameState);
GameState* criaGameState();

#endif
