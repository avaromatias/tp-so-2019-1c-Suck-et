//
// Created by utnso on 18/04/19.
// Funciones de uso general - abstracciones utiles
//

#include "generales.h"

void printArrayDeStrings(char** arrayDeStrings){

    int count=0;
    while (arrayDeStrings[count] != NULL) {
        for (int i = 0; i <strlen(arrayDeStrings[count]) ; ++i) {
            printf("%c", arrayDeStrings[count][i]);

        }
        printf("\n");
        count++;
    }
}
int tamanioDeArrayDeStrings(char** arrayDeString){

    int count=0;
    while (arrayDeString[count] != NULL) {
        count++;
    }
    printf("Cantidad: %i\n", count);
}

char** parser(char* input){
    return string_split(input, " ");
}