//
// Created by utnso on 12/05/19.
//

#ifndef KERNEL_CONEXIONES_H
#define KERNEL_CONEXIONES_H

#include <semaphore.h>
#include <commons/collections/queue.h>
#include <commons/collections/dictionary.h>
#include "../libs/sockets.h"
#include "../libs/consola.h"

//Estructura para manejar estadisticas de Requests de un FD
typedef struct {
    int fdMemoria;
    time_t inicioRequest;
    time_t finRequest;
    double duracionEnSegundos;
    TipoRequest tipoRequest;
} estadisticasRequest;

//Estructura necesaria para guardar las metricas de un solo FD
typedef struct {
    char *criterio;
    int cantidadSelects;
    int cantidadInserts;
    double tiempoTotalSelects;
    double tiempoTotalInserts;
} metricasParaCriterios;

//Estructura necesaria para guardar las metricas de un solo FD
typedef struct {
    char *criterio;
    double readLatency;
    double writeLatency;
    int reads;
    int writes;
    t_list *fds;
} t_metricasDefinidas;

typedef struct {
    t_list *estadisticas;
    metricasParaCriterios *metricas;
   } t_metricas;

//Estructura que guarda los tiempos de refresh

typedef  struct{
    int nivelDeMultiProcesamiento;
    int refreshMetadata;
    int Quantum;
    int retardoEjecucion;
}t_datos_configuracion;


typedef struct {
    GestorConexiones *conexion;
    t_dictionary *tablaDeMemoriasConCriterios;
    t_dictionary *metadataTabla;
    pthread_mutex_t *mutexJournal;
    t_dictionary *supervisorDeHilos;
    pthread_mutex_t *mutexSemaforoHilo;
    t_list* memoriasConocidas;
    t_log *logger;
    sem_t *semaforo_colaDeNew;
    t_queue *colaDeNew;
    sem_t* cantidadProcesosEnNew;
    t_datos_configuracion* datosConfiguracion;
    pthread_mutex_t* mutexDatosConfiguracion
} parametros_thread_k;

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
    int contadorPID;
    t_log *logger;
} parametros_plp;

//Estructura necesaria para el PCP
typedef struct {
    int *quantum;
    int *retardoEjecucion;
    t_queue *colaDeReady;
    sem_t *mutexColaDeReady;
    t_queue *colaDeFinalizados;
    sem_t *mutexColaFinalizados;
    sem_t *cantidadProcesosEnReady;
    pthread_mutex_t *mutexJournal;
    pthread_mutex_t *mutexSemaforoHilo;
    t_log *logger;
} parametros_pcp;

//Estructura hibrida necesaria para planificacion
typedef struct {
    p_consola_kernel *parametrosConsola;
    parametros_plp *parametrosPLP;
    parametros_pcp *parametrosPCP;
    t_dictionary *supervisorDeHilos;
    t_metricas *metricas;
    int memoriasUtilizables;
    double relojActual;
    t_list* memoriasConocidas;
} p_planificacion;

//Estructura
typedef struct {
    char* ipNodoMemoria;
    int puertoNodoMemoria;
    int fdNodoMemoria;
    int memoryNumber;
} t_nodoMemoria;

p_planificacion *paramPlanificacionGeneral;

//Conexión con Memoria
//crearHiloConexiones(misConexiones, logger, tablaDeMemoriasConCriterios, metadataTablas, mutexJournal, supervisorDeHilos, memoriasConocidas, mutexColaDeNew, colaDeNew, cantidadProcesosEnNew, datosConfiguracion, mutexDatosConfiguracion);
pthread_t *crearHiloConexiones(GestorConexiones *unaConexion, t_log *logger, t_dictionary *tablaDeMemoriasConCriterios, t_dictionary *metadataTabla, pthread_mutex_t *mutexJournal, t_dictionary *visorDeHilos, t_list* memoriasConocidas, sem_t *semaforo_colaDeNew, t_queue *colaDeNew, sem_t* cantidadProcesosEnNew, t_datos_configuracion* datosConfiguracion, pthread_mutex_t* mutexDatosConfiguracion);

void *atenderConexiones(void *parametrosThread);

void atenderMensajes(Header header, char *mensaje);

void confirmarHandshake(Header header, parametros_thread parametros);

void desconexionMemoria(int fdConectado, GestorConexiones *conexiones, t_dictionary *tablaDeMemoriasConCriterios,
                        t_log *logger, pthread_mutex_t *mutexJournal);

void gestionarRespuesta(int fdMemoria, int pid, TipoRequest tipoRequest, t_dictionary *supervisorDeHilos,
                        t_dictionary *memoriasConCriterios, t_dictionary *metadata, char *mensaje, t_log *logger);

char **obtenerDatosDeConexion(char *datosConexionMemoria); //para Gossiping

void borrarFdDeListaDeFdsConectados(int fdADesconectar, t_dictionary *tablaMemoriasConCriterios, char *criterio);

void actualizarMetadata(t_dictionary *metadata, char *mensaje, t_log *logger);

bool tenemosMemoriaEnListaDeMemorias(t_list *listaDeNodosMemorias, t_nodoMemoria *nodoDatosDeMemoria);

//GOSSPING

void agregarIpMemoria(char* ipNuevaMemoria, int puertoNuevaMemoria, t_list* memoriasConocidas, t_log* logger);
void agregarMemoriasRecibidas(char* memoriasRecibidas, t_list* memoriasConocidas, t_log* logger);
void forzarJournalingEnTodasLasMemorias(GestorConexiones* misConexiones, sem_t *semaforo_colaDeNew, t_queue *colaDeNew, sem_t* cantidadProcesosEnNew, t_log* logger);

//Inotify

void avisoNuevoNivelDeMultiProcesamiento(char* nuevoNivelDeMP, t_list* memoriasConocidas);


//Estructura necesaria para el manejo de archivosLQL
typedef struct {
    t_queue *colaDeRequests;//cada Request va a ser un t_comando
    int cantidadDeLineas;
    int PID;
} t_archivoLQL;


#endif //KERNEL_CONEXIONES_H
