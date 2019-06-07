//
// Created by utnso on 04/05/19.
//

#ifndef MEMORIA_CONEXIONES_H
#define MEMORIA_CONEXIONES_H

#include <semaphore.h>
#include "../libs/sockets.h"

typedef struct  {
    int fd;
    sem_t* semaforo;
} t_control_conexion;

typedef struct {
    GestorConexiones* conexion;
    t_log* logger;
    t_control_conexion* conexionKernel;
} parametros_thread_memoria;

void* atenderConexiones(void* parametrosThread);

pthread_t* crearHiloConexiones(GestorConexiones* conexion, t_control_conexion* conexionKernel, t_log* logger);

void atenderMensajes(Header header, void* mensaje, parametros_thread_memoria* parametros);
void atenderHandshake(Header header, Componente componente, parametros_thread_memoria* parametros);

char* recibirMensaje(t_control_conexion* conexion);

void conectarseALissandra(t_control_conexion* conexionLissandra, char* ipLissandra, int puertoLissandra, t_log* logger);

#endif //MEMORIA_CONEXIONES_H