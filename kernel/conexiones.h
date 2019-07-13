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
    pthread_mutex_t *mutexJournal;
} parametros_thread_k;

//Conexión con Memoria
pthread_t *crearHiloConexiones(GestorConexiones *unaConexion, t_log *logger, t_dictionary *tablaDeMemoriasConCrits,
                               pthread_mutex_t *mutexJournal);

void *atenderConexiones(void *parametrosThread);

void atenderMensajes(Header header, char *mensaje);

void confirmarHandshake(Header header, parametros_thread parametros);

void desconexionMemoria(int fdConectado, GestorConexiones *conexiones, t_dictionary *tablaDeMemoriasConCriterios,
                        t_log *logger, pthread_mutex_t *mutexJournal);

void
gestionarRespuesta(int fdMemoria, t_dictionary *metadataTab, TipoRequest tipoRequest, char *mensaje, t_log *logger);

void actualizarMetadata(t_dictionary *metadata, char *mensaje, t_log *logger);

void enviarJournal(int fdMemoria);

char **obtenerDatosDeConexion(char *datosConexionMemoria); //para Gossiping

#endif //KERNEL_CONEXIONES_H
