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

t_dictionary tablaDeMemorias;

//Para Planificador
t_queue *colaDeNew;
t_queue *colaDeReady;
t_queue *colaDeExecute;
t_list *finalizados;

//Funciones propias de Kernel
t_configuracion cargarConfiguracion(char *, t_log *);

int gestionarRequest(t_comando requestParseada, int fdMemoria);

bool validarComandosKernel(t_comando requestParseada, t_log *logger);

bool esComandoValidoDeKernel(t_comando comando);

void ejecutarConsola(int (*gestionarRequest)(t_comando, int), t_log *logger, int fdMemoria);

void *analizarRequest(t_comando requestParseada, t_log *logger, int fdMemoria);

void *administrarRequestsLQL(t_archivoLQL archivoLQL, t_log *logger, int fdMemoria);

int gestionarRun(char *pathArchivo, int fdMemoria);

int gestionarSelectKernel(char *nombreTabla, char *key, int fdMemoria);

int gestionarCreateKernel(char *nombreTabla, char *tipoConsistencia, char *cantidadParticiones, char *tiempoCompactacion, int fdMemoria);

int gestionarInsertKernel(char *nombreTabla, char *key, char *valor, int fdMemoria);

int gestionarDropKernel(char *nombreTabla, int fdMemoria);

//void conectarseAMemoriaPrincipal(t_memoria_conocida *memoriaConocida, char* ipMemoria, int puertoMemoria, t_log* logger);

#endif /* KERNEL_H_ */
