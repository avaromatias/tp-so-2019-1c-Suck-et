/*
 ============================================================================
 Name        : LFS.h
 Author      : Suck et
 	 UTN FRBA - Sistemas Operativos
 	 	 TP-1C-2019-Lissandra
 ============================================================================
 */


#ifndef LFS_H_
#define LFS_H_


#include <stdio.h>
#include <string.h>
#include <libgen.h>
#include <stdlib.h>
#include <time.h>
#include <libgen.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <commons/config.h>
#include <commons/log.h>
#include <commons/collections/dictionary.h>
#include <semaphore.h>
#include <commons/collections/list.h>
#include <commons/bitarray.h>

#include "../libs/config.h"
#include "../libs/sockets.h"
#include "../libs/consola.h"


//Variables y estructuras
typedef struct {
    int puertoEscucha;
    char *puntoMontaje;
    int retardo;
    int tamanioValue;
    int tiempoDump;
} t_configuracion;

typedef struct {
    int consistency;
    int partitions;
    char *compaction_time;
} t_metadata;

typedef struct {
    GestorConexiones *conexion;
    t_log *logger;
    sem_t *memoriaConectada;
    int *fdMemoria;
} parametros_thread_lfs;

typedef struct {
    char *comando;
} parametros_thread_request;

t_configuracion configuracion;
t_log *logger;
t_dictionary *metadatas;


//Header de funciones
t_configuracion cargarConfiguracion(char *path, t_log *logger);

void atenderMensajes(Header header, char *mensaje, parametros_thread_lfs *parametros);

void lfsInsert(char *nombreTabla, char *key, char *valor, time_t timestamp);

pthread_t *crearHiloRequest(char *mensaje);

char *procesarComando(char *comando);

void mkdir_recursive(char *path);

int validarConsistencia(char *tipoConsistencia);

void crearBinarios(char *nombreTabla, int particiones);

void crearMetadata(char *nombreTabla, char *tipoConsistencia, char *particiones, char *tiempoCompactacion);

void lfsCreate(char *nombreTabla, char *tipoConsistencia, char *particiones, char *tiempoCompactacion);

void *procesarComandoPorRequest(void *params);

int obtenerTamanioBloque(int bloque);
int archivoVacio(char * path);

void lfsSelect(char *nombreTabla, char *key);

void mkdir_recursive(char *path);

/**
* @NAME: gestionarRequest
* @DESC: Gestiona los comandos recibidos por consola y decide como proceder
*
* @RETURN:
* -2: Comando invalido
* -1: Numero de parametros invalido
* 0: Ejecucion exitosa
*/
int gestionarRequest(char **request);

/**
* @NAME: existeTabla
* @DESC: Valida si existe el directorio de la tabla especificada por parametro
*
* @RETURN:
* -1: No se encontro o no se pudo acceder a la tabla especificada
* 0: Existe el directorio especificado
*/
int existeTabla(char *path);

/**
* @NAME: obtenerMetadata
* @DESC: Almacena o actualiza la metadata de una tabla en el diccionario de metadatas
*/
void obtenerMetadata(char *tabla);

/**
* @NAME: calcularParticion
* @DESC: Retorna el numero de particion asignado a una key especifica
*/
int calcularParticion(char *key, t_metadata *metadata);

void *atenderConexiones(void *parametrosThread);


#endif /* LFS_H_ */
