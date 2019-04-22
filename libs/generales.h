//
// Created by utnso on 18/04/19.
//

#ifndef TP_2019_1C_SUCK_ET_ARRAYDESTRING_H
#define TP_2019_1C_SUCK_ET_GENERALES_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <commons/string.h>

typedef struct {
    bool valido;
    enum {
        SELECT,
        CREATE,
        DESCRIBE,
        INSERT,
        DROP
    } palabraClave;
    union {
        struct {
            char* nombreTabla;
            int key;
        } SELECT;
        struct {
            char* nombreTabla;
            char* tipoConsistencia;
            int numeroParticiones;
            u_int32_t tiempoCompactacion;
        } CREATE;
        struct {
            char* nombreTabla;
        } DESCRIBE;
        struct {
            char* nombreTabla;
            char* key;
            char** valor;
            timestamp timestamp;
        } INSERT;
        struct {
            char* nombreTabla;
        } DROP;
    } argumentos;
} tipoRequest

void printArrayDeStrings(char** arrayDeStrings);
int tamanioDeArrayDeStrings(char** arrayDeString);
char** parser(char* input);

#endif //TP_2019_1C_SUCK_ET_ARRAYDESTRING_H
