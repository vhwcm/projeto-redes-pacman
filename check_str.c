#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

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

        // Try to match "S*7 U"
        // In hex, 'S' is 53, '*' is 2a, '7' is 37, ' ' is 20, 'U' is 55
        for (int j = 0; j < len - 4; j++) {
            if (data[offset + j] == 'S' && data[offset + j + 1] == '*' && data[offset + j + 2] == '7' && data[offset + j + 3] == ' ' && data[offset + j + 4] == 'U') {
                printf("Found 'S*7 U' in absolute packet %d (seq %d)\n", i, i % 64);
            }
        }
    }
    free(data);
    return 0;
}
