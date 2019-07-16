//
// Created by utnso on 12/05/19.
//

#ifndef KERNEL_CONEXIONES_H
#define KERNEL_CONEXIONES_H

#include <semaphore.h>
#include <commons/collections/queue.h>
#include <commons/collections/dictionary.h>
#include "../libs/sockets.h"

typedef struct {
    GestorConexiones *conexion;
    t_log *logger;
    int *fdMemoria;
} parametros_thread_memoria;

typedef struct {
    t_log *logger;
    GestorConexiones *conexion;
    t_dictionary *tablaDeMemoriasConCriterios;
    t_dictionary *metadataTabla;
    pthread_mutex_t *mutexJournal;
    t_dictionary *supervisorDeHilos;
    pthread_mutex_t *mutexSemaforoHilo;
} parametros_thread_k;

//Conexión con Memoria
pthread_t *crearHiloConexiones(GestorConexiones *unaConexion, t_log *logger, t_dictionary *tablaDeMemoriasConCriterios,
                               t_dictionary *metadataTabla, pthread_mutex_t *mutexJournal, t_dictionary *visorDeHilos);

void *atenderConexiones(void *parametrosThread);

void atenderMensajes(Header header, char *mensaje);

void confirmarHandshake(Header header, parametros_thread parametros);

void desconexionMemoria(int fdConectado, GestorConexiones *conexiones, t_dictionary *tablaDeMemoriasConCriterios,
                        t_log *logger, pthread_mutex_t *mutexJournal);

void gestionarRespuesta(int fdMemoria, int pid, TipoRequest tipoRequest, t_dictionary *supervisorDeHilos,
                        t_dictionary *memoriasConCriterios, t_dictionary *metadata, char *mensaje, t_log *logger);

void enviarJournal(int fdMemoria);

char **obtenerDatosDeConexion(char *datosConexionMemoria); //para Gossiping

void borrarFdDeListaDeFdsConectados(int fdADesconectar, t_dictionary *tablaMemoriasConCriterios, char *criterio);

void actualizarMetadata(t_dictionary *metadata, char *mensaje, t_log *logger);

#endif //KERNEL_CONEXIONES_H
