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
#include <stdint.h>
#include <commons/config.h>
#include <commons/log.h>
#include <commons/collections/dictionary.h>
#include <commons/collections/queue.h>
#include <readline/readline.h>
#include <time.h>
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

typedef struct  {
    char* key;
    char* base;
    bool modificada;
} t_pagina;

typedef struct {
    char* pathTabla; // viene a ser el identificador del segmento
    t_dictionary* tablaDePaginas; // viene a ser la base del segmento
} t_segmento;

typedef struct {
    char* base;
    bool ocupado;
} t_marco;

//Tipo de la Memoria Principal que aloja las paginas
typedef struct {
    char* direcciones;
    int tamanioMemoria;
    int tamanioPagina;
    int cantidadTotalMarcos;
    int marcosOcupados;
    t_dictionary* tablaDeSegmentos;
    t_marco* tablaDeMarcos;
} t_memoria;

typedef struct {
    t_memoria* memoria;
    int fdLissandra;
    t_log* logger;
} parametros_consola_memoria;

t_configuracion cargarConfiguracion(char* path, t_log* logger);

int gestionarRequest(char **request, t_memoria* memoria);

t_memoria* inicializarMemoriaPrincipal(t_configuracion configuracion, int tamanioPagina, t_log* logger);

int calcularTamanioDePagina(int tamanioValue);
//Esta funcion envia la petición del TAM_VALUE a lissandra y devuelve la respuesta del HS
int getTamanioValue(int fdLissandra, t_log* logger);
int cantidadTotalMarcosMemoria(t_memoria memoria);
void inicializarTablaDeMarcos(t_memoria* memoriaPrincipal);
void insertarEnMemoriaAndActualizarTablaDePaginas(t_pagina* nuevaPagina, char* value, t_dictionary* tablaDePaginas);
t_pagina* crearPagina(char* key, t_memoria* memoria);
char* formatearPagina(char* key, char* value);
bool hayMarcosLibres(t_memoria memoria);
char* getMarcoLibre(t_memoria* memoria);
char* insert(char* nombreTabla, char* key, char* value, t_memoria* memoria);
char* insertarNuevaPagina(char* key, char* value, t_dictionary* tablaDePaginas, t_memoria* memoria);
t_segmento* crearSegmento(char* nombreTabla, t_memoria* memoria);
char* reemplazarPagina(char* key, char* nuevoValor, t_dictionary* tablaDePaginas);
t_pagina* cmdSelect(char* nombreTabla, char* key, t_memoria* memoria);
#endif /* MEMORIA_H_ */