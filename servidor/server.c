#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <poll.h>
#include <linux/if_packet.h>
#include <net/ethernet.h>
#include <arpa/inet.h>

#include "labirinto.h"
#include "rede.h"


void enviarVisualizacao(int soquete, int labirinto[MAP_SIZE][MAP_SIZE]);
void printaMensagem(Mensagem *mensagem);
void realizaMovimento(int labirinto[MAP_SIZE][MAP_SIZE], int novaPosX, int novaPosY, GameState *gameState);
void movimentaPacMan(int soquete, int tipo, int labirinto[MAP_SIZE][MAP_SIZE], GameState *gameState);
int leProtocoloMontaMensagem(Mensagem *mensagem, unsigned char buffer[2048], int *i, int soquete);
void meu_log(char* mensagem);

static uint8_t next_seq_send = 0;
static uint8_t expected_seq_recv = 0;
static int ack_counter = 0;

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
        printf("recebendo\n");
    }
    else
    {
        printf("Uso: %s <nome_rede> [arquivo_labirinto]\n", argv[0]);
        return 1;
    }
    unsigned char buffer[2048];
    for (int i = 0; i < 2048; i++) {
        buffer[i] = 0;
    }
    unsigned int soquete = cria_raw_socket(nome_rede);
    ssize_t bytes;

    struct pollfd fds[1];
    fds[0].fd = soquete;
    fds[0].events = POLLIN;

    while (1)
    {
        int ret = poll(fds, 1, 1000); // 1s timeout
        
        if (ret == 0) {
            // Timeout de 1s sem receber nada
            if (expected_seq_recv > 0 || ack_counter > 0) {
                printf("Timeout de 1s! Enviando NAK para sequencia esperada %d\n", expected_seq_recv);
                enviarNAK(expected_seq_recv, soquete);
            }
            continue;
        } else if (ret < 0) {
            perror("Erro no poll");
            break;
        }

        struct sockaddr_ll from;
        socklen_t fromlen = sizeof(from);
        bytes = recvfrom(soquete, buffer, sizeof(buffer), 0, (struct sockaddr *)&from, &fromlen);
        if (bytes <= 0)
            continue;

        // No loopback, ignoramos ecos de transmissão no loopback
        if (modo_loopback && from.sll_pkttype == PACKET_OUTGOING)
            continue;

        for (int i = 0; i < bytes; i++)
        {
            int encontrado = 0;

            if (buffer[i] == MARCA_INICIO)
            {
                int offset = 1;
                if (i + offset + 1 < bytes) {
                    uint8_t tamanho = buffer[i + offset] >> 3;
                    if (i + offset + 2 + tamanho < bytes) {
                        uint8_t crc = buffer[i + offset + 2 + tamanho];
                        if (verifica_crc8(&buffer[i + offset], tamanho + 2, crc)) {
                            encontrado = 1;
                        }
                    }
                }
            }

            if (encontrado)
            {
                Mensagem *mensagemCliente = criaMensagemDoCliente();
                unsigned int tipo = leProtocoloMontaMensagem(mensagemCliente, buffer, &i, soquete);
                
                // ACKs e NAKs (mensagens de controle)
                if (tipo == 1 || tipo == 15) {
                    printf("Recebido %s para sequencia %d\n", tipo == 1 ? "AK" : "NAK", mensagemCliente->num_sequencia);
                    free(mensagemCliente->dados);
                    free(mensagemCliente);
                    continue;
                }

                // Lógica de Sequencialização para mensagens de DADOS do cliente
                if (mensagemCliente->num_sequencia == expected_seq_recv) {
                    printf("Mensagem recebida na sequencia correta: %d\n", expected_seq_recv);
                    expected_seq_recv = (expected_seq_recv + 1) % 64;
                    ack_counter++;
                    
                    if (ack_counter >= 10) {
                        printf("Enviando AK (cumulativo) para sequencia %d\n", (expected_seq_recv + 63) % 64);
                        enviarAK((expected_seq_recv + 63) % 64, soquete);
                        ack_counter = 0;
                    }

                    switch (tipo)
                    {
                    case 2:
                        meu_log("vizualização recebida");
                        enviarVisualizacao(soquete, gameState->labirinto);
                        break;
                    case 10:
                    case 11:
                    case 12:
                    case 13:
                        meu_log("movimentacao recebida");
                        movimentaPacMan(soquete, tipo, gameState->labirinto, gameState);
                        enviarVisualizacao(soquete, gameState->labirinto);
                        break;
                    default:
                        break;
                    }
                } else {
                    printf("Erro de sequencia! Esperado: %d, Recebido: %d. Enviando NAK.\n", 
                           expected_seq_recv, mensagemCliente->num_sequencia);
                    enviarNAK(expected_seq_recv, soquete);
                }

                free(mensagemCliente->dados);
                free(mensagemCliente);
            }
        }
    }
    return 0;
}

void enviarVisualizacao(int soquete, int labirinto[MAP_SIZE][MAP_SIZE])
{
    uint8_t total_data[MAP_SIZE * MAP_SIZE];
    for (int i = 0; i < MAP_SIZE; i++) {
        for (int j = 0; j < MAP_SIZE; j++) {
            total_data[i * MAP_SIZE + j] = (uint8_t)labirinto[i][j];
        }
    }

    int sent_bytes = 0;
    while (sent_bytes < MAP_SIZE * MAP_SIZE) {
        int chunk_size = (MAP_SIZE * MAP_SIZE - sent_bytes > 31) ? 31 : (MAP_SIZE * MAP_SIZE - sent_bytes);
        
        Mensagem *msg = criaMensagemDoServidor();
        msg->tipo = 2;
        msg->num_sequencia = next_seq_send;
        next_seq_send = (next_seq_send + 1) % 64;
        
        msg->tamanho = chunk_size;
        msg->dados = malloc(chunk_size);
        memcpy(msg->dados, total_data + sent_bytes, chunk_size);
        
        enviaMensagem(msg, soquete);
        
        free(msg->dados);
        free(msg);
        
        sent_bytes += chunk_size;
    }
}

void realizaMovimento(int labirinto[MAP_SIZE][MAP_SIZE], int novaPosX, int novaPosY, GameState *gameState)
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
            break;
        case 'X': // Parede
            printf("Parede bloqueando o movimento em (%d, %d)%s\n", posXVerificar, posYVerificar,
                   wrapAround ? " (wrap-around)" : "");
            break;
        case 'R': // Fantasma vermelho
            printf("Fantasma vermelho em (%d, %d)%s\n", posXVerificar, posYVerificar,
                   wrapAround ? " (wrap-around)" : "");
            break;
        case 'B': // Fantasma azul
            printf("Fantasma azul em (%d, %d)%s\n", posXVerificar, posYVerificar,
                   wrapAround ? " (wrap-around)" : "");
            break;
        case 'G': // Fantasma verde
            printf("Fantasma verde em (%d, %d)%s\n", posXVerificar, posYVerificar,
                   wrapAround ? " (wrap-around)" : "");
            break;
        case 'Y': // Fantasma amarelo
            printf("Fantasma amarelo em (%d, %d)%s\n", posXVerificar, posYVerificar,
                   wrapAround ? " (wrap-around)" : "");
            break;
        case '1': // Pastilha dourada arquivo texto (1.txt)
            printf("Pastilha dourada 1.txt em (%d, %d)%s\n", posXVerificar, posYVerificar,
                   wrapAround ? " (wrap-around)" : "");
            break;
        case '2': // Pastilha dourada arquivo texto (2.txt)
            printf("Pastilha dourada 2.txt em (%d, %d)%s\n", posXVerificar, posYVerificar,
                   wrapAround ? " (wrap-around)" : "");
            break;
        case '3': // Pastilha dourada arquivo jpg (3.jpg)
            printf("Pastilha dourada 3.jpg em (%d, %d)%s\n", posXVerificar, posYVerificar,
                   wrapAround ? " (wrap-around)" : "");
            break;
        case '4': // Pastilha dourada arquivo jpg (4.jpg)
            printf("Pastilha dourada 4.jpg em (%d, %d)%s\n", posXVerificar, posYVerificar,
                   wrapAround ? " (wrap-around)" : "");
            break;
        case '5': // Pastilha dourada arquivo mp4 (5.mp4)
            printf("Pastilha dourada 5.mp4 em (%d, %d)%s\n", posXVerificar, posYVerificar,
                   wrapAround ? " (wrap-around)" : "");
            break;
        case '6': // Pastilha dourada arquivo mp4 (6.mp4)
            printf("Pastilha dourada 6.mp4 em (%d, %d)%s\n", posXVerificar, posYVerificar,
                   wrapAround ? " (wrap-around)" : "");
            break;
        default:
            printf("Elemento desconhecido '%c' em (%d, %d)%s\n", elemento, posXVerificar, posYVerificar,
                   wrapAround ? " (wrap-around)" : "");
    }
}

void movimentaPacMan(int soquete, int tipo, int labirinto[MAP_SIZE][MAP_SIZE], GameState *gameState)
{
    int posXAtual = gameState->artefatosPosX[0];
    int posYAtual = gameState->artefatosPosY[0];
    int novaPosX = posXAtual;
    int novaPosY = posYAtual;

    switch(tipo) {
        case 10:
            novaPosX = posXAtual + 1;
            printf("Tentando mover para direita: (%d, %d) -> (%d, %d)\n", posXAtual, posYAtual, novaPosX, novaPosY);
            break;
        case 11:
            novaPosX = posXAtual - 1;
            printf("Tentando mover para esquerda: (%d, %d) -> (%d, %d)\n", posXAtual, posYAtual, novaPosX, novaPosY);
            break;
        case 12:
            novaPosY = posYAtual - 1;
            printf("Tentando mover para cima: (%d, %d) -> (%d, %d)\n", posXAtual, posYAtual, novaPosX, novaPosY);
            break;
        case 13:
            novaPosY = posYAtual + 1;
            printf("Tentando mover para baixo: (%d, %d) -> (%d, %d)\n", posXAtual, posYAtual, novaPosX, novaPosY);
            break;
    }

    realizaMovimento(labirinto, novaPosX, novaPosY, gameState);

    printf("Movimento processado!\n");

    Mensagem *mensagem = criaMensagemDoServidor();
    mensagem->num_sequencia = 1;
    mensagem->tamanho = 1;
    mensagem->tipo = 3;
    uint8_t dados[1] = {1};
    mensagem->dados = dados;

    enviaMensagem(mensagem, soquete);
}


int leProtocoloMontaMensagem(Mensagem *mensagem, unsigned char buffer[2048], int *i, int soquete)
{
    int offset = 1;
    uint8_t tamanho = buffer[*i + offset] >> 3;
    uint8_t numnum_sequencia = ((buffer[*i + offset] & 0x07) << 3) | (buffer[*i + offset + 1] >> 5);
    uint8_t tipo = buffer[*i + offset + 1] & 0x1F;
    mensagem->tamanho = tamanho;
    mensagem->num_sequencia = numnum_sequencia;
    mensagem->tipo = tipo;
    if (tamanho > 0)
    {
        mensagem->dados = malloc(tamanho);
        memcpy(mensagem->dados, &buffer[*i + offset + 2], tamanho);
    }
    (*i) += tamanho + 3; // Pula: Head(2) + Dados(t) + CRC(1).
    return tipo;
}

void meu_log(char* mensagem) {
    printf("%s\n", mensagem);
}
