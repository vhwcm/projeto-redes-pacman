#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <linux/if_packet.h>
#include <linux/if_ether.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <unistd.h>
#include <sys/time.h>
#include <errno.h>

#include "rede.h"
#include <stdarg.h>

static FILE *log_file = NULL;

void init_log(const char *filename) {
    if (log_file) fclose(log_file);
    log_file = fopen(filename, "w");
}

void log_print(const char *format, ...) {
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);

    if (log_file) {
        va_start(args, format);
        vfprintf(log_file, format, args);
        va_end(args);
        fflush(log_file);
    }
}

int debug = 1;
int modo_loopback = 0;

long long timestamp_ms(void)
{
    struct timeval tp;
    gettimeofday(&tp, NULL);
    return (long long)tp.tv_sec * 1000 + tp.tv_usec / 1000;
}

uint8_t calcula_crc8(const uint8_t *dados, int tamanho)
{
    uint8_t crc = 0x00;
    for (int i = 0; i < tamanho; i++)
    {
        crc ^= dados[i];
        for (int j = 0; j < 8; j++)
        {
            if (crc & 0x80)
                crc = (crc << 1) ^ 0x07;
            else
                crc <<= 1;
        }
    }
    return crc;
}

int verifica_crc8(const uint8_t *dados, int tamanho, uint8_t crc_recebido)
{
    uint8_t crc_calc = calcula_crc8(dados, tamanho);
    return crc_calc == crc_recebido;
}

char *montaMensagem(Mensagem *mensagem)
{
    int tamanhoDados      = mensagem->tamanho;
    int tamanho_envio     = TAM_MAXIMO_MENSAGEM + MIN_MENSAGE_SIZE;

    unsigned char *protocolo = calloc(tamanho_envio, sizeof(unsigned char));
    protocolo[0] = MARCA_INICIO;
    protocolo[1] = (uint8_t)((tamanhoDados << 3) | ((mensagem->num_sequencia) >> 3));
    protocolo[2] = (uint8_t)(((mensagem->num_sequencia & 0x07) << 5) | (mensagem->tipo & 0x1F));
    for (int i = 0; i < tamanhoDados; i++)
    {
        protocolo[i + 3] = mensagem->dados[i];
    }
    uint8_t crc = calcula_crc8(mensagem->dados, tamanhoDados);
    protocolo[tamanhoDados + 3] = crc;

    return (char *)protocolo;
}

int desmontaMensagem(const char *mensagem, Mensagem *protocolo)
{
    if (mensagem[0] != MARCA_INICIO) {
        return 0;
    }

    protocolo->tamanho = (mensagem[1] >> 3) & 0x1F;
    protocolo->num_sequencia = ((mensagem[1] & 0x07) << 3) | ((mensagem[2] >> 5) & 0x07);
    protocolo->tipo = mensagem[2] & 0x1F;
    int tamanhoMensagem = protocolo->tamanho;
    protocolo->dados = malloc(tamanhoMensagem);
    for (int i = 0; i < tamanhoMensagem; i++)
    {
        protocolo->dados[i] = mensagem[i + 3];
    }
    unsigned int crc = (unsigned char)mensagem[tamanhoMensagem + 3];

    if (!verifica_crc8((uint8_t *)&mensagem[3], tamanhoMensagem, crc))
    {
         fprintf(stderr, "CRC inválido! Mensagem corrompida.\n");
        return 0;
    }

    return 1;
}

int cria_raw_socket(char* nome_interface_rede) {
    // Cria arquivo para o socket sem qualquer protocolo
    int soquete = socket(AF_PACKET, SOCK_DGRAM, htons(ETH_P_ALL));
    if (soquete == -1) {
        fprintf(stderr, "Erro ao criar socket: Verifique se você é root!\n");
        exit(-1);
    }
 
    int ifindex = if_nametoindex(nome_interface_rede);
 
    struct sockaddr_ll endereco = {0};
    endereco.sll_family = AF_PACKET;
    endereco.sll_protocol = htons(ETH_P_ALL);
    endereco.sll_ifindex = ifindex;
    // Inicializa socket
    if (bind(soquete, (struct sockaddr*) &endereco, sizeof(endereco)) == -1) {
        fprintf(stderr, "Erro ao fazer bind no socket\n");
        exit(-1);
    }
 
    struct packet_mreq mr = {0};
    mr.mr_ifindex = ifindex;
    mr.mr_type = PACKET_MR_PROMISC;
    // Não joga fora o que identifica como lixo: Modo promíscuo
    if (setsockopt(soquete, SOL_PACKET, PACKET_ADD_MEMBERSHIP, &mr, sizeof(mr)) == -1) {
        fprintf(stderr, "Erro ao fazer setsockopt: "
            "Verifique se a interface de rede foi especificada corretamente.\n");
        exit(-1);
    }
 
    return soquete;
}

static char* constroi_frame(Mensagem *mensagem, int frame_index, uint8_t seq_inicial, int num_frames) {
    Mensagem *frame = criaMensagem();
    if (frame_index < num_frames) {
        int offset = frame_index * TAM_MAXIMO_MENSAGEM;
        int chunkSize = mensagem->tamanho - offset;
        if (chunkSize > TAM_MAXIMO_MENSAGEM)
            chunkSize = TAM_MAXIMO_MENSAGEM;
        
        frame->tipo = mensagem->tipo;
        frame->num_sequencia = (seq_inicial + frame_index) % 64;
        frame->tamanho = chunkSize;
        frame->dados = mensagem->dados + offset;
    } else {
        frame->tipo = 16;
        frame->num_sequencia = (seq_inicial + frame_index) % 64;
        frame->tamanho = 0;
        frame->dados = NULL;
    }
    printaMensagem(frame);
    char *raw = montaMensagem(frame);
    free(frame);
    return raw;
}

void enviaMensagem(Mensagem *mensagem, int soquete, uint8_t *seq)
{
    int totalDados = mensagem->tamanho;
    int num_frames, frames_totais;
    if (totalDados == 0) {
        num_frames = 1;
        frames_totais = 1;
    } else {
        num_frames = (totalDados + TAM_MAXIMO_MENSAGEM - 1) / TAM_MAXIMO_MENSAGEM;
        frames_totais = num_frames + 1;
    }

    int base = 0;
    int proximo_envio = 0;
    uint8_t seq_inicial = *seq;
    int janela_tamanho = 5;

    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 200000;
    setsockopt(soquete, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(tv));

    while (base < frames_totais) {
        // verifica se há espaço na janela e não acabou os frames ainda.
        while (proximo_envio < base + janela_tamanho && proximo_envio < frames_totais) {
            char *raw = constroi_frame(mensagem, proximo_envio, seq_inicial, num_frames);
            int frameTam = TAM_MAXIMO_MENSAGEM + MIN_MENSAGE_SIZE;
            ssize_t sent = send(soquete, raw, frameTam, 0);
            if (sent < 0) perror("ERROR: enviaMensagem send");
            
            if (mensagem->tamanho > 0) {
                float pct = ((float)(proximo_envio + 1) / frames_totais) * 100.0f;
                log_print("Enviando pacote absoluto %d de %d (%.1f%% concluído)\n", proximo_envio + 1, frames_totais, pct);
            }
            
            free(raw);
            proximo_envio++;
        }

        char buffer[TAM_MAXIMO_MENSAGEM + MIN_MENSAGE_SIZE + 500];
        Mensagem protocolo;
        
        int bytes_recebidos = recv(soquete, buffer, TAM_MAXIMO_MENSAGEM + MIN_MENSAGE_SIZE + 50, 0);
        if (bytes_recebidos > 0) {
            if (desmontaMensagem(buffer, &protocolo)) {
                if (protocolo.tipo == 0) {
                    printf("ack recebido para a sequencia %d\n", protocolo.num_sequencia);
                    uint8_t ack_seq = protocolo.num_sequencia;
                    uint8_t seq_base = (seq_inicial + base) % 64;
                    int diff = (ack_seq - seq_base + 64) % 64;
                    if (diff >= 0 && diff < janela_tamanho) {
                        base = base + diff + 1;
                    }
                } else if (protocolo.tipo == 1) {
                    printf("ack recebido para a sequencia %d\n", protocolo.num_sequencia);
                    uint8_t nak_seq = protocolo.num_sequencia;
                    uint8_t seq_base = (seq_inicial + base) % 64;
                    int diff = (nak_seq - seq_base + 64) % 64;
                    if (diff >= 0 && diff < janela_tamanho) {
                        base = base + diff;
                        proximo_envio = base;
                    }
                }
                if (protocolo.dados) free(protocolo.dados);
            }
        } else {
            proximo_envio = base;
        }
    }
    
    *seq = (seq_inicial + frames_totais) % 64;
}



void printaMensagem(Mensagem *mensagem)
{
    log_print("=== MENSAGEM ===\n");
    log_print("Tipo: %d\n", mensagem->tipo);
    log_print("Num num_sequencia: %d\n", mensagem->num_sequencia);
    log_print("Tamanho: %d\n", mensagem->tamanho);

    if (mensagem->tamanho > 0 && mensagem->dados != NULL) {
        log_print("Dados: ");
        for (uint32_t i = 0; i < mensagem->tamanho; i++) {
            log_print("%02X ", mensagem->dados[i]);
        }
        log_print("\n");
        uint8_t crc = calcula_crc8(mensagem->dados, mensagem->tamanho);
        log_print("CRC: %02X\n", crc);
    } else {
        log_print("Dados: (vazio)\n");
        log_print("CRC: 00\n");
    }
    log_print("================\n");
}

void exibe_mensagem(Mensagem m)
{
    log_print("--- ITENS DA MENSAGEM ---\n");
    log_print("Tamanho: %u\n", m.tamanho);
    log_print("Num sequencia: %u\n", m.num_sequencia);
    log_print("Tipo: %u\n", m.tipo);

    if (m.tamanho > 0 && m.dados != NULL) {
        log_print("Dados: ");
        for (uint32_t i = 0; i < m.tamanho; i++) {
            log_print("%02X ", m.dados[i]);
        }
        log_print("\n");
        uint8_t crc = calcula_crc8(m.dados, m.tamanho);
        log_print("CRC: %02X\n", crc);
    } else {
        log_print("Dados: (vazio)\n");
        log_print("CRC: 00\n");
    }
    log_print("-------------------------\n");
}


void enviarAK(uint8_t seq, int soquete)
{
    Mensagem ack;
    ack.tipo          = 0;
    ack.num_sequencia = seq;
    ack.tamanho       = 0;
    ack.dados         = NULL;
    char *raw = montaMensagem(&ack);
    send(soquete, raw, TAM_MAXIMO_MENSAGEM + MIN_MENSAGE_SIZE, 0);

    free(raw);
}

void enviarNAK(uint8_t seq, int soquete)
{
    Mensagem nak;
    nak.tipo          = 1;
    nak.num_sequencia = seq;
    nak.tamanho       = 0;
    nak.dados         = NULL;
    char *raw = montaMensagem(&nak);
    send(soquete, raw, TAM_MAXIMO_MENSAGEM + MIN_MENSAGE_SIZE, 0);

    free(raw);
}

Mensagem *criaMensagem()
{
    Mensagem *mensagem = malloc(sizeof(Mensagem));
    mensagem->num_sequencia = 0;
    mensagem->tipo          = 0;
    mensagem->tamanho       = 0;
    mensagem->dados         = NULL;
    return mensagem;
}
