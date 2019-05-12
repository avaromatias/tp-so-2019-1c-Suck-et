//
// Created by utnso on 04/05/19.
//

#ifndef MEMORIA_CONEXIONES_H
#define MEMORIA_CONEXIONES_H

#include <semaphore.h>
#include "../libs/sockets.h"

typedef struct {
    GestorConexiones* conexion;
    t_log* logger;
    sem_t* kernelConectado;
    int* fdKernel;
} parametros_thread_memoria;

void* atenderConexiones(void* parametrosThread);

pthread_t* crearHiloConexiones(GestorConexiones* conexion, int* fdKernel, sem_t* kernelConectado, t_log* logger);

void atenderMensajes(Header header, void* mensaje, parametros_thread_memoria* parametros);

char* recibirMensaje(int* fdEmisor);

#endif //MEMORIA_CONEXIONES_H