#ifndef LABIRINTO_H
#define LABIRINTO_H

#include <stdio.h>

#define MAP_SIZE 40
#define LABIRINTO_SIZE MAP_SIZE *MAP_SIZE

typedef struct
{
    int labirinto[MAP_SIZE][MAP_SIZE];
    int artefatosPosX[12]; // '/0' representa que já foi coletado
    int artefatosPosY[12]; // '/0' representa que já foi coletado
} GameState;

void iniciaLabirinto(int labirinto[MAP_SIZE][MAP_SIZE]);
void carregaLabirinto(FILE *arquivo, int labirinto[MAP_SIZE][MAP_SIZE], GameState *gameState);
void carregaPosicoesLabirinto(int labirinto[MAP_SIZE][MAP_SIZE], GameState *gameState);
void posicionaAretefatoNoGameState(int x, int y, char a, GameState *gameState);
void printa_labirinto(int labirinto[MAP_SIZE][MAP_SIZE]);
void enviarNAK(int soquete, int numSequencia, int servidorOuCliente);
void enviarAK(int soquete, int numSequencia, int servidorOuCliente);

#endif
