#include <stdio.h>
#include <stdlib.h>

struct MyStructure {
    int num; 
    char letter;
    char string[20];
};

int main() {
    struct MyStructure myStruct;

    myStruct.num = 1234; 
    myStruct.letter = 'A';

    printf("Number: %d\n", myStruct.num);
    printf("Letter: %c\n", myStruct.letter);

    // myStruct.string = "Hello, World!"; -> not allowed, arrays cannot be assigned directly

    strcpy(myStruct.string, "Hello, World!"); // Use strcpy to copy string into the array

    printf("String: %s\n", myStruct.string);

    //Simpler syntax
    struct MyStructure anotherStruct = {5678, 'B', "Another String"};

    printf("Another Number: %d\nAnother Letter: %c\nAnother String: %s\n", anotherStruct.num, anotherStruct.letter, anotherStruct.string);

    struct MyStructure str1 = {1, 'X', "First"};
    struct MyStructure str2; 

    str2 = str1; // This is allowed, it copies the contents of str1 to str2 

    printf("Copied Structure: Number: %d, Letter: %c, String: %s\n", str2.num, str2.letter, str2.string);

    // Modify values

    str2.num = 9999;
    str2.letter = 'Z';
    strcpy(str2.string, "Modified String");

    printf("Modified Structure: Number: %d, Letter: %c, String: %s\n", str2.num, str2.letter, str2.string);

    return 0;
}

