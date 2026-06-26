#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <termios.h>
#include <linux/if_packet.h>
#include <net/ethernet.h>
#include <arpa/inet.h>

#include "../rede.h"

#define MAP_SIZE      40
#define TIMEOUT_RESP  5000

#define R   "\x1b[0m"
#define BD  "\x1b[1m"
#define CY  "\x1b[36m"
#define RD  "\x1b[91m"
#define GR  "\x1b[92m"
#define YL  "\x1b[93m"
#define BL  "\x1b[94m"
#define WH  "\x1b[97m"
#define GY  "\x1b[90m"
#define BG_W "\x1b[47m"
#define BG_K "\x1b[40m"

static int           soquete_global;
static uint8_t       num_seq_send = 0;
static struct termios term_orig;
static int           term_active = 0;

static long long ts_ms(void) {
    struct timeval tp;
    gettimeofday(&tp, NULL);
    return (long long)tp.tv_sec * 1000 + tp.tv_usec / 1000;
}

static void term_restore(void) {
    if (term_active) {
        tcsetattr(STDIN_FILENO, TCSANOW, &term_orig);
        term_active = 0;
    }
}

static void term_raw(void) {
    tcgetattr(STDIN_FILENO, &term_orig);
    atexit(term_restore);
    struct termios raw = term_orig;
    raw.c_lflag &= ~(ICANON | ECHO);
    raw.c_cc[VMIN]  = 1;
    raw.c_cc[VTIME] = 0;
    tcsetattr(STDIN_FILENO, TCSANOW, &raw);
    term_active = 1;
}

static char getch(void) {
    char c;
    if (read(STDIN_FILENO, &c, 1) < 0) return 0;
    return c;
}

static void envia_tipo(uint8_t tipo) {
    Mensagem msg;
    msg.tipo          = tipo;
    msg.num_sequencia = num_seq_send;
    num_seq_send      = (num_seq_send + 1) % 64;
    msg.tamanho       = 0;
    msg.dados         = NULL;
    char *raw = montaMensagem(&msg);
    send(soquete_global, raw, TAM_MAXIMO_MENSAGEM + MIN_MENSAGE_SIZE, 0);
    free(raw);
}

static int recebe_mensagem(int timeout_ms, uint8_t *tipo_out, uint8_t **dados_out, uint32_t *tam_out) {
    unsigned char buf[4096];
    uint8_t *acum      = NULL;
    uint32_t acum_tam  = 0;
    uint32_t acum_cap  = 0;
    uint8_t  acum_tipo = 0;
    uint8_t  last_seq  = 0xFF;

    struct timeval tv = {0, 100 * 1000};
    setsockopt(soquete_global, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    long long inicio = ts_ms();
    while (ts_ms() - inicio < timeout_ms) {
        ssize_t bytes = recv(soquete_global, buf, sizeof(buf), 0);
        if (bytes <= 0) continue;
        
        // Atualiza o timer para nao dar timeout enquanto estiver ativamente recebendo pedacos grandes (como um MP4)
        inicio = ts_ms();
        // if (modo_loopback && from.sll_pkttype == PACKET_OUTGOING) continue;

        for (int i = 0; i < (int)bytes; i++) {
            if ((unsigned char)buf[i] != MARCA_INICIO) continue;

            if (i + 3 >= (int)bytes) continue;
            uint8_t tam  = buf[i + 1] >> 3;
            uint8_t seq  = ((buf[i + 1] & 0x07) << 3) | (buf[i + 2] >> 5);
            uint8_t tipo = buf[i + 2] & 0x1F;
            

            if (i + 3 + (int)tam >= (int)bytes) { i += tam + 3; continue; }

            uint8_t crc_r = buf[i + 3 + tam];
            if (!verifica_crc8(&buf[i + 3], tam, crc_r)) { i += tam + 3; continue; }

            // Filtra os próprios pacotes em loopback com base no tipo
            if (modo_loopback) {
                if (tipo >= 10 && tipo <= 13) { i += tam + 3; continue; } // Eco do nosso movimento
                if (tipo == 2 && tam == 0)    { i += tam + 3; continue; } // Eco do nosso pedido de mapa
            }

            if (tipo == 16) {
                *tipo_out  = acum_tipo;
                *dados_out = acum;
                *tam_out   = acum_tam;
                return 1;
            }

            if (tam > 0) {
                if (seq != last_seq) {
                    last_seq = seq;
                    if (acum_tipo == 0) acum_tipo = tipo;
                    if (acum_tam + tam > acum_cap) {
                        acum_cap = (acum_cap == 0) ? 1024 : acum_cap * 2;
                        if (acum_cap < acum_tam + tam) acum_cap = acum_tam + tam;
                        acum = realloc(acum, acum_cap);
                    }
                    memcpy(acum + acum_tam, &buf[i + 3], tam);
                    acum_tam += tam;
                }
            }

            i += tam + 3;
        }
    }

    free(acum);
    *tipo_out = 0; *dados_out = NULL; *tam_out = 0;
    return 0;
}

static void exibe_mapa(uint8_t *dados, uint32_t tam) {
    if (tam < (uint32_t)(MAP_SIZE * MAP_SIZE)) {
        log_print(RD "Mapa incompleto (%u bytes)\n" R, tam);
        return;
    }
    log_print("\x1b[2J\x1b[H");
    for (int i = 0; i < MAP_SIZE; i++) {
        log_print(" ");
        for (int j = 0; j < MAP_SIZE; j++) {
            char c = (char)dados[i * MAP_SIZE + j];
            switch (c) {
                case 'X': log_print(BD BG_W WH "  " R); break;
                case 'P': log_print(BD YL " C" R);       break;
                case 'R': log_print(BD RD " R" R);       break;
                case 'G': log_print(BD GR " G" R);       break;
                case 'B': log_print(BD BL " B" R);       break;
                case 'Y': log_print(BD YL " Y" R);       break;
                case '1': case '2':
                case '3': case '4':
                case '5': case '6': log_print(BD YL " *" R); break;
                default:  log_print(GY "  " R);          break;
            }
        }
        log_print("\n");
    }
    log_print("\n");
    log_print(BD CY "  ╔══════════════════════════════════╗\n" R);
    log_print(BD CY "  ║       CONTROLES DO PACMAN        ║\n" R);
    log_print(BD CY "  ╠══════════════════════════════════╣\n" R);
    log_print(BD CY "  ║  " R BD WH "W" R CY " = Cima     " R BD WH "S" R CY " = Baixo     " BD CY "║\n" R);
    log_print(BD CY "  ║  " R BD WH "A" R CY " = Esquerda  " R BD WH "D" R CY " = Direita   " BD CY "║\n" R);
    log_print(BD CY "  ║  " R BD WH "Q" R CY " = Sair                      " BD CY "║\n" R);
    log_print(BD CY "  ╚══════════════════════════════════╝\n" R);
    log_print("\n" BD "> " R);
    fflush(stdout);
}

static void exibe_txt(uint8_t *dados, uint32_t tam) {
    const char *tmp = "/tmp/pacman_txt.txt";
    FILE *f = fopen(tmp, "wb");
    if (!f) return;
    fwrite(dados, 1, tam, f);
    fclose(f);

    term_restore();
    log_print(BD YL "\n╔═══════════════════════╗\n║  🏆 PASTILHA TXT!  ║\n╚═══════════════════════╝\n" R);
    fflush(stdout);
    char cmd[256];
    snprintf(cmd, sizeof(cmd), "less '%s'", tmp);
    system(cmd);
    term_raw();
}

static void exibe_jpg(uint8_t *dados, uint32_t tam) {
    const char *tmp = "/tmp/pacman_img.jpg";
    FILE *f = fopen(tmp, "wb");
    if (!f) return;
    fwrite(dados, 1, tam, f);
    fclose(f);

    log_print(BD YL "\n╔═══════════════════════╗\n║  🏆 PASTILHA JPG!  ║\n╚═══════════════════════╝\n" R);
    fflush(stdout);

    char cmd[256];
    if (system("which feh > /dev/null 2>&1") == 0)
        snprintf(cmd, sizeof(cmd), "feh '%s' > /dev/null 2>&1 &", tmp);
    else if (system("which eog > /dev/null 2>&1") == 0)
        snprintf(cmd, sizeof(cmd), "eog '%s' > /dev/null 2>&1 &", tmp);
    else if (system("which display > /dev/null 2>&1") == 0)
        snprintf(cmd, sizeof(cmd), "display '%s' > /dev/null 2>&1 &", tmp);
    else {
        log_print(WH "Imagem salva em: %s\n" R, tmp);
        return;
    }
    system(cmd);
}

static void exibe_mp4(uint8_t *dados, uint32_t tam) {
    const char *tmp = "/tmp/pacman_video.mp4";
    FILE *f = fopen(tmp, "wb");
    if (!f) return;
    fwrite(dados, 1, tam, f);
    fclose(f);

    log_print(BD YL "\n╔═══════════════════════╗\n║  🏆 PASTILHA MP4!  ║\n╚═══════════════════════╝\n" R);
    log_print("[DIAG] Bytes recebidos: %u\n", tam);
    {
        FILE *orig = fopen("naruto_video.mp4", "rb");
        if (orig) {
            fseek(orig, 0, SEEK_END);
            long orig_sz = ftell(orig);
            fclose(orig);
            log_print("[DIAG] Tamanho original: %ld bytes — %s\n", orig_sz,
                   (long)tam == orig_sz ? "OK (sem perda)" : "DIVERGENTE (perda de pacotes!)");
        }
    }
    fflush(stdout);

    char cmd[256];
    if (system("which mpv > /dev/null 2>&1") == 0)
        snprintf(cmd, sizeof(cmd), "mpv '%s' > /dev/null 2>&1 &", tmp);
    else if (system("which vlc > /dev/null 2>&1") == 0)
        snprintf(cmd, sizeof(cmd), "vlc '%s' > /dev/null 2>&1 &", tmp);
    else {
        log_print(WH "Vídeo salvo em: %s\n" R, tmp);
        return;
    }
    system(cmd);
}

static int processa(uint8_t tipo, uint8_t *dados, uint32_t tam) {
    switch (tipo) {
        case 2:  exibe_mapa(dados, tam); return 1;
        case 5:  exibe_txt(dados, tam);  return 0;
        case 6:  exibe_jpg(dados, tam);  return 0;
        case 7:  exibe_mp4(dados, tam);  return 0;
        case 9:
            term_restore();
            log_print(BD GR "\n╔══════════════════════════╗\n║    🎉 VOCÊ VENCEU! 🎉    ║\n╚══════════════════════════╝\n" R "\n");
            fflush(stdout);
            return -1;
        case 14:
            term_restore();
            log_print(BD RD "\n╔══════════════════════╗\n║     GAME OVER! 💀    ║\n╚══════════════════════╝\n" R "\n");
            fflush(stdout);
            return -1;
        default:
            /* Ignora mensagens de controle ou eco de loopback silenciosamente */
            return 0;
    }
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Uso: %s <interface> [--raw]\n", argv[0]);
        return 1;
    }

    int use_sock_raw = 0;
    if (argc > 1 && strcmp(argv[argc-1], "--raw") == 0) {
        use_sock_raw = 1;
        argc--;
    }

    char *iface = argv[1];
    modo_loopback  = (strcmp(iface, "lo") == 0);
    soquete_global = cria_raw_socket(iface, use_sock_raw);

    int rcvbuf = 8 * 1024 * 1024;
    setsockopt(soquete_global, SOL_SOCKET, SO_RCVBUF, &rcvbuf, sizeof(rcvbuf));

    envia_tipo(2);

    uint8_t tipo; uint8_t *dados; uint32_t tam;
    if (recebe_mensagem(TIMEOUT_RESP, &tipo, &dados, &tam)) {
        processa(tipo, dados, tam);
        free(dados);
    }

    term_raw();

    while (1) {
        char c = getch();
        if (c == 'q' || c == 'Q') break;

        uint8_t cmd = 0;
        switch (c) {
            case 'w': case 'W': cmd = 12; break;
            case 's': case 'S': cmd = 13; break;
            case 'a': case 'A': cmd = 11; break;
            case 'd': case 'D': cmd = 10; break;
            default: continue;
        }

        envia_tipo(cmd);

        int got_map = 0;
        long long t0 = ts_ms();
        while (!got_map && ts_ms() - t0 < 2000) {
            uint8_t t; uint8_t *d; uint32_t s;
            if (!recebe_mensagem(2000   , &t, &d, &s)) continue;
            int r = processa(t, d, s);
            free(d);
            if (r == 1) got_map = 1;
            if (r == -1) { term_restore(); close(soquete_global); return 0; }
        }
    }

    term_restore();
    close(soquete_global);
    return 0;
}
