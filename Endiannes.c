#include <stdio.h>
#include <stdlib.h>

void Endiannes(unsigned int x) {
    if (*((unsigned char*)&x)) {
        printf("Little Endian\n");
    } else {
        printf("Big Endian\n");
    }
    for (int i = 0; i < 4; i++) {
        printf("%02x ", *((unsigned char*)&x + i));
    }
    printf("\n");
}

int main() {
    unsigned int x = 1;
    printf("Testing with value: %u\n", x);
    Endiannes(x);
    x = 16777216;
    printf("Testing with value: %u\n", x);
    Endiannes(x);
    return 0;
}