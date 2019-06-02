//
// Created by utnso on 18/04/19.
//

#ifndef TP_2019_1C_SUCK_ET_ARRAYDESTRING_H
#define TP_2019_1C_SUCK_ET_GENERALES_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <commons/string.h>

typedef enum {
    SELECT,
    INSERT,
    CREATE,
    DROP,
    DESCRIBE,
    JOURNAL,
    ADD,
    RUN,
    METRICS,
    HELP,
    EXIT,
    INVALIDO,
} TipoRequest;

typedef struct{
    TipoRequest tipoRequest;
    char** parametros;
}t_comando;

void printArrayDeStrings(char** arrayDeStrings);
int tamanioDeArrayDeStrings(char** arrayDeString);
char** parser(char* input);
int pesoString(char*);

#endif //TP_2019_1C_SUCK_ET_ARRAYDESTRING_H