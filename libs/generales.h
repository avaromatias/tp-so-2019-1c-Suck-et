//
// Created by utnso on 18/04/19.
//

#ifndef TP_2019_1C_SUCK_ET_ARRAYDESTRING_H
#define TP_2019_1C_SUCK_ET_GENERALES_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <libgen.h>
#include <sys/stat.h>
#include <sys/types.h>
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
    RESPUESTA_GOSSIPING,
    RESPUESTA_GOSSIPING_2
} TipoRequest;

typedef struct{
    TipoRequest tipoRequest;
    char** parametros;
    int cantidadParametros;
} t_comando;

void printArrayDeStrings(char** arrayDeStrings);
int tamanioDeArrayDeStrings(char** arrayDeString);
char** parser(char* input);
int pesoString(char*);
char *concat(int count, ...);
int arrayIncluye(char **array, char *elemento);
char *convertirArrayAString(char **array);
void mkdir_recursive(char *path);
void valorSinComillas(char *valor);
t_comando instanciarComando(char **request);
int obtenerCantidadParametros(char **request);
bool cantidadDeParametrosEsValida(char* request, int cantidadDeParametros);
bool stringEsVacio(const char* string);
char *armarLinea(char *key, char *valor, time_t timestamp);
char **desarmarLinea(char *linea);
int archivoVacio(char *path);
void freeArrayDeStrings(char **array);
void vaciarString(char** string);

#endif //TP_2019_1C_SUCK_ET_ARRAYDESTRING_H