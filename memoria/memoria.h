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
#include <inttypes.h>

typedef struct t_memoria_d t_memoria;

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
    char* base;
    bool ocupado;
} t_marco;

typedef struct  {
    char* key;
    t_marco* marco;
    bool modificada;
    long ultimaVezUsada;
} t_pagina;

typedef struct {
    char* pathTabla; // viene a ser el identificador del segmento
    t_dictionary* tablaDePaginas; // viene a ser la base del segmento
} t_segmento;

//Tipo de la Memoria Principal que aloja las paginas
struct t_memoria_d {
    char* direcciones;
    int tamanioMemoria;
    int tamanioPagina;
    int cantidadMaximaCaracteresValue;
    int cantidadTotalMarcos;
    int marcosOcupados;
    t_dictionary* tablaDeSegmentos;
    t_marco* tablaDeMarcos;
};

typedef struct {
    t_memoria* memoria;
    struct t_control_conexion* conexionLissandra;
    t_log* logger;
} parametros_consola_memoria;
pthread_t* crearHiloConsola(t_memoria* memoria, t_log* logger, t_control_conexion* conexionLissandra);

t_configuracion cargarConfiguracion(char* path, t_log* logger);

char* gestionarRequest(t_comando comando, t_memoria* memoria, t_control_conexion* conexionLissandra, t_log* logger);

t_memoria* inicializarMemoriaPrincipal(t_configuracion configuracion, int tamanioPagina, t_log* logger);

int calcularTamanioDePagina(int tamanioValue);
//Esta funcion envia la petición del TAM_VALUE a lissandra y devuelve la respuesta del HS
int getTamanioValue(t_control_conexion* conexionLissandra, t_log* logger);
int cantidadTotalMarcosMemoria(t_memoria memoria);
void inicializarTablaDeMarcos(t_memoria* memoriaPrincipal);
void insertarEnMemoriaAndActualizarTablaDePaginas(t_pagina* nuevaPagina, char* value, int tamanioPagina, t_dictionary* tablaDePaginas);
t_pagina* crearPagina(char* key, t_memoria* memoria);
char* formatearPagina(char* key, char* value, char* timestamp);
bool hayMarcosLibres(t_memoria memoria);
t_marco* getMarcoLibre(t_memoria* memoria);
t_pagina* insert(char* nombreTabla, char* key, char* value, t_memoria* memoria, char* timestamp, t_log* logger,  t_control_conexion* conexionLissandra);
t_pagina* insertarNuevaPagina(char* key, char* value, t_dictionary* tablaDePaginas, t_memoria* memoria, bool recibiTimestamp);
t_segmento* crearSegmento(char* nombreTabla, t_memoria* memoria);
t_pagina* reemplazarPagina(char* key, char* nuevoValor, int tamanioPagina, t_dictionary* tablaDePaginas);
t_pagina* cmdSelect(char* nombreTabla, char* key, t_memoria* memoria);

//void logearValorDeSemaforo(sem_t* unSemaforo, t_log* logger, char* unString);

// drop
char* gestionarDrop(char* nombreTabla, int fdLissandra, t_memoria* memoria, t_log* logger);
char* drop(char* nombreTabla, t_memoria* memoria);
void liberarPaginasSegmento(t_dictionary* tablaDePaginas, t_memoria* memoria);
void eliminarPagina(void* pagina);
void eliminarSegmento(void* segmento);

//JOURNAL
typedef  struct {
    char* nombreTabla;
    t_control_conexion* conexionLissandra;
    t_log* logger;

}parametros_journal;
void mi_dictionary_iterator(parametros_journal* parametrosJournal, t_dictionary *self, void(*closure)(parametros_journal*,char*,void*));
void enviarInsertLissandra(parametros_journal* parametrosJournal, char* key, char* value, char* timestamp);
void vaciarMemoria(t_memoria* memoria, t_log* logger);
pthread_t* crearHiloJournal(t_memoria* memoria, t_log* logger, t_control_conexion* conexixonLissandra, int retardoJournal);

//gossiping
typedef struct {
    char* ipSeed;
    char* puertoSeed;
}t_nodo_memoria;

#endif /* MEMORIA_H_ */