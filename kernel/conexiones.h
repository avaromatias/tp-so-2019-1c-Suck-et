//
// Created by utnso on 12/05/19.
//

#ifndef KERNEL_CONEXIONES_H
#define KERNEL_CONEXIONES_H

#include <semaphore.h>
#include <commons/collections/dictionary.h>
#include "../libs/sockets.h"

typedef struct {
    GestorConexiones* conexion;
    t_log* logger;
    int* fdMemoria;
} parametros_thread_memoria;

typedef struct	{
    t_log* logger;
    GestorConexiones* conexion;
    t_dictionary *tablaDeMemoriasConCriterios;
} parametros_thread_k;

//Conexión con Memoria
pthread_t *crearHiloConexiones(GestorConexiones *unaConexion, t_log *logger, t_dictionary *tablaDeMemoriasConCrits);
void* atenderConexiones(void* parametrosThread);
void atenderMensajes(Header header, char* mensaje);
void confirmarHandshake(Header header, parametros_thread parametros);
void desconexionMemoria(int fdMemoria, t_dictionary *memConCriterios, t_log *logger);

#endif //KERNEL_CONEXIONES_H
