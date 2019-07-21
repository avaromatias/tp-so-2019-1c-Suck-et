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

#include <time.h>
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
#include "conexiones.h"

#define EVENT_SIZE  ( sizeof (struct inotify_event) + 24 )
#define BUF_LEN     ( 512 * EVENT_SIZE )

//Estructura necesaria para el archivo de configuración
typedef struct {
    char *ipMemoria;
    int puertoMemoria;
    int quantum;
    int multiprocesamiento;
    int refreshMetadata;
    int retardoEjecucion;
    char* directorioAMonitorear;
} t_configuracion;

//Estructura para manejar estadisticas de Requests de un FD
typedef struct {
    int fdMemoria;
    double inicioRequest;
    double finRequest;
    double duracionEnSegundos;
    char *tipoRequest;
} estadisticasRequest;

//Estructura necesaria para guardar las metricas de un solo FD
typedef struct {
    int fd;
    int cantidadSelects;
    int cantidadInserts;
    double tiempoTotalSelects;
    double tiempoTotalInserts;
} metricasParaUnFd;

//Estructura necesaria para el manejo de archivosLQL
/*typedef struct {
    t_queue *colaDeRequests;//cada Request va a ser un t_comando
    int cantidadDeLineas;
    int PID;
    int cantidadDeSelectProcesados;
    int cantidadDeInsertProcesados;
} t_archivoLQL;*/

/******************************
 ** COMPORTAMIENTO DE KERNEL **
 ******************************/

void inicializarSemyMutex();

t_configuracion cargarConfiguracion(char *, t_log *);

void inicializarEstructurasKernel(t_dictionary *tablaDeMemoriasConCriterios);

int gestionarRequestPrimitivas(t_comando requestParseada, p_planificacion *paramPlanifGeneral,
                               pthread_mutex_t *mutexDeHiloRequest, estadisticasRequest *estadisticasRequest,
                               sem_t *semConcurrenciaMetricas);

int gestionarRequestKernel(t_comando requestParseada, p_planificacion *paramPlanifGeneral);

void ejecutarConsola(p_consola_kernel *parametros, t_configuracion configuracion, p_planificacion *paramPlanifGral);

bool analizarRequest(t_comando requestParseada, p_consola_kernel *parametros);

int conectarseAMemoriaPrincipal(char *ipMemoria, int puertoMemoria, GestorConexiones *misConexiones, t_log *logger);

/******************************
 ***** GESTIÓN DE COMANDOS ****
 ******************************/

bool validarComandosKernel(t_comando requestParseada, t_log *logger);

bool esComandoValidoDeKernel(t_comando comando);

void imprimirMensajeAdd(int numeroMemoria, char *criterio);

int gestionarSelectKernel(char *nombreTabla, char *key, int fdMemoria, int PID, estadisticasRequest *estadisticasRequest);

int gestionarCreateKernel(char *tabla, char *consistencia, char *cantParticiones, char *tiempoCompactacion,
                          int fdMemoria, int PID);

int gestionarInsertKernel(char *nombreTabla, char *key, char *valor, int fdMemoria, int PID,
                          estadisticasRequest *estadisticasRequest);


int gestionarDropKernel(char *nombreTabla, int fdMemoria, int PID);

int gestionarDescribeTablaKernel(char *nombreTabla, int fdMemoria, int PID);

int gestionarDescribeGlobalKernel(int fdMemoria, int PID);

int gestionarAdd(char **parametrosDeRequest, p_planificacion *paramPlanificacionGeneral);

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

void
planificarRequest(p_planificacion *paramPlanificacionGeneral, t_archivoLQL *archivoLQL, pthread_mutex_t *semaforoHilo);


//Gossiping


typedef struct {
    t_log *logger;
    t_list *memoriasConocidas;
    GestorConexiones *misConexiones;
} parametros_gossiping;

pthread_t *crearHiloGossiping(GestorConexiones *misConexiones, t_list *memoriasConocidas, t_log *logger);

void conectarseANuevasMemorias(t_list *memoriasConocidas, GestorConexiones *misConexiones, t_log *logger);

void gossiping(parametros_gossiping *parametros);

int conectarseANuevoNodoMemoria(char *ipMemoria, int puertoMemoria, GestorConexiones *misConexiones, t_log *logger);

//Estructura que guarda los tiempos de refresh

typedef  struct{
    int nivelDeMultiProcesamiento;
    int refreshMetadata;
    int Quantum;
    int sleep;
}t_datos_configuracion;

typedef struct{

    char* directorioAMonitorear;

    t_log* logger;
    char* nombreArchivoDeConfiguracion;
    pthread_mutex_t* mutexDatosConfiguracion;
    t_datos_configuracion* datosConfiguracion;

}parametros_hilo_monitor;


pthread_t* crearHiloMonitor(char* directorioAMonitorear, char* nombreArchivoConfiguracionConExtension, t_log* logger, t_datos_configuracion* datosConfiguracion, pthread_mutex_t* mutexDatosConfiguracion);


#endif /* KERNEL_H_ */
