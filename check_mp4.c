#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

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
    int num_packets = (size + packet_size - 1) / packet_size;
    for (int i = 0; i < num_packets; i++) {
        int offset = i * packet_size;
        int len = size - offset;
        if (len > packet_size) len = packet_size;

        if (len >= 11) { // We need at least 11 bytes to check dados[9] and dados[10]
            uint8_t eth_type_hi = data[offset + 9];
            uint8_t eth_type_lo = data[offset + 10];
            
            // For example, 0x0800 is IPv4
            if (eth_type_hi == 0x08 && eth_type_lo == 0x00) {
                printf("Packet absolute %d (seq %d) has EtherType 0x0800!\n", i, i % 64);
            }
        }
        
        // Let's specifically print packet absolute 20
        if (i == 20) {
            printf("Packet 20 bytes 9-10: %02x %02x\n", data[offset + 9], data[offset + 10]);
        }
    }
    free(data);
    return 0;
}
