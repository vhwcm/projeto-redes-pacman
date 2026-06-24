#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

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

int main() {
    FILE *f = fopen("servidor/naruto_video.mp4", "rb");
    if (!f) return 1;
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    rewind(f);
    uint8_t *data = malloc(size);
    fread(data, 1, size, f);
    fclose(f);

    int packet_size = 31;
    for (int i = 0; i < 96000; i++) {
        int offset = i * packet_size;
        int len = 31;
        uint8_t crc = calcula_crc8(&data[offset], len);
        if (crc == 0xd0) {
            printf("Packet %d has CRC d0 (seq %d)\n", i, i % 64);
        }
    }
    free(data);
    return 0;
}
