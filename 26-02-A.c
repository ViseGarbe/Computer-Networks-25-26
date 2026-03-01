#include <stdio.h>
#include <stdlib.h>

int x;
int y; 
int z; 

char factorial(char x) {

    char *y = malloc(1);
    printf("\nAddress of y is %lx", y);

    printf("\nAddress of x is %lx\n", &x);
    if (x == 1) {
        return 1;
    } else {
        return x * factorial(x - 1);
    }
    /*
        Address of n is 16b28eafe
        Address of n is 16b28eade
        Address of n is 16b28eabe
        Address of n is 16b28ea9e
        Address of n is 16b28ea7e
        Factorial of 5 is 120
    */
}

int main() {
    printf("Address of x is %lx\n", &x);
    printf("Address of y is %lx\n", &y);
    printf("Address of z is %lx\n", &z);

    /*
    Increased by 4, really far away from the area of store of the local variable 
        Address of x is 1020e4000
        Address of y is 1020e4004
        Address of z is 1020e4008
    */
    
    printf("\nFactorial of 5 is %d\n", factorial(5));


    //Dynamic allocated memry : 
    
}