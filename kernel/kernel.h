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
#include <semaphore.h>
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

//Estructura necesaria para el manejo de archivosLQL
typedef struct {
    t_queue *colaDeRequests;
    int cantidadDeLineas;
    int cantidadDeSelectProcesados;
    int cantidadDeInsertProcesados;
} t_archivoLQL;

//Estructura necesaria para manejar las consistencias y la metadata
typedef struct {
    t_log *logger;
    GestorConexiones *conexiones;
    t_dictionary *metadataTablas;
    t_dictionary *memoriasConCriterios;
} p_consola_kernel;

//Estructura necesaria para el PLP
typedef struct {
    t_queue *colaDeNew;
    t_queue *colaDeReady;
    sem_t *mutexColaDeNew;
    sem_t *mutexColaDeReady;
    t_log *logger;
    sem_t *cantidadProcesosEnNew;
    sem_t *cantidadProcesosEnReady;
} parametros_plp;

//Estructura necesaria para el PCP
typedef struct {
    int *quantum;
    t_queue *colaDeReady;
    t_list *finalizados;
    sem_t *mutexColaDeReady;
    t_log *logger;
    sem_t *cantidadProcesosEnReady;
} parametros_pcp;

//Estructura hibrida necesaria para planificacion
typedef struct {
    p_consola_kernel *parametrosConsola;
    parametros_plp *parametrosPLP;
    parametros_pcp *parametrosPCP;
} p_planificacion;

// ***** COMPORTAMIENTOS DEL KERNEL *****

t_configuracion cargarConfiguracion(char *, t_log *);

int gestionarRequestPrimitivas(t_comando requestParseada, p_consola_kernel *parametros);

int gestionarRequestKernel(t_comando requestParseada, p_consola_kernel *parametros, parametros_plp *parametrosPLP);

void ejecutarConsola(p_consola_kernel *parametros, t_configuracion configuracion, parametros_plp *parametrosPLP);

bool analizarRequest(t_comando requestParseada, p_consola_kernel *parametros);

void *administrarRequestsLQL(t_archivoLQL archivoLQL, p_consola_kernel *parametros);

int conectarseAMemoriaPrincipal(char *ipMemoria, int puertoMemoria, GestorConexiones *misConexiones, t_log *logger);

// ***** GESTIÓN DE COMANDOS *****

bool validarComandosKernel(t_comando requestParseada, t_log *logger);

bool esComandoValidoDeKernel(t_comando comando);

int gestionarSelectKernel(char *nombreTabla, char *key, int fdMemoria);

int gestionarCreateKernel(char *tabla, char *consistencia, char *cantParticiones, char *tpoCompactacion, int fdMemoria);

int gestionarInsertKernel(char *nombreTabla, char *key, char *valor, int fdMemoria);

int gestionarDropKernel(char *nombreTabla, int fdMemoria);

int gestionarAdd(char **parametrosDeRequest, p_consola_kernel *parametros);

int gestionarRun(char *pathArchivo, p_consola_kernel *parametros, parametros_plp *parametrosPLP);

int diferenciarRequest(t_comando requestParseada);

void actualizarCantRequest(t_archivoLQL *archivoLQL, t_comando requestParseada);

// ***** MANEJO DE MEMORIAS *****

bool existenMemoriasConectadas(GestorConexiones *misConexiones);

char *criterioBuscado(t_comando requestParseada, t_dictionary *metadataTablas);

int seleccionarMemoriaIndicada(p_consola_kernel *parametros, char *criterio);

char **obtenerDatosDeConexion(char *datosConexionMemoria); //para Gossiping

// ***** PLANIFICADORES *****

bool encolarDirectoNuevoPedido(t_comando requestParseada);

t_archivoLQL *convertirRequestALQL(t_comando requestParseada);

pthread_t *crearHiloPlanificadorLargoPlazo(parametros_plp *parametros);

pthread_t *crearHiloPlanificadorCortoPlazo(parametros_pcp *parametros);

void *sincronizacionPLP(void *parametrosPLP);

void instanciarPCPs(parametros_pcp *parametrosPCP, parametros_plp *parametrosPLP, p_consola_kernel *parametrosConsola);

void *planificarRequest(p_planificacion *paramPlanificacionGeneral, t_archivoLQL *archivoLQL);

#endif /* KERNEL_H_ */
