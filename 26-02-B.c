#include <stdlib.h>
#include <stdio.h>

char a; 
char b; 
short int x; 
short int y; 
int w; 
int z;

int main() {

    printf("Address of (char) a is: %lx\n", &a);
    printf("Address of (char) b is: %lx\n", &b);

    printf("Address of (short int) x is: %lx\n", &x);
    printf("Address of (short int) y is: %lx\n", &y);

    printf("Address of (int) w is: %lx\n", &w);
    printf("Address of (int) z is: %lx\n", &z);

    /*
    Address of (char) a is: 102a10000
    Address of (char) b is: 102a10001
    Address of (short int) x is: 102a10008
    Address of (short int) y is: 102a1000a
    Address of (int) w is: 102a10004
    Address of (int) z is: 102a1000c
    */

}
