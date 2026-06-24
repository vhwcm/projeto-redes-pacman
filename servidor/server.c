#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <linux/if_packet.h>
#include <net/ethernet.h>
#include <arpa/inet.h>
#include <unistd.h>

#include "labirinto.h"
#include "../rede.h"

#define ACK_TIMEOUT_MS 200

static long long timestamp_ms(void)
{
    struct timeval tp;
    gettimeofday(&tp, NULL);
    return (long long)tp.tv_sec * 1000 + tp.tv_usec / 1000;
}


void enviarVisualizacao(int soquete, char labirinto[MAP_SIZE][MAP_SIZE]);
void printaMensagem(Mensagem *mensagem);
char realizaMovimento(char labirinto[MAP_SIZE][MAP_SIZE], int novaPosX, int novaPosY, GameState *gameState);
void movimentaPacMan(int soquete, int tipo, char labirinto[MAP_SIZE][MAP_SIZE], GameState *gameState);
void movimentaFantasmas(int soquete, char labirinto[MAP_SIZE][MAP_SIZE], GameState *gameState);
void enviarArquivo(int soquete, const char *caminho, uint8_t tipo);
void enviaGameOver(int soquete);
// int leProtocoloMontaMensagem(Mensagem *mensagem, unsigned char buffer[2048], int *i, int soquete);
void meu_log(char *mensagem);

static uint8_t expected_seq_recv = 0;
static int ack_counter = 0;
static uint8_t seq_server = 0;

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        log_print("Uso: %s <nome_rede> [arquivo_labirinto]\n", argv[0]);
        return 1;
    }

    char *nome_rede = argv[1];

    if (strcmp(nome_rede, "lo") == 0) {
        modo_loopback = 1;
    }

    GameState *gameState = criaGameState();
    if (gameState == NULL) {
        log_print("Erro ao criar GameState\n");
        return 1;
    }

    if (argc == 3)
    {
        FILE *arquivoCSV = fopen(argv[2], "r");
        if (arquivoCSV == NULL)
        {
            log_print("Erro ao abrir arquivo\n");
            return 1;
        }
        carregaLabirinto(arquivoCSV, gameState->labirinto, gameState);
        fclose(arquivoCSV);
    }
    else if (argc == 2)
    {
        printa_labirinto(gameState->labirinto);
        log_print("recebendo\n");
    }
    else
    {
        log_print("argumentos incorretos\n", argv[0]);
        return 1;
    }
    unsigned char buffer[2048];
    for (int i = 0; i < 2048; i++) {
        buffer[i] = 0;
    }
    unsigned int soquete = cria_raw_socket(nome_rede);

    struct timeval tv;
    tv.tv_sec  = 0;
    tv.tv_usec = ACK_TIMEOUT_MS * 1000;
    setsockopt(soquete, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    long long ultimo_ack_ts = timestamp_ms();

    while (1)
    {

        struct sockaddr_ll from;
        socklen_t fromlen = sizeof(from);
        ssize_t bytes = recvfrom(soquete, buffer, sizeof(buffer), 0, (struct sockaddr *)&from, &fromlen);

        long long agora = timestamp_ms();
        if (agora - ultimo_ack_ts >= ACK_TIMEOUT_MS && ack_counter > 0) {
            log_print("Timeout de 200ms. Enviando ACK para seq %d\n", (expected_seq_recv + 63) % 64);
            enviarAK((expected_seq_recv + 63) % 64, soquete);
            ack_counter = 0;
            ultimo_ack_ts = agora;
        }

        if (bytes <= 0)
            continue;

        Mensagem *mensagemCliente = criaMensagem();
        if (desmontaMensagem((char *)buffer, mensagemCliente))
        {
            unsigned int tipo = mensagemCliente->tipo;
            
            if (modo_loopback) {
                if (tipo == 0 || tipo == 1 || tipo == 5 || tipo == 6 || tipo == 7 || tipo == 14 || tipo == 16) {
                    if (mensagemCliente->dados) free(mensagemCliente->dados);
                    free(mensagemCliente);
                    continue; // Nosso próprio pacote (eco do servidor)
                }
                if (tipo == 2 && mensagemCliente->tamanho > 0) {
                    if (mensagemCliente->dados) free(mensagemCliente->dados);
                    free(mensagemCliente);
                    continue; // Nosso próprio pacote (mapa enviado pelo servidor)
                }
            }

            // ACKs e NAKs (mensagens de controle)
            if (tipo == 0 || tipo == 1) {
                log_print("Recebido %s para sequencia %d\n", tipo == 0 ? "AK" : "NAK", mensagemCliente->num_sequencia);
                if (mensagemCliente->dados) free(mensagemCliente->dados);
                free(mensagemCliente);
                continue;
            }

            // Lógica de Sequencialização para mensagens de DADOS do cliente
            if (mensagemCliente->num_sequencia == expected_seq_recv) {
                log_print("Mensagem recebida na sequencia correta: %d\n", expected_seq_recv);
                expected_seq_recv = (expected_seq_recv + 1) % 64;
                ack_counter++;
                ultimo_ack_ts = timestamp_ms();

                if (ack_counter >= 4) {
                    log_print("Enviando AK (cumulativo) para sequencia %d\n", (expected_seq_recv + 63) % 64);
                    enviarAK((expected_seq_recv + 63) % 64, soquete);
                    ack_counter = 0;
                    ultimo_ack_ts = timestamp_ms();
                }

                switch (tipo)
                {
                case 3:
                    meu_log("vizualização recebida");
                    enviarVisualizacao(soquete, gameState->labirinto);
                    break;
                case 10:
                case 11:
                case 12:
                case 13:
                    meu_log("movimentacao recebida");
                    movimentaPacMan(soquete, tipo, gameState->labirinto, gameState);
                    movimentaFantasmas(soquete, gameState->labirinto, gameState);
                    enviarVisualizacao(soquete, gameState->labirinto);
                    break;
                default:
                    break;
                }
            } else {
                log_print("Erro de sequencia! Esperado: %d, Recebido: %d. Enviando NAK.\n", 
                       expected_seq_recv, mensagemCliente->num_sequencia);
                enviarNAK(expected_seq_recv, soquete);
            }

            if (mensagemCliente->dados) free(mensagemCliente->dados);
            free(mensagemCliente);
        } else {
            free(mensagemCliente);
        }
    }
    return 0;
}

void enviarVisualizacao(int soquete, char labirinto[MAP_SIZE][MAP_SIZE])
{
    uint8_t total_data[MAP_SIZE * MAP_SIZE];
    for (int i = 0; i < MAP_SIZE; i++) {
        for (int j = 0; j < MAP_SIZE; j++) {
            total_data[i * MAP_SIZE + j] = (uint8_t)labirinto[i][j];
        }
    }

    Mensagem *msg = criaMensagem();
    msg->tipo    = 2;
    msg->tamanho = MAP_SIZE * MAP_SIZE;
    msg->dados   = total_data;

    enviaMensagem(msg, soquete, &seq_server);

    free(msg);
}

void enviarArquivo(int soquete, const char *caminho, uint8_t tipo)
{
    FILE *f = fopen(caminho, "rb");
    if (!f) {
        log_print("Erro ao abrir arquivo: %s\n", caminho);
        return;
    }

    fseek(f, 0, SEEK_END);
    long tamanho = ftell(f);
    rewind(f);

    uint8_t *dados = malloc(tamanho);
    if (!dados) {
        fclose(f);
        return;
    }

    fread(dados, 1, tamanho, f);
    fclose(f);

    Mensagem *msg = criaMensagem();
    msg->tipo    = tipo;
    msg->tamanho = (uint32_t)tamanho;
    msg->dados   = dados;

    log_print("Enviando arquivo %s (%ld bytes) tipo=%d\n", caminho, tamanho, tipo);
    enviaMensagem(msg, soquete, &seq_server);

    free(dados);
    free(msg);
}

void enviaGameOver(int soquete)
{
    log_print("GAME OVER! Enviando tipo=14\n");
    Mensagem *msg = criaMensagem();
    msg->tipo    = 14;
    msg->tamanho = 0;
    msg->dados   = NULL;
    enviaMensagem(msg, soquete, &seq_server);
    free(msg);
}

char realizaMovimento(char labirinto[MAP_SIZE][MAP_SIZE], int novaPosX, int novaPosY, GameState *gameState)
{
    int posXVerificar = novaPosX;
    int posYVerificar = novaPosY;

    if (novaPosX < 0)         posXVerificar = MAP_SIZE - 1;
    else if (novaPosX >= MAP_SIZE) posXVerificar = 0;
    if (novaPosY < 0)         posYVerificar = MAP_SIZE - 1;
    else if (novaPosY >= MAP_SIZE) posYVerificar = 0;

    char elemento = labirinto[posXVerificar][posYVerificar];

    if (elemento == 'X' || elemento == 'P') {
        return elemento;
    }

    int pxAtual = gameState->artefatosPosX[0];
    int pyAtual = gameState->artefatosPosY[0];

    labirinto[pxAtual][pyAtual] = '0';
    labirinto[posXVerificar][posYVerificar] = 'P';
    gameState->artefatosPosX[0] = posXVerificar;
    gameState->artefatosPosY[0] = posYVerificar;

    switch (elemento) {
        case 'R': gameState->artefatosPosX[7] = -1; gameState->artefatosPosY[7] = -1; break;
        case 'G': gameState->artefatosPosX[8] = -1; gameState->artefatosPosY[8] = -1; break;
        case 'B': gameState->artefatosPosX[9] = -1; gameState->artefatosPosY[9] = -1; break;
        case 'Y': gameState->artefatosPosX[10] = -1; gameState->artefatosPosY[10] = -1; break;
        default: break;
    }

    return elemento;
}

void movimentaPacMan(int soquete, int tipo, char labirinto[MAP_SIZE][MAP_SIZE], GameState *gameState)
{
    int posXAtual = gameState->artefatosPosX[0];
    int posYAtual = gameState->artefatosPosY[0];
    int novaPosX  = posXAtual;
    int novaPosY  = posYAtual;

    switch (tipo) {
        case 10: novaPosY = posYAtual + 1; log_print("Movendo para DIREITA\n");   break;
        case 11: novaPosY = posYAtual - 1; log_print("Movendo para ESQUERDA\n");  break;
        case 12: novaPosX = posXAtual - 1; log_print("Movendo para CIMA\n");      break;
        case 13: novaPosX = posXAtual + 1; log_print("Movendo para BAIXO\n");     break;
    }

    char elem = realizaMovimento(labirinto, novaPosX, novaPosY, gameState);
    log_print("Movimento PacMan: elem='%c'\n", elem);

    if (elem == 'R' || elem == 'G' || elem == 'B' || elem == 'Y') {
        log_print("Pacman colidiu com o fantasma '%c'!\n", elem);
        enviarArquivo(soquete, "BOO.png", 8);
        usleep(100000);
        enviaGameOver(soquete);
        usleep(100000);
        exit(0);
    }

    switch (elem) {
        case '1': case '2': 
            log_print("pacman pegou pastilha TXT!\n");
            enviarArquivo(soquete, "livro.txt",          5); break;
        case '3': case '4': 
            log_print("pacman pegou pastilha JPG!\n");
            enviarArquivo(soquete, "naruto.jpg",          6); break;
        case '5': case '6': 
            log_print("pacman pegou pastilha MP4!\n");
            enviarArquivo(soquete, "naruto_video.mp4",    7); break;
        default: break;
    }

    if (elem >= '1' && elem <= '6') {
        int artefatosRestantes = 0;
        for (int i = 0; i < MAP_SIZE; i++) {
            for (int j = 0; j < MAP_SIZE; j++) {
                if (labirinto[i][j] >= '1' && labirinto[i][j] <= '6') {
                    artefatosRestantes++;
                }
            }
        }
        if (artefatosRestantes == 0) {
            log_print("Todos os aretefatos foram coletados, enviando sucesso(TIPO=9)\n");
            Mensagem *msg_vitoria = criaMensagem();
            msg_vitoria->tipo    = 9;
            msg_vitoria->tamanho = 0;
            msg_vitoria->dados   = NULL;
            enviaMensagem(msg_vitoria, soquete, &seq_server);
            free(msg_vitoria);
            usleep(100000);
            exit(0);
        }
    }
}

static const int DDX[4] = {-1,  0,  1,  0};
static const int DDY[4] = { 0,  1,  0, -1};

static int is_parede_fantasma(char c) {
    return c == 'X' || c == 'R' || c == 'G' || c == 'B' || c == 'Y'
        || (c >= '1' && c <= '6');
}

static char movimentaUmFantasma(char labirinto[MAP_SIZE][MAP_SIZE],
                                 int gsIdx, char simbolo,
                                 int priority[4], int *currentDir,
                                 GameState *gameState)
{
    if (gameState->artefatosPosX[gsIdx] < 0) return '0';

    int x = gameState->artefatosPosX[gsIdx];
    int y = gameState->artefatosPosY[gsIdx];

    int nx_reto = x + DDX[*currentDir];
    int ny_reto = y + DDY[*currentDir];
    int caminho_reto_livre =
        (nx_reto >= 0 && nx_reto < MAP_SIZE && ny_reto >= 0 && ny_reto < MAP_SIZE)
        && !is_parede_fantasma(labirinto[nx_reto][ny_reto]);

    if (caminho_reto_livre) {
        char dest = labirinto[nx_reto][ny_reto];
        labirinto[x][y] = '0';
        labirinto[nx_reto][ny_reto] = simbolo;
        gameState->artefatosPosX[gsIdx] = nx_reto;
        gameState->artefatosPosY[gsIdx] = ny_reto;
        return dest;
    }

    for (int i = 0; i < 4; i++) {
        int dir = priority[i];
        if (dir == *currentDir) continue;
        int nx  = x + DDX[dir];
        int ny  = y + DDY[dir];

        if (nx < 0 || nx >= MAP_SIZE || ny < 0 || ny >= MAP_SIZE) continue;
        if (is_parede_fantasma(labirinto[nx][ny])) continue;

        char dest = labirinto[nx][ny];
        labirinto[x][y] = '0';
        labirinto[nx][ny] = simbolo;
        gameState->artefatosPosX[gsIdx] = nx;
        gameState->artefatosPosY[gsIdx] = ny;
        *currentDir = dir;
        return dest;
    }

    return 'X';
}

void movimentaFantasmas(int soquete, char labirinto[MAP_SIZE][MAP_SIZE], GameState *gameState)
{
    static int dir_r = 1, dir_b = 3, dir_g = 0, dir_y = 0;
    static int verde_toggle = 0;

    int prio[4];

    prio[0] = (dir_r + 3) % 4; prio[1] = dir_r; prio[2] = (dir_r + 1) % 4; prio[3] = (dir_r + 2) % 4;
    if (movimentaUmFantasma(labirinto, 7, 'R', prio, &dir_r, gameState) == 'P') {
        log_print("FANTASMA 'R'\n");
        enviarArquivo(soquete, "BOO.png", 8); usleep(100000); enviaGameOver(soquete); usleep(100000); exit(0);
    }

    prio[0] = (dir_b + 1) % 4; prio[1] = dir_b; prio[2] = (dir_b + 3) % 4; prio[3] = (dir_b + 2) % 4;
    if (movimentaUmFantasma(labirinto, 9, 'B', prio, &dir_b, gameState) == 'P') {
        log_print("FANTASMA 'B'\n");
        enviarArquivo(soquete, "BOO.png", 8);
        usleep(100000);
        enviaGameOver(soquete);
        usleep(100000);
        exit(0);
    }

    if (verde_toggle == 0) {
        prio[0] = (dir_g + 1) % 4; prio[1] = dir_g; prio[2] = (dir_g + 3) % 4; prio[3] = (dir_g + 2) % 4;
    } else {
        prio[0] = (dir_g + 3) % 4; prio[1] = dir_g; prio[2] = (dir_g + 1) % 4; prio[3] = (dir_g + 2) % 4;
    }
    verde_toggle = !verde_toggle;
    if (movimentaUmFantasma(labirinto, 8, 'G', prio, &dir_g, gameState) == 'P') {
        log_print("FANTASMA 'G'\n");
        enviarArquivo(soquete, "BOO.png", 8); usleep(100000); enviaGameOver(soquete); usleep(100000); exit(0);
    }

    int shuffle[4] = {0, 1, 2, 3};
    for (int i = 3; i > 0; i--) {
        int j = rand() % (i + 1);
        int tmp = shuffle[i]; shuffle[i] = shuffle[j]; shuffle[j] = tmp;
    }
    if (movimentaUmFantasma(labirinto, 10, 'Y', shuffle, &dir_y, gameState) == 'P') {
        log_print("FANTASMA 'Y'\n");
        enviarArquivo(soquete, "BOO.png", 8); usleep(100000); enviaGameOver(soquete); usleep(100000); exit(0);
    }
}

void meu_log(char *mensagem) {
    log_print("%s\n", mensagem);
}

