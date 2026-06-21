#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdarg.h>
#include <ctype.h>
#include <errno.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <pthread.h>
#include <linux/if_packet.h>
#include <net/ethernet.h>
#include <arpa/inet.h>

#include "../rede.h"

#define ACK_TIMEOUT_MS 200

static long long timestamp_ms(void)
{
    struct timeval tp;
    gettimeofday(&tp, NULL);
    return (long long)tp.tv_sec * 1000 + tp.tv_usec / 1000;
}

static int     soquete_global;
static uint8_t num_seq_send = 0;
static uint8_t expected_seq_recv = 0;

static void log_add(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    vprintf(fmt, ap);
    va_end(ap);
    printf("\n");
    fflush(stdout);
}

static void monitorar(int ms)
{
    unsigned char buffer[2048];

    struct timeval tv;
    tv.tv_sec  = 0;
    tv.tv_usec = 100 * 1000;
    setsockopt(soquete_global, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    long long inicio = timestamp_ms();
    while (timestamp_ms() - inicio < ms) {
        struct sockaddr_ll from;
        socklen_t fromlen = sizeof(from);
        ssize_t bytes = recvfrom(soquete_global, buffer, sizeof(buffer), 0, (struct sockaddr *)&from, &fromlen);

        if (bytes <= 0) continue;

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
                        if (verifica_crc8(&buffer[i + offset + 2], tamanho, crc)) {
                            encontrado = 1;
                        }
                    }
                }
            }

            if (!encontrado) continue;

            if (modo_loopback && pacotes_para_ignorar > 0) {
                pacotes_para_ignorar--;
                i += (buffer[i + 1] >> 3) + 3;
                continue;
            }

            int offset = 1;
            uint8_t tamanho = buffer[i + offset] >> 3;
            uint8_t seq     = ((buffer[i + offset] & 0x07) << 3) | (buffer[i + offset + 1] >> 5);
            uint8_t tipo    = buffer[i + offset + 1] & 0x1F;
            uint8_t crc     = buffer[i + offset + 2 + tamanho];

            char dados_hex[128] = "";
            for (int d = 0; d < tamanho && d < 16; d++) {
                char hex[5];
                snprintf(hex, sizeof(hex), "%02X ", buffer[i + offset + 2 + d]);
                strncat(dados_hex, hex, sizeof(dados_hex) - strlen(dados_hex) - 1);
            }

            log_add("[MONITOR] CRC=0x%02X | Tipo=%2d | Dados=[%s]", crc, tipo, dados_hex);

            if (tipo == 1 || tipo == 15) {
                log_add("[RECV] %s seq=%d", tipo == 1 ? "AK" : "NAK", seq);
            } else {
                if (seq == expected_seq_recv) {
                    log_add("[RECV] tipo=%2d seq=%2d tam=%d  dados=[%s]", tipo, seq, tamanho, dados_hex);
                    expected_seq_recv = (expected_seq_recv + 1) % 64;
                } else {
                    log_add("[ERR] Sequencia incorreta! Esperado=%d Recebido=%d", expected_seq_recv, seq);
                }
            }

            i += tamanho + 3;
        }
    }
}

static void envia_msg(int tipo, const char *dados_str)
{
    Mensagem msg;
    msg.tipo          = tipo;
    msg.num_sequencia = num_seq_send;
    num_seq_send      = (num_seq_send + 1) % 64;

    if (dados_str && strlen(dados_str) > 0) {
        int len      = strlen(dados_str);
        msg.tamanho  = len > TAM_MAXIMO_MENSAGEM ? TAM_MAXIMO_MENSAGEM : len;
        msg.dados    = (uint8_t *)dados_str;
    } else {
        msg.tamanho  = 0;
        msg.dados    = NULL;
    }

    char *raw = montaMensagem(&msg);

    log_add("[DBG]  send: fd=%d tipo=%d seq=%d tam=%d",
            soquete_global, tipo, (num_seq_send + 63) % 64, msg.tamanho);
    ssize_t sent = send(soquete_global, raw, TAM_MAXIMO_MENSAGEM + MIN_MENSAGE_SIZE, 0);
    if (modo_loopback) pacotes_para_ignorar++;
    free(raw);

    if (sent < 0) {
        log_add("[ERR] Falha no send: tipo=%d seq=%d errno=%d (%s)",
                tipo, (num_seq_send + 63) % 64, errno, strerror(errno));
    } else {
        log_add("[SENT] tipo=%2d seq=%2d tam=%d  dados=\"%s\"",
                tipo, (num_seq_send + 63) % 64, msg.tamanho,
                dados_str ? dados_str : "");
    }
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

    int repeticoes = 1;
    printf("Quantas vezes repetir o processo? ");
    fflush(stdout);
    if (scanf("%d", &repeticoes) != 1) {
        repeticoes = 1;
    }

    for (int r = 0; r < repeticoes; r++) {
        log_add("--- Ciclo %d/%d ---", r + 1, repeticoes);
        envia_msg(2, NULL);
        monitorar(10000);
    }

    close(soquete_global);
    return 0;
}
