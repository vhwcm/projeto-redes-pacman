#include <stdio.h>
#include <stdint.h>

uint8_t calc_unsigned(const uint8_t *dados, int tamanho) {
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

uint8_t calc_signed(const char *dados, int tamanho) {
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
    uint8_t test[] = {0xFF, 0x80, 0x7F, 0x00};
    printf("Unsigned: %x\n", calc_unsigned(test, 4));
    printf("Signed: %x\n", calc_signed((const char*)test, 4));
    return 0;
}
