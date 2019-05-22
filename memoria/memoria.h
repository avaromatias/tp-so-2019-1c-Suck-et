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

typedef struct {
    int timestamp;
    u_int16_t key;
    char* value;
} t_pagina;

typedef struct{
    t_pagina unaPagina;
    int numeroDePagina;
    int flagDeModificado;
} t_registroPagina;

typedef struct {
    char* nombreTabla;
    int base;
    int tamanioSegmento; //indica el limite del segmento
} t_segmento;

//Tipo de la Memoria Principal que aloja las paginas
typedef struct {
    int tamanioMemoria;
    char* direcciones;
    int tamanioDePagina;
} t_memoria;

t_configuracion cargarConfiguracion(char* path, t_log* logger);

t_configuracion configuracion; //Declaro mi instancia de t_configuracion como global

int gestionarRequest(char **request);

t_memoria* inicializarMemoriaPrincipal(t_configuracion configuracion, t_log* logger);

int calcularTamanioDePagina(int tamanioValue);
//Esta funcion envia la petición del TAM_VALUE a lissandra y devuelve la respuesta del HS
int getTamanioValue(int fdLissandra, t_log* logger);

// no usar variables globales
//Instancia global de la memoria

#endif /* MEMORIA_H_ */