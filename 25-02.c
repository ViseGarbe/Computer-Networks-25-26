// 25-02 Computer Networks

#include <stdio.h>
#include <stdlib.h>

int main() {
    char a; // A char is a byte -> string of 8 bit nothing else
    char b; 
    char *p; // It is a varibale where you store the address of another variable
    char c[5] = {'H','B','L','L','O'}; 

    a = 'F';

    // C langugae is completly agnostic about variable a. 

    printf("Variable a contains %c\n", a); //You can interpret those bits as a char

    printf("Variable a contains %d\n", a); //You can interpret those bits as a number

    printf("Variable a contains %x\n", a); 
    
    printf("------> %lx\n", &a); // Memory address of the variable a
    // It seems useless in this case, but normally it is really usefull 


    printf("\nVariable a is in the memory cell #%lx\n", &a); // Memory address of the varialbe a

    p = &a; 

    printf("Variable p contains cell address #%lx\n", p); // Memory address of the varialbe a
    printf("Variable p is in the memory cell #%lx\n", &p); // Memory address of the varialbe a

    printf("Variable p can watch variable a value: %c\n", *p); 

    *p = 'G'; // P can change the value of the variable a ---> that can be dangerous

    printf("Variable p can change variable a value: %c\n", a); 

    // This is just the beginning of the problem

    b = 'H'; 

    printf("\nVariable b contains %c\n", b); 

    printf("Variable b is in the memory cell #%lx\n", &b); 

    *(p-1) = 'Z'; // --> Tendenzialmente pericoloso

    printf("Variable b is a little confused... value is %c\n", b); 

    printf("\nThe value of the first two elements of c[] are %c and %c\n", c[0], c[1]); 

    p = &c[0];

    printf("p is accessing to the cell of the first two elements of c[]: #%lx and #%lx\n", p, (p+1)); 

    printf("p is accessing to the value of the first two elements of c[]: %c and %c \n", *p, *(p+1)); 

    // Using the pointer with plus of minus we can watch all the cell of the memory 

    p = c; // Not normal, but the array name is a pointer to the first cell of himself, so it is the same of writing p = & c[0]

    printf("p is accessing to the value with sintax p[0] and p[1] to the value of the first two elements of c[]: %c and %c\n", p[0], p[1]); 

    // The pointer p and the array are the same thing? 

    printf("\nThe value the non-elements c[5] is %c -----> Varaible c has no acess over his size\n", c[5]);

    //c = &a; //---> generates an error 


    char *ptr[5]; // -> 



}