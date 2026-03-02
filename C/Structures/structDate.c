#include <stdio.h>
#include <stdlib.h>

struct Date {
    int day; 
    int month; 
    int year; 
};

struct MixedData {
    char a; 
    int b; 
    char c; 
}; 

int main() {
    struct Date today; 

    today.day = 2; 
    today.month = 3; 
    today.year = 2026; 

    printf("Today is the %d-%d-%d", today.day, today.month, today.year);

    struct MixedData data; 

    data.a = 'A'; 
    data.b = 69; 
    data.c = 'Z'; 

    printf("Mixed data insert are the %c-%d-%c", data.a, data.b, data.c);

    return 0; 
}