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
#define EVENT_SIZE  ( sizeof (struct inotify_event) + 48)
#define BUF_LEN     ( EVENT_SIZE )

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
typedef struct t_nodoMemoria nodoMemoria;
typedef struct t_configuracion_d t_configuracion;
typedef struct t_retardos_memoria_d t_retardos_memoria;
typedef struct t_sincro_journaling_d t_sincro_journaling;
typedef struct t_parametros_conexion_lissandra_d t_parametros_conexion_lissandra;

struct t_parametros_conexion_lissandra_d {
    char* ip;
    int puerto;
};

#include "conexiones.h"

struct t_retardos_memoria_d {
    int retardoMemoria;
    int retardoGossiping;
    int retardoJournaling;
    int retardoFileSystem;
};

struct t_configuracion_d    {
    int puerto;
    char* ipFileSystem;
    int puertoFileSystem;
    char** ipSeeds;
    char** puertoSeeds;
    int retardoMemoria;
    int retardoFileSystem;
    int tamanioMemoria;
    int retardoJournal;
    int retardoGossiping;
    int memoryNumber;
    char* ipMemoria;
};

struct t_sincro_journaling_d {
    sem_t semaforoJournaling;
    int cantidadRequestsEnParalelo;
    pthread_mutex_t mutexNivel;
    pthread_mutex_t ejecutandoJournaling;
};

typedef struct {
    char* base;
    bool ocupado;
} t_marco;

typedef struct  {
    char* key;
    t_marco* marco;
    bool modificada;
    unsigned long long ultimaVezUsada;
} t_pagina;

typedef struct {
    char* nombreTabla;
    t_pagina* paginaLRU;
}t_reemplazo_pagina;

typedef struct {
    char* pathTabla; // viene a ser el identificador del segmento
    t_dictionary* tablaDePaginas; // viene a ser la base del segmento
    pthread_mutex_t enUso;
} t_segmento;

typedef struct {
    bool enUso;
    pthread_mutex_t semaforo;
    pthread_mutex_t tablaDeSegmentosEnUso;
    pthread_mutex_t tablaDeMarcosEnUso;
    pthread_mutex_t mutexMarcosOcupados;
} t_control_memoria;

//Tipo de la Memoria Principal que aloja las paginas
struct t_memoria_d {
    char* direcciones;
    int tamanioMemoria;
    int tamanioPagina;
    int cantidadMaximaCaracteresValue;
    int cantidadTotalMarcos;
    int marcosOcupados;
    int tamanioValue;
    int memoryNumber;
    t_dictionary* tablaDeSegmentos;
    t_marco* tablaDeMarcos;
    t_list* memoriasConocidas;
    t_list* nodosMemoria;
    t_control_memoria control;
};

typedef struct {
    char* directorioAMonitorear;
    t_retardos_memoria* retardos;
    t_log* logger;
    char* nombreArchivoDeConfiguracion;
    pthread_mutex_t* semaforoRetardos;
}parametros_hilo_monitor;

typedef struct {
    t_memoria* memoria;
    t_log* logger;
    t_retardos_memoria* retardos;
    t_sincro_journaling* semaforoJournaling;
    t_parametros_conexion_lissandra conexionLissandra;
    pthread_mutex_t* semaforoRetardos;
} parametros_hilo_journal;

typedef struct {
    t_memoria* memoria;
    t_parametros_conexion_lissandra conexionLissandra;
    t_log* logger;
    t_sincro_journaling* semaforoJournaling;
} parametros_consola_memoria;

t_configuracion cargarConfiguracion(char* path, t_log* logger);

t_paquete gestionarRequest(t_comando comando, t_memoria* memoria, int conexionLissandra, t_log* logger, t_sincro_journaling* semaforoJournalingf);

t_memoria* inicializarMemoriaPrincipal(t_configuracion configuracion, int tamanioPagina, t_log* logger);

int calcularTamanioDePagina(int tamanioValue);
//Esta funcion envia la petición del TAM_VALUE a lissandra y devuelve la respuesta del HS
int getTamanioValue(t_parametros_conexion_lissandra conexionLissandra, t_log* logger);
int cantidadTotalMarcosMemoria(t_memoria memoria);
void inicializarTablaDeMarcos(t_memoria* memoriaPrincipal);
void insertarEnMemoriaAndActualizarTablaDePaginas(t_pagina* nuevaPagina, char* value, int tamanioPagina, t_dictionary* tablaDePaginas);
t_pagina* crearPagina(char* key, t_memoria* memoria);
char* formatearPagina(char* key, char* value, char* timestamp, t_memoria* memoria);
bool hayMarcosLibres(t_memoria* memoria);
t_marco* getMarcoLibre(t_memoria* memoria);
t_pagina* insert(char* nombreTabla, char* key, char* value, t_memoria* memoria, char* timestamp, t_log* logger, int conexionLissandra, t_sincro_journaling* semaforoJournaling);
t_pagina* insertarNuevaPagina(char* key, char* value, t_dictionary* tablaDePaginas, t_memoria* memoria, bool recibiTimestamp);
t_segmento* crearSegmento(char* nombreTabla, t_memoria* memoria);
t_pagina* reemplazarPagina(char* key, char* nuevoValor, int tamanioPagina, t_dictionary* tablaDePaginas);
t_pagina* cmdSelect(char* nombreTabla, char* key, t_memoria* memoria);

//void logearValorDeSemaforo(sem_t* unSemaforo, t_log* logger, char* unString);

t_paquete gestionarSelect(char *nombreTabla, char *key, int conexionLissandra, t_memoria *memoria, t_log *logger,
                          t_sincro_journaling *semaforoJournaling);
t_paquete gestionarInsert(char *nombreTabla, char *key, char *valueConComillas, t_memoria *memoria, t_log *logger,
                          int conexionLissandra, t_sincro_journaling *semaforoJournaling);
t_paquete gestionarCreate(char *nombreTabla, char *tipoConsistencia, char *cantidadParticiones,
                          char *tiempoCompactacion, int conexionLissandra, t_log *logger);

// drop
t_paquete gestionarDrop(char *nombreTabla, int conexionLissandra, t_memoria *memoria, t_log *logger, t_sincro_journaling* semaforoJournaling);
char* drop(char* nombreTabla, t_memoria* memoria, t_sincro_journaling* semaforoJournaling);
void liberarPaginasSegmento(t_dictionary* tablaDePaginas, t_memoria* memoria);
void eliminarPagina(void* pagina);
void eliminarSegmento(void* segmento);

//JOURNAL
typedef  struct {
    char* nombreTabla;
    int conexionLissandra;
    t_log* logger;

}parametros_journal;

void mi_dictionary_iterator(parametros_journal* parametrosJournal, t_dictionary *self, void(*closure)(parametros_journal*,char*,void*));
void enviarInsertLissandra(parametros_journal* parametrosJournal, char* key, char* value, unsigned long long timestamp);
void vaciarMemoria(t_memoria* memoria, t_log* logger, t_sincro_journaling* semaforoJournaling);
pthread_t* crearHiloJournal(t_memoria* memoria, t_log* logger, t_parametros_conexion_lissandra conexionLissandra, t_retardos_memoria* retardos, t_sincro_journaling* semaforoJournaling, pthread_mutex_t* semaforoRetardos);

//monitoreo

void* monitorearDirectorio(void* parametros);
pthread_t* crearHiloMonitor(char* directorioAMonitorear, char* nombreArchivoConfiguracionConExtension, t_log* logger, t_retardos_memoria* retardos, pthread_mutex_t* semaforoRetardos);

//monitoreo

//gossiping

typedef struct {
    t_log* logger;
    t_memoria* memoria;
    GestorConexiones* misConexiones;
    t_configuracion archivoDeConfiguracion;
    pthread_mutex_t* semaforoMemoriasConocidas;
    t_sincro_journaling* semaforoJournaling;
    t_retardos_memoria* retardosMemoria;
    pthread_mutex_t* semaforoRetardos;
} parametros_gossiping;

struct t_nodoMemoria{
    char* ipNodoMemoria;
    int puertoNodoMemoria;
    int fdNodoMemoria;
};

void agregarIpMemoria(char* ipMemoriaSeed, char* puertoMemoriaSeed, t_list* memoriasConocidas, t_log* logger);
pthread_t * crearHiloGossiping(GestorConexiones* misConexiones , t_memoria* memoria, t_log* logger, t_configuracion configuracion, pthread_mutex_t* semaforoMemoriasConocidas, t_sincro_journaling* semaforoJournaling, t_retardos_memoria* retardos, pthread_mutex_t* semaforoRetardos);

void eliminarNodoMemoria(int fdNodoMemoria, t_list* nodosMemoria);
void eliminarMemoriaConocida(t_memoria* memoria, nodoMemoria* unNodoMemoria);
bool esNodoMemoria(int fdConectado, t_list* nodosMemoria);
void conectarYAgregarNuevaMemoria(char* ipNuevaMemoria, GestorConexiones* misConexiones, t_log* logger, t_memoria* memoria);
#endif /* MEMORIA_H_ */

char* getValueFromContenidoPagina(char* contenidoPagina);
char* getKeyFromContenidoPagina(char* contenidoPagina);
unsigned long long getTimestampFromContenidoPagina(char* contenidoPagina);

t_retardos_memoria* iniciarRetardos(t_configuracion configuracion);

pthread_t* crearHiloConsola(t_memoria* memoria, t_log* logger, t_parametros_conexion_lissandra conexionLissandra, t_sincro_journaling* semaforoJournaling);

