//
// Created by utnso on 12/05/19.
//

#ifndef KERNEL_CONEXIONES_H
#define KERNEL_CONEXIONES_H

#include <semaphore.h>
#include "../libs/sockets.h"

typedef struct {
    GestorConexiones* conexion;
    t_log* logger;
    sem_t* kernelConectado;
    int* fdKernel;
} parametros_thread_memoria;

//Conexión con Memoria
pthread_t* crearHiloConexiones(GestorConexiones* unaConexion, t_log* logger);
void* atenderConexiones(void* parametrosThread);
void atenderMensajes(Header header, char* mensaje);

#endif //KERNEL_CONEXIONES_H
