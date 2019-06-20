//
// Created by utnso on 04/05/19.
//

#ifndef MEMORIA_CONEXIONES_H
#define MEMORIA_CONEXIONES_H

#include <semaphore.h>
#include "../libs/sockets.h"

typedef struct t_control_conexion_d t_control_conexion;

#include "memoria.h"

struct t_control_conexion_d {
    int fd;
    sem_t* semaforo;
};

typedef struct {
    GestorConexiones* conexion;
    t_log* logger;
    t_control_conexion* conexionKernel;
    t_memoria* memoria;
} parametros_thread_memoria;

typedef struct  {
    TipoMensaje tipoMensaje;
    char* mensaje;
} t_paquete;

void* atenderConexiones(void* parametrosThread);

pthread_t* crearHiloConexiones(GestorConexiones* conexion, t_control_conexion* conexionKernel, t_log* logger);

void atenderMensajes(Header header, void* mensaje, parametros_thread_memoria* parametros);
void atenderHandshake(Header header, Componente componente, parametros_thread_memoria* parametros);

t_paquete recibirMensaje(t_control_conexion* conexion);

void conectarseALissandra(t_control_conexion* conexionLissandra, char* ipLissandra, int puertoLissandra, t_log* logger);
bool conectarseANodoMemoria(char* unaIp, int unPuerto, t_log* logger);

#endif //MEMORIA_CONEXIONES_H