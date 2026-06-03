#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdarg.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/socket.h>
#include <ncurses.h>
#include <pthread.h>
#include <linux/if_packet.h>
#include <net/ethernet.h>
#include <arpa/inet.h>

#include "../rede.h"

#define LOG_MAX   500
#define INPUT_MAX 256

static char   log_buf[LOG_MAX][256];
static int    log_count = 0;
static pthread_mutex_t log_lock = PTHREAD_MUTEX_INITIALIZER;
static volatile int precisa_redesenhar = 0;

static int     soquete_global;
static uint8_t num_seq_send = 0;
static uint8_t expected_seq_recv = 0;
static int     ack_counter = 0;

static WINDOW *win_log;
static WINDOW *win_status;
static WINDOW *win_input;

static void log_add(const char *fmt, ...)
{
    char buf[256];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);

    pthread_mutex_lock(&log_lock);
    if (log_count < LOG_MAX) {
        strncpy(log_buf[log_count++], buf, 255);
    } else {
        memmove(log_buf[0], log_buf[1], 255 * (LOG_MAX - 1));
        strncpy(log_buf[LOG_MAX - 1], buf, 255);
    }
    precisa_redesenhar = 1;
    pthread_mutex_unlock(&log_lock);
}

static void redesenha_log(void)
{
    int h, w;
    getmaxyx(win_log, h, w);
    werase(win_log);
    box(win_log, 0, 0);
    wattron(win_log, A_BOLD);
    mvwprintw(win_log, 0, 2, " Log de Mensagens ");
    wattroff(win_log, A_BOLD);

    pthread_mutex_lock(&log_lock);
    int start = log_count > (h - 2) ? log_count - (h - 2) : 0;
    for (int i = start; i < log_count; i++) {
        const char *line = log_buf[i];
        int attr = COLOR_PAIR(3);
        if      (strncmp(line, "[SENT]", 6) == 0) attr = COLOR_PAIR(1);
        else if (strncmp(line, "[RECV]", 6) == 0) attr = COLOR_PAIR(2);
        else if (strncmp(line, "[ERR]",  5) == 0) attr = COLOR_PAIR(4);
        wattron(win_log, attr);
        mvwprintw(win_log, i - start + 1, 1, "%.*s", w - 2, line);
        wattroff(win_log, attr);
    }
    pthread_mutex_unlock(&log_lock);
    wrefresh(win_log);
}

static void *thread_receber(void *arg)
{
    unsigned char buffer[2048];
    (void)arg;

    while (1) {
        struct sockaddr_ll from;
        socklen_t fromlen = sizeof(from);
        ssize_t bytes = recvfrom(soquete_global, buffer, sizeof(buffer), 0, (struct sockaddr *)&from, &fromlen);
        if (bytes <= 0) continue;

        // Ignora ecos de transmissão no loopback
        if (modo_loopback && from.sll_pkttype == PACKET_OUTGOING)
            continue;

        for (int i = 0; i < (int)bytes; i++) {
            int encontrado = 0;

            if ((unsigned char)buffer[i] == MARCA_INICIO) {
                int offset = 1;
                if (i + offset + 1 < (int)bytes) {
                    uint8_t tamanho = buffer[i + offset] >> 3;
                    if (i + offset + 2 + tamanho < (int)bytes) {
                        uint8_t crc = buffer[i + offset + 2 + tamanho];
                        if (verifica_crc8(&buffer[i + offset], tamanho + 2, crc)) {
                            encontrado = 1;
                        }
                    }
                }
            }

            if (!encontrado) continue;

            int offset = 1;
            uint8_t tamanho = buffer[i + offset] >> 3;
            uint8_t seq     = ((buffer[i + offset] & 0x07) << 3) | (buffer[i + offset + 1] >> 5);
            uint8_t tipo    = buffer[i + offset + 1] & 0x1F;

            // ACKs e NAKs são mensagens de controle, não incrementam sequência
            if (tipo == 1 || tipo == 15) {
                log_add("[RECV] %s seq=%d", tipo == 1 ? "AK" : "NAK", seq);
                i += tamanho + 3;
                continue;
            }

            if (seq == expected_seq_recv) {
                char dados_hex[128] = "";
                for (int d = 0; d < tamanho && d < 16; d++) {
                    char hex[5];
                    snprintf(hex, sizeof(hex), "%02X ", buffer[i + offset + 2 + d]);
                    strncat(dados_hex, hex, sizeof(dados_hex) - strlen(dados_hex) - 1);
                }

                log_add("[RECV] tipo=%2d seq=%2d tam=%d  dados=[%s]", tipo, seq, tamanho, dados_hex);
                
                expected_seq_recv = (expected_seq_recv + 1) % 64;
                ack_counter++;
                
                if (ack_counter >= 10) {
                    uint8_t ack_seq = (expected_seq_recv + 63) % 64;
                    enviarAK(ack_seq, soquete_global);
                    log_add("[SENT] AK cumulativo seq=%d", ack_seq);
                    ack_counter = 0;
                }
            } else {
                log_add("[ERR] Sequencia incorreta! Esperado=%d Recebido=%d. Enviando NAK.", expected_seq_recv, seq);
                enviarNAK(expected_seq_recv, soquete_global);
            }

            i += tamanho + 3;
        }
    }
    return NULL;
}

static void envia_msg(int tipo, const char *dados_str)
{
    Mensagem *msg = criaMensagemDoCliente();
    msg->tipo          = tipo;
    msg->num_sequencia = num_seq_send++;
    msg->num_sequencia %= 64;

    if (dados_str && strlen(dados_str) > 0) {
        int len       = strlen(dados_str);
        msg->tamanho  = len;
        msg->dados    = malloc(len);
        memcpy(msg->dados, dados_str, len);
    } else {
        msg->tamanho  = 1;
        msg->dados    = malloc(1);
        msg->dados[0] = 0;
    }

    enviaMensagem(msg, soquete_global);
    log_add("[SENT] tipo=%2d seq=%2d tam=%d  dados=\"%s\"",
            msg->tipo, msg->num_sequencia, msg->tamanho,
            dados_str ? dados_str : "");

    free(msg->dados);
    free(msg);
}

static void desenha_status(const char *nome_rede)
{
    werase(win_status);
    wbkgd(win_status, COLOR_PAIR(5));
    mvwprintw(win_status, 0, 1,
        "Interface: %-8s  modo: %-9s  | tipos: 2=viz  10=dir 11=esq 12=cima 13=baixo",
        nome_rede, modo_loopback ? "loopback" : "ethernet");
    mvwprintw(win_status, 1, 1,
        "Formato de entrada: <tipo> [dados]   Exemplos: '2'  '10'  '3 hello'   | q=sair");
    wrefresh(win_status);
}

int main(int argc, char *argv[])
{
    if (argc < 2) {
        fprintf(stderr, "Uso: %s <interface>\n", argv[0]);
        return 1;
    }

    char *nome_rede = argv[1];
    modo_loopback   = (strcmp(nome_rede, "lo") == 0);
    soquete_global  = cria_raw_socket(nome_rede);

    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    curs_set(1);

    if (has_colors()) {
        start_color();
        use_default_colors();
        init_pair(1, COLOR_GREEN,  -1);
        init_pair(2, COLOR_CYAN,   -1);
        init_pair(3, COLOR_WHITE,  -1);
        init_pair(4, COLOR_RED,    -1);
        init_pair(5, COLOR_BLACK, COLOR_CYAN);
        init_pair(6, COLOR_YELLOW, -1);
    }

    int rows, cols;
    getmaxyx(stdscr, rows, cols);

    int log_h = rows - 4;
    win_log    = newwin(log_h, cols, 0,        0);
    win_status = newwin(2,     cols, log_h,     0);
    win_input  = newwin(2,     cols, log_h + 2, 0);

    wtimeout(win_input, 100);

    desenha_status(nome_rede);

    pthread_t tid;
    pthread_create(&tid, NULL, thread_receber, NULL);
    pthread_detach(tid);

    log_add("Pronto! Conectado em '%s'. Digite mensagens abaixo.", nome_rede);
    redesenha_log();

    char input[INPUT_MAX] = {0};
    int  input_len = 0;

    while (1) {
        if (precisa_redesenhar) {
            precisa_redesenhar = 0;
            redesenha_log();
        }

        werase(win_input);
        box(win_input, 0, 0);
        wattron(win_input, A_BOLD);
        mvwprintw(win_input, 0, 2, " Entrada ");
        wattroff(win_input, A_BOLD);
        wattron(win_input, COLOR_PAIR(6));
        mvwprintw(win_input, 1, 1, "> %s", input);
        wattroff(win_input, COLOR_PAIR(6));
        wmove(win_input, 1, 3 + input_len);
        wrefresh(win_input);

        int ch = wgetch(win_input);
        if (ch == ERR) continue;

        if (ch == 'q' && input_len == 0) break;

        if (ch == '\n' || ch == KEY_ENTER) {
            if (input_len > 0) {
                int  tipo = 0;
                char dados[INPUT_MAX] = {0};
                int  n = sscanf(input, "%d %[^\n]", &tipo, dados);
                if (n >= 1) {
                    envia_msg(tipo, n >= 2 ? dados : NULL);
                } else {
                    log_add("[ERR] Formato invalido. Use: <tipo> [dados]");
                }
                memset(input, 0, sizeof(input));
                input_len = 0;
                redesenha_log();
            }
        } else if (ch == KEY_BACKSPACE || ch == 127 || ch == '\b') {
            if (input_len > 0) input[--input_len] = '\0';
        } else if (isprint(ch) && input_len < INPUT_MAX - 1) {
            input[input_len++] = (char)ch;
            input[input_len]   = '\0';
        }
    }

    endwin();
    close(soquete_global);
    return 0;
}
