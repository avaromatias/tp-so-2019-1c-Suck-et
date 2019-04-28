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
#include <commons/config.h>
#include <commons/log.h>
#include <readline/readline.h>
#include "../libs/config.h"
#include "../libs/sockets.h"
#include "../libs/generales.h"


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

void atenderMensajes(Header, char*);

typedef struct {
    int timestamps;
    u_int16_t key;
    char* value;
}t_pagina;

typedef struct {
    t_pagina* pagina;
    int numeroDePagina;
    int flagDeModificado;
}t_registro;

typedef struct{
    t_registro** registros;
}t_tablaDePaginas;


#endif /* MEMORIA_H_ */
