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

//Estructura necesaria para el archivo de configuración
typedef struct {
    char *ipMemoria;
    int puertoMemoria;
    int quantum;
    int multiprocesamiento;
    int refreshMetadata;
    int retardoEjecucion;
} t_configuracion;

//Estructura necesaria para las metadatas de las tablas
typedef struct {
    char *consistencia;
    int particiones;
    int tiempoCompactacion;
} t_metadataTablas;

//Estructura necesaria para el manejo de archivosLQL
typedef struct {
    t_queue *colaDeRequests;//cada Request va a ser un t_comando
    int cantidadDeLineas;
    int PID;
    int cantidadDeSelectProcesados;
    int cantidadDeInsertProcesados;
} t_archivoLQL;

//Estructura necesaria para manejar las consistencias y la metadata
typedef struct {
    t_log *logger;
    GestorConexiones *conexiones;
    t_dictionary *metadataTablas;
    t_dictionary *memoriasConCriterios;
    pthread_mutex_t *mutexJournal;
    t_dictionary *supervisorDeHilos;
} p_consola_kernel;

//Estructura necesaria para el PLP
typedef struct {
    t_queue *colaDeNew;
    t_queue *colaDeReady;
    sem_t *mutexColaDeNew;
    sem_t *mutexColaDeReady;
    sem_t *cantidadProcesosEnNew;
    sem_t *cantidadProcesosEnReady;
    int *contadorPID;
    t_log *logger;
} parametros_plp;

//Estructura necesaria para el PCP
typedef struct {
    int *quantum;
    t_queue *colaDeReady;
    t_queue *colaDeFinalizados;
    sem_t *mutexColaDeReady;
    sem_t *mutexColaFinalizados;
    t_log *logger;
    sem_t *cantidadProcesosEnReady;
    pthread_mutex_t *mutexJournal;
    pthread_mutex_t *mutexSemaforoHilo;
} parametros_pcp;

//Estructura hibrida necesaria para planificacion
typedef struct {
    p_consola_kernel *parametrosConsola;
    parametros_plp *parametrosPLP;
    parametros_pcp *parametrosPCP;
    t_dictionary *supervisorDeHilos;
} p_planificacion;

/******************************
 ** COMPORTAMIENTO DE KERNEL **
 ******************************/

void inicializarSemyMutex();

t_configuracion cargarConfiguracion(char *, t_log *);

void inicializarEstructurasKernel(t_dictionary *tablaDeMemoriasConCriterios);

int gestionarRequestPrimitivas(t_comando requestParseada, p_planificacion *paramPlanifGeneral);

int gestionarRequestKernel(t_comando requestParseada, p_planificacion *paramPlanifGeneral);

void ejecutarConsola(p_consola_kernel *parametros, t_configuracion configuracion, p_planificacion *paramPlanifGral);

bool analizarRequest(t_comando requestParseada, p_consola_kernel *parametros);

int conectarseAMemoriaPrincipal(char *ipMemoria, int puertoMemoria, GestorConexiones *misConexiones, t_log *logger);

/******************************
 ***** GESTIÓN DE COMANDOS ****
 ******************************/

bool validarComandosKernel(t_comando requestParseada, t_log *logger);

bool esComandoValidoDeKernel(t_comando comando);

int gestionarSelectKernel(char *nombreTabla, char *key, int fdMemoria, p_planificacion *paramPlanifGeneral);

int gestionarCreateKernel(char *tabla, char *consistencia, char *cantParticiones, char *tiempoCompactacion,
                          int fdMemoria, p_planificacion *paramPlanifGeneral);

int gestionarInsertKernel(char *nombreTabla, char *key, char *valor, int fdMemoria, p_planificacion *paramPlanifGral);


int gestionarDropKernel(char *nombreTabla, int fdMemoria, p_planificacion *paramPlanifGeneral);

int gestionarDescribeTablaKernel(char *nombreTabla, int fdMemoria, p_planificacion *paramPlanifGeneral);

int gestionarDescribeGlobalKernel(int fdMemoria, p_planificacion *paramPlanifGeneral);

int gestionarAdd(char **parametrosDeRequest, p_consola_kernel *parametros);

int gestionarRun(char *pathArchivo, p_consola_kernel *parametros, parametros_plp *parametrosPLP);

int gestionarJournalKernel(p_planificacion *paramPlanifGeneral);

int diferenciarRequest(t_comando requestParseada);

void actualizarCantRequest(t_archivoLQL *archivoLQL, t_comando requestParseada);

int extensionCorrecta(char *direccionAbsoluta);

void enviarJournal(int memoriaSeleccionada, p_planificacion *paramPlanifGeneral);

/******************************
 ***** MANEJO DE MEMORIAS *****
 ******************************/

bool existenMemoriasConectadas(GestorConexiones *misConexiones);

char *criterioBuscado(t_comando requestParseada, t_dictionary *metadataTablas);

int seleccionarMemoriaIndicada(p_consola_kernel *parametros, char *criterio, int key);

int seleccionarMemoriaParaDescribe(p_consola_kernel *parametros);

/****************************
 ****** PLANIFICACIÓN *******
 ****************************/
t_archivoLQL *crearLQL(parametros_plp *parametrosPLP);

bool encolarDirectoNuevoPedido(t_comando requestParseada);

t_archivoLQL *convertirRequestALQL(t_comando *requestParseada);

pthread_t *crearHiloPlanificadorLargoPlazo(parametros_plp *parametros);

pthread_t *crearHiloPlanificadorCortoPlazo(p_planificacion *parametros);

void *sincronizacionPLP(void *parametrosPLP);

void instanciarPCPs(p_planificacion *);

void planificarRequest(p_planificacion *paramPlanificacionGeneral, t_archivoLQL *archivoLQL);

//Estructuras de las metricas
typedef struct {
    int fdMemoria;
    //char* criteroAsociado;
    long inicioRequest;
    long finRequets;
    long duracionEnSegundos;
    char *tipoRequest;
} t_estadistica_request;


#endif /* KERNEL_H_ */
