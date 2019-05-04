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

pthread_t* crearHiloServidor(int puerto, GestorConexiones* conexion, t_log* logger);

void* atenderConexionesNuevo(void* parametros);

typedef struct {
    int timestamp;
    u_int16_t key;
    char value[10]; //Cada hardcodeo que meto en el codigo es un arbolito que se muere
}t_pagina;

typedef struct {
    t_pagina pagina;
    int numeroDePagina;
    int flagDeModificado;
}t_registro;

typedef struct{
    t_registro* registros;
}t_tablaDePaginas;

typedef struct{
    int tamanioMemoria;
    char* direcciones;

}t_memoria;


// no usar variables globales
//Instancia global de la memoria
t_memoria memoria;
//Instancia global del File Descriptor del File System
int FD_FS;
//Instancia global del Tamaño maximo del value (viene del FS)
int TAM_VALUE;
//Instancia global del File Descriptor de mi instancia de memoria
int FD_CLIENTE;
#endif /* MEMORIA_H_ */

void* atenderConexionesNuevo(void* parametrosThread);

void atenderMensajesNuevos(Header, void*);