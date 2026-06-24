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
    int offset = 1507 * packet_size;
    
    printf("Bytes 9-10: %02x %02x\n", data[offset+9], data[offset+10]);
    free(data);
    return 0;
}
