#include <stdio.h>
#include <stdlib.h>

int isLittleEndian(unsigned int x) {
    return *((unsigned char*)&x);
}

int main() {
    unsigned int x = 16777216;

    printf("Value of x (unsigned int) is %u\n", x);
    if (isLittleEndian(x))
        printf("Little Endian\n");
    else
        printf("Big Endian\n");

    x = 1;
    printf("Value of x (unsigned int) is %u\n", x);
    if (isLittleEndian(x))
        printf("Little Endian\n");
    else
        printf("Big Endian\n");

    return 0;
}