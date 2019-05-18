/*
 ============================================================================
 Name        : Memoria.h
 Author      : Suck et
 	 UTN FRBA - Sistemas Operativos
 	 	 TP-1C-2019-Lissandra
 ============================================================================
 */

#ifndef MEMORIA_H_
#define MEMORIA_H_

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <semaphore.h>
#include <commons/config.h>
#include <commons/log.h>
#include <readline/readline.h>
#include "../libs/config.h"
#include "../libs/sockets.h"
#include "../libs/consola.h"
#include "conexiones.h"


typedef struct {
    int puerto;
    char* ipFileSystem;
    int puertoFileSystem;
    char** ipSeeds;
    int* puertoSeeds;
    int retardoMemoria;
    int retardoFileSystem;
    int tamanioMemoria;
    int retardoJournal;
    int retardoGossiping;
    int cantidadDeMemorias;
} t_configuracion;

t_configuracion cargarConfiguracion(char* path, t_log* logger);

t_configuracion configuracion; //Declaro mi instancia de t_configuracion como global

int gestionarRequest(char **request);

//Tamaño del value indicado por LFS en el handshake
int TAM_VALUE = 0;

typedef struct {
    int timestamp;
    u_int16_t key;
    char* value; //Cada hardcodeo que meto en el codigo es un arbolito que se muere
    int numeroDePagina;
    int flagDeModificado;
}t_pagina;

typedef struct{
    t_pagina* paginas;
}t_tablaDePaginas;


//Tipo de la Memoria Principal que aloja las paginas
typedef struct{
    int tamanioMemoria;
    char* direcciones;

}t_memoria;

char* config_get_string_in_array_by_index(char** array, int indiceBuscado);

// no usar variables globales
//Instancia global de la memoria
t_memoria memoria;
#endif /* MEMORIA_H_ */