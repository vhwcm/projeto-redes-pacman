#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "labirinto.h"
#include "rede.h"


void enviarVisualizacao(int soquete, unsigned int labirinto[MAP_SIZE][MAP_SIZE]);
void printaMensagem(Mensagem *mensagem);
int realizaMovimento(unsigned int labirinto[MAP_SIZE][MAP_SIZE], int novaPosX, int novaPosY, GameState *gameState);
int leProtocoloMontaMensagem(Mensagem *mensagem, unsigned char bytes[2048], unsigned int *i, int soquete);

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        printf("Uso: %s <nome_rede> [arquivo_labirinto]\n", argv[0]);
        return 1;
    }

    char *nome_rede = argv[1];

    int modo_loopback = 0;
    if (strcmp(nome_rede, "lo") == 0) {
        modo_loopback = 1;
    }

    GameState *gameState = criaGameState();
    if (gameState == NULL) {
        printf("Erro ao criar GameState\n");
        return 1;
    }

    if (argc == 3)
    {
        FILE *arquivoCSV = fopen(argv[2], "r");
        if (arquivoCSV == NULL)
        {
            printf("Erro ao abrir arquivo\n");
            return 1;
        }
        carregaLabirinto(arquivoCSV, gameState->labirinto, gameState);
        fclose(arquivoCSV);
    }
    else if (argc == 2)
    {
        printa_labirinto(gameState->labirinto);
    }
    else
    {
        printf("Uso: %s <nome_rede> [arquivo_labirinto]\n", argv[0]);
        return 1;
    }
    unsigned char buffer[2048];
    unsigned int soquete = cria_raw_socket(nome_rede);
    unsigned int bytes;
    int descartar_proxima_msg = 0;

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
                if (modo_loopback && descartar_proxima_msg) {
                    descartar_proxima_msg = 0;
                    continue;
                }
                
                Mensagem *mensagemCliente = criaMensagemDoCliente();
                unsigned int tipo = leProtocoloMontaMensagem(mensagemCliente, bytes, &i, soquete);
                

                if (modo_loopback) {
                    descartar_proxima_msg = 1;
                }
                
                switch (tipo)
                {
                case 2:
                    enviarVisualizacao(soquete, gameState->labirinto);
                    break;
                case 10:
                case 11:
                case 12:
                case 13:
                    movimentaPacMan(soquete, tipo, gameState->labirinto, gameState);
                    enviarVisualizacao(soquete, gameState->labirinto);
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

void realizaMovimento(unsigned int labirinto[MAP_SIZE][MAP_SIZE], int novaPosX, int novaPosY, GameState *gameState)
{
    int posXVerificar = novaPosX;
    int posYVerificar = novaPosY;
    int wrapAround = 0;
    
    if (novaPosX < 0) {
        posXVerificar = MAP_SIZE - 1;
        wrapAround = 1;
        printf("Saindo pela esquerda, aparecendo na direita em (%d, %d)\n", posXVerificar, posYVerificar);
    } else if (novaPosX >= MAP_SIZE) {
        posXVerificar = 0;
        wrapAround = 1;
        printf("Saindo pela direita, aparecendo na esquerda em (%d, %d)\n", posXVerificar, posYVerificar);
    } else if (novaPosY < 0) {
        posYVerificar = MAP_SIZE - 1;
        wrapAround = 1;
        printf("Saindo por cima, aparecendo embaixo em (%d, %d)\n", posXVerificar, posYVerificar);
    } else if (novaPosY >= MAP_SIZE) {
        posYVerificar = 0;
        wrapAround = 1;
        printf("Saindo por baixo, aparecendo em cima em (%d, %d)\n", posXVerificar, posYVerificar);
    }
    
    char elemento = labirinto[posXVerificar][posYVerificar];
    
    switch(elemento) {
        case '0': // Espaço vazio
            labirinto[gameState->artefatosPosX[0]][gameState->artefatosPosY[0]] = '0';
            
            labirinto[posXVerificar][posYVerificar] = 'P';
            gameState->artefatosPosX[0] = posXVerificar;
            gameState->artefatosPosY[0] = posYVerificar;
            break;
        case 'P': // PacMan
            printf("Encontrou PacMan na posição (%d, %d)%s\n", posXVerificar, posYVerificar, 
                   wrapAround ? " (wrap-around)" : "");
            // Não pode mover
        case 'X': // Parede
            printf("Parede bloqueando o movimento em (%d, %d)%s\n", posXVerificar, posYVerificar,
                   wrapAround ? " (wrap-around)" : "");
            // Não pode mover
        case 'R': // Fantasma vermelho
            printf("Fantasma vermelho em (%d, %d)%s\n", posXVerificar, posYVerificar,
                   wrapAround ? " (wrap-around)" : "");
            // Não pode mover
        case 'B': // Fantasma azul
            printf("Fantasma azul em (%d, %d)%s\n", posXVerificar, posYVerificar,
                   wrapAround ? " (wrap-around)" : "");
            // Não pode mover
        case 'G': // Fantasma verde
            printf("Fantasma verde em (%d, %d)%s\n", posXVerificar, posYVerificar,
                   wrapAround ? " (wrap-around)" : "");
            // Não pode mover
        case 'Y': // Fantasma amarelo
            printf("Fantasma amarelo em (%d, %d)%s\n", posXVerificar, posYVerificar,
                   wrapAround ? " (wrap-around)" : "");
            // Não pode mover
        case '1': // Pastilha dourada arquivo texto (1.txt)
            printf("Pastilha dourada 1.txt em (%d, %d)%s\n", posXVerificar, posYVerificar,
                   wrapAround ? " (wrap-around)" : "");
            // Não pode mover
        case '2': // Pastilha dourada arquivo texto (2.txt)
            printf("Pastilha dourada 2.txt em (%d, %d)%s\n", posXVerificar, posYVerificar,
                   wrapAround ? " (wrap-around)" : "");
            // Não pode mover
        case '3': // Pastilha dourada arquivo jpg (3.jpg)
            printf("Pastilha dourada 3.jpg em (%d, %d)%s\n", posXVerificar, posYVerificar,
                   wrapAround ? " (wrap-around)" : "");
            // Não pode mover
        case '4': // Pastilha dourada arquivo jpg (4.jpg)
            printf("Pastilha dourada 4.jpg em (%d, %d)%s\n", posXVerificar, posYVerificar,
                   wrapAround ? " (wrap-around)" : "");
            // Não pode mover
        case '5': // Pastilha dourada arquivo mp4 (5.mp4)
            printf("Pastilha dourada 5.mp4 em (%d, %d)%s\n", posXVerificar, posYVerificar,
                   wrapAround ? " (wrap-around)" : "");
            // Não pode mover
        case '6': // Pastilha dourada arquivo mp4 (6.mp4)
            printf("Pastilha dourada 6.mp4 em (%d, %d)%s\n", posXVerificar, posYVerificar,
                   wrapAround ? " (wrap-around)" : "");
            // Não pode mover
        default:
            printf("Elemento desconhecido '%c' em (%d, %d)%s\n", elemento, posXVerificar, posYVerificar,
                   wrapAround ? " (wrap-around)" : "");
    }
}

void movimentaPacMan(int soquete, int tipo, unsigned int labirinto[MAP_SIZE][MAP_SIZE], GameState *gameState)
{
    int posXAtual = gameState->artefatosPosX[0];
    int posYAtual = gameState->artefatosPosY[0];
    int novaPosX = posXAtual;
    int novaPosY = posYAtual;
    
    switch(tipo) {
        case 10: // Direita
            novaPosX = posXAtual + 1;
            printf("Tentando mover para direita: (%d, %d) -> (%d, %d)\n", posXAtual, posYAtual, novaPosX, novaPosY);
            break;
        case 11: // Esquerda
            novaPosX = posXAtual - 1;
            printf("Tentando mover para esquerda: (%d, %d) -> (%d, %d)\n", posXAtual, posYAtual, novaPosX, novaPosY);
            break;
        case 12: // Cima
            novaPosY = posYAtual - 1;
            printf("Tentando mover para cima: (%d, %d) -> (%d, %d)\n", posXAtual, posYAtual, novaPosX, novaPosY);
            break;
        case 13: // Baixo
            novaPosY = posYAtual + 1;
            printf("Tentando mover para baixo: (%d, %d) -> (%d, %d)\n", posXAtual, posYAtual, novaPosX, novaPosY);
            break;
    }
    
    // Realiza o movimento
    realizaMovimento(labirinto, novaPosX, novaPosY, gameState);
    
    printf("Movimento processado!\n");

    Mensagem *mensagem = criaMensagemDoServidor();
    mensagem->num_sequencia = 1;
    mensagem->tamanho = 1;
    mensagem->tipo = 3;
    uint8_t dados[1] = {1}; // Movimento processado
    mensagem->dados = dados;

    enviaMensagem(mensagem, soquete);
}

void printaMensagem(Mensagem *mensagem)
{
    printf("=== MENSAGEM ===\n");
    printf("Tipo: %d\n", mensagem->tipo);
    printf("Num Sequencia: %d\n", mensagem->num_sequencia);
    printf("Tamanho: %d\n", mensagem->tamanho);
    printf("Do Servidor: %s\n", mensagem->mensagemDoServidor ? "Sim" : "Não");
    
    if (mensagem->tamanho > 0 && mensagem->dados != NULL) {
        printf("Dados: ");
        for (int i = 0; i < mensagem->tamanho; i++) {
            printf("%02X ", mensagem->dados[i]);
        }
        printf("\n");
    } else {
        printf("Dados: (vazio)\n");
    }
    printf("================\n");
}

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
