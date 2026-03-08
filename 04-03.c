#include <stdio.h>
#include <stdlib.h>

/*
Exercise: Analyzing Floating-Point Representation in C 
Objective: Write a C program to dissect and print the components of a single-precision floating-point number according to the IEEE 754 standard. Your program should extract and display the sign bit, the exponent, and the fraction (mantissa) of the given floating-point number. 
 
Background: In the IEEE 754 standard for floating-point arithmetic, a single-precision floating-point number (float in C) is represented using 32 bits, divided into three parts: 
1 bit for the sign (0 for positive, 1 for negative), 
8 bits for the exponent, biased by 127, 
23 bits for the fraction (mantissa), representing the significant digits of the number. 
 
Requirements: 
Your program should accept a single floating-point number as input. You may hard-code this number into your program for simplicity. 
Implement a function named printFloatParts that takes a float as an argument and prints its sign, exponent (in biased form), and fraction (mantissa). 
The sign should be printed as an unsigned integer (0 or 1). 
The exponent should be printed as an unsigned integer in its biased form. 
The fraction should be printed as a hexadecimal number to represent the exact bits of the mantissa. 
 
You must use pointer casting to access the binary representation of the floating-point number as an unsigned 32-bit integer. Do not use unions or any form of array manipulation for this task. 
 
Ensure your program properly handles both positive and negative floating-point values. 
Hints: 
Remember that the sign bit is the most significant bit (MSB), the exponent follows the sign bit, and the fraction (mantissa) occupies the least significant 23 bits. 
Use bitwise operations to extract each component from the 32-bit representation. 
To access the binary representation, cast the address of the floating-point number to a pointer of type uint32_t and then dereference it. 
*/

void printFloatParts(float number) {
    // Cast the address of the float to a pointer to uint32_t and dereference it to get the binary representation
    uint32_t binaryRepresentation = *(uint32_t*)&number; 

    // Extract the sign bit (1 bit)
    uint32_t sign = (binaryRepresentation >> 31) & 0x1; 

    // Extract the exponent (8 bits)
    uint32_t exponent = (binaryRepresentation >> 23) & 0xFF; // 0xFF = 255 in decimal, which is 8 bits of 1s 11111111 in binary

    // Extract the fraction (mantissa) (23 bits)
    uint32_t fraction = binaryRepresentation & 0x7FFFFF;

    // -15.75 in binary is             1  10000010 -1111100 00000000 00000000
    // 0x7FFFFF =  which is 23 bits of 1s 00000000 01111111 11111111 11111111 

    // Print the results
    printf("Floating-point number: %f\n", number);
    printf("Sign bit: %u\n", sign);
    printf("Exponent (biased): %u\n", exponent);
    printf("Fraction (mantissa) in hexadecimal: 0x%X\n", fraction);
}

int main() {
    float number = -15.75; // You can change this to any float value you want to analyze

    printFloatParts(number);
    return 0;
}





