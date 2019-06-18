/*
 ============================================================================
 Name        : Kernel.h
 Author      : Suck et
 	 UTN FRBA - Sistemas Operativos
 	 	 TP-1C-2019-Lissandra
 ============================================================================
 */


#ifndef KERNEL_H_
#define KERNEL_H_

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <commons/config.h>
#include <commons/log.h>
#include <commons/collections/list.h>
#include <commons/collections/queue.h>
#include "../libs/config.h"
#include "../libs/sockets.h"
#include "../libs/consola.h"
#include "conexiones.h"

typedef struct {
    char *ipMemoria;
    int puertoMemoria;
    int quantum;
    int multiprocesamiento;
    int refreshMetadata;
    int retardoEjecucion;
} t_configuracion;

typedef struct {
    char **listaDeRequests;
    int cantidadDeLineas;
} t_archivoLQL;

typedef struct {
    int indice;
    int fileDescriptor;
    char* consistencia;
} memoria_conocida;

typedef struct {
    t_log *logger;
    GestorConexiones *conexiones;
    t_dictionary *metadataTablas;
    t_dictionary *memoriasConCriterios;
} parametros_consola_kernel;

typedef struct {
    t_queue *colaDeNew;
    t_queue *colaDeReady;
    t_queue *colaDeExecute;
    t_list *finalizados;
} colasPlanificacion;

//Para Planificador

// ***** COMPORTAMIENTOS DEL KERNEL *****

t_configuracion cargarConfiguracion(char *, t_log *);

int gestionarRequest(t_comando requestParseada, parametros_consola_kernel *parametros);

void ejecutarConsola(int (*gestionarRequest)(t_comando, parametros_consola_kernel*), parametros_consola_kernel *parametros);

void *analizarRequest(t_comando requestParseada, parametros_consola_kernel *parametros);

void *administrarRequestsLQL(t_archivoLQL archivoLQL, parametros_consola_kernel *parametros);

int conectarseAMemoriaPrincipal(char* ipMemoria, int puertoMemoria, GestorConexiones* misConexiones, t_log* logger);

// ***** GESTIÓN DE COMANDOS *****

bool validarComandosKernel(t_comando requestParseada, t_log *logger);

bool esComandoValidoDeKernel(t_comando comando);

int gestionarSelectKernel(char *nombreTabla, char *key, int fdMemoria);

int gestionarCreateKernel(char *nombreTabla, char *tipoConsistencia, char *cantidadParticiones, char *tiempoCompactacion, int fdMemoria);

int gestionarInsertKernel(char *nombreTabla, char *key, char *valor, int fdMemoria);

int gestionarDropKernel(char *nombreTabla, int fdMemoria);

int gestionarAdd(char** parametrosDeRequest, parametros_consola_kernel *parametros);

int gestionarRun(char *pathArchivo, parametros_consola_kernel *parametros);


#endif /* KERNEL_H_ */
