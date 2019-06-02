/*
 ============================================================================
 Name        : Generales.c
 Author      : Suck et
 	 UTN FRBA - Sistemas Operativos
 	 	 TP-1C-2019-Lissandra
 ============================================================================
 */

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

int pesoString(char* string)    {
    return string == NULL? 0 : sizeof(char) * (strlen(string) + 1);
}

char** parser(char* input){
    return string_split(input, " ");
}
