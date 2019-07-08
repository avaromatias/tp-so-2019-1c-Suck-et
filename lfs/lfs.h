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
#include <sys/mman.h>
#include <fcntl.h>
#include <stdarg.h>
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
    char *consistency;
    int partitions;
    int compaction_time;
} t_metadata;

typedef struct {
    TipoMensaje tipoRespuesta;
    char *valor
} t_response;

typedef struct {
    GestorConexiones *conexion;
    t_log *logger;
    int tamanioValue;
} parametros_thread_lfs;

typedef struct {
    Header header;
    char *mensaje;
} parametros_thread_request;

typedef struct {
    char *tabla;
    char *tiempoCompactacion;
} parametros_thread_compactacion;

typedef struct {
    char *tabla;
    int particion;
} t_bloqueAsignado;

t_configuracion configuracion;
t_log *logger;
t_dictionary *bloquesAsignados;
t_dictionary *metadatas;
t_dictionary *memTable;
t_dictionary *archivosAbiertos;
t_dictionary *tablasEnUso;
t_dictionary *hilosTablas;
t_bitarray *bitmap;
pthread_mutex_t* mutexAsignacionBloques;


//Header de funciones
t_configuracion cargarConfiguracion(char *path, t_log *logger);

void atenderMensajes(void *parametrosRequest);

char **obtenerTablas();

t_response *lfsDescribe(char *nombreTabla);

void *obtenerSemaforoPath(char *path);

char *obtenerBloquesSegunExtension(char *nombreTabla, char *ext);

char **filtrarKeyMax(char **listaLineas);

//char* stringDeArraySinCorchetes(char* array);

t_response *lfsCreate(char *nombreTabla, char *tipoConsistencia, char *particiones, char *tiempoCompactacion);

t_response *lfsSelect(char *nombreTabla, char *key);

t_response *lfsInsert(char *nombreTabla, char *key, char *valor, time_t timestamp);

int obtenerBloqueDisponible(char *nombreTabla, int particion);

void cargarBloquesAsignados(char *path);

int validarConsistencia(char *tipoConsistencia);

void crearBinarios(char *nombreTabla, int particiones);

void crearMetadata(char *nombreTabla, char *tipoConsistencia, char *particiones, char *tiempoCompactacion);

char *generarContenidoParaParticion(char *tamanio, char *bloques);

pthread_t *crearHiloCompactacion(char *nombreTabla, char *tiempoCompactacion);

char *obtenerNombreArchivoParticion(int particion);

int obtenerTamanioBloque(int bloque);

int obtenerTamanioBloques(char *puntoMontaje);

int obtenerCantidadBloques(char *puntoMontaje);

char *obtenerLineaMasReciente(char **bloques, char *key);

char *obtenerStringBloquesDeArchivo(char *nombreTabla, char *nombreArchivo);

char **convertirStringDeBloquesAArray(char *bloques);

void eliminarCharDeString(char *string, char ch);

char *obtenerBloquesAsignados(char *nombreTabla, int particion);

//char *stringDeArraySinCorchetes(char *array);
//char **bloquesEnParticion(char *nombreTabla, char *nombreArchivo);
char *obtenerStringBloquesSegunExtension(char *nombreTabla, char *ext);

char **obtenerLineasDeBloques(char **bloques);

/**
* @NAME: gestionarRequest
* @DESC: Gestiona los comandos recibidos por consola y decide como proceder
*
*/
t_response *gestionarRequest(t_comando comando);

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
