#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>

#define MARCA_INICIO 0b01111110
#define TAM_MAXIMO_MENSAGEM 31
#define MIN_MENSAGE_SIZE 4

typedef struct {
    uint32_t tamanho;
    uint8_t num_sequencia : 6;
    uint8_t tipo : 5;
    uint8_t *dados;
} Mensagem;

uint8_t calcula_crc8(const uint8_t *dados, int tamanho) {
    uint8_t crc = 0x00;
    for (int i = 0; i < tamanho; i++) {
        crc ^= dados[i];
        for (int j = 0; j < 8; j++) {
            if (crc & 0x80) crc = (crc << 1) ^ 0x07;
            else crc <<= 1;
        }
    }
    return crc;
}

int verifica_crc8(const uint8_t *dados, int tamanho, uint8_t crc_recebido) {
    return calcula_crc8(dados, tamanho) == crc_recebido;
}

char *montaMensagem(Mensagem *mensagem) {
    int tamanhoDados = mensagem->tamanho;
    int tamanho_envio = TAM_MAXIMO_MENSAGEM + MIN_MENSAGE_SIZE;
    unsigned char *protocolo = calloc(tamanho_envio, sizeof(unsigned char));
    protocolo[0] = MARCA_INICIO;
    protocolo[1] = (uint8_t)((tamanhoDados << 3) | ((mensagem->num_sequencia) >> 3));
    protocolo[2] = (uint8_t)(((mensagem->num_sequencia & 0x07) << 5) | (mensagem->tipo & 0x1F));
    for (int i = 0; i < tamanhoDados; i++) {
        protocolo[i + 3] = mensagem->dados[i];
    }
    uint8_t crc = calcula_crc8(mensagem->dados, tamanhoDados);
    protocolo[tamanhoDados + 3] = crc;
    return (char *)protocolo;
}

int desmontaMensagem(const char *mensagem, Mensagem *protocolo) {
    if (mensagem[0] != MARCA_INICIO) return 0;
    protocolo->tamanho = (mensagem[1] >> 3) & 0x1F;
    protocolo->num_sequencia = ((mensagem[1] & 0x07) << 3) | ((mensagem[2] >> 5) & 0x07);
    protocolo->tipo = mensagem[2] & 0x1F;
    int tamanhoMensagem = protocolo->tamanho;
    protocolo->dados = malloc(tamanhoMensagem);
    for (int i = 0; i < tamanhoMensagem; i++) {
        protocolo->dados[i] = mensagem[i + 3];
    }
    unsigned int crc = (unsigned char)mensagem[tamanhoMensagem + 3];
    if (!verifica_crc8((uint8_t *)&mensagem[3], tamanhoMensagem, crc)) {
        return 0; // Invalid
    }
    return 1; // Valid
}

int main() {
    srand(time(NULL));
    int errors = 0;
    for (int i = 0; i < 100000; i++) {
        Mensagem m;
        m.tamanho = rand() % 32;
        m.num_sequencia = rand() % 64;
        m.tipo = rand() % 32;
        m.dados = malloc(m.tamanho);
        for(int j=0; j<m.tamanho; j++) m.dados[j] = rand() % 256;
        
        char *raw = montaMensagem(&m);
        Mensagem p;
        if (!desmontaMensagem(raw, &p)) {
            errors++;
        }
        free(m.dados);
        free(p.dados);
        free(raw);
    }
    printf("Errors: %d\n", errors);
    return 0;
}
