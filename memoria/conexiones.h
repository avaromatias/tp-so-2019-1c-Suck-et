//
// Created by utnso on 04/05/19.
//

#ifndef MEMORIA_CONEXIONES_H
#define MEMORIA_CONEXIONES_H

#include <semaphore.h>
#include "../libs/sockets.h"

typedef struct t_control_conexion_d t_control_conexion;
typedef struct parametros_thread_requests_d parametros_thread_request;
typedef struct t_paquete_d t_paquete;

#include "memoria.h"

struct t_control_conexion_d {
    int fd;
    sem_t* semaforo;
};

typedef struct {
    GestorConexiones* conexion;
    t_log* logger;
    t_control_conexion* conexionKernel;
    t_control_conexion* conexionLissandra;
    t_memoria* memoria;
    pthread_mutex_t* semaforoMemoriasConocidas;
    pthread_mutex_t* semaforoJournaling;
} parametros_thread_memoria;

struct  t_paquete_d {
    TipoMensaje tipoMensaje;
    char* mensaje;
};

struct parametros_thread_requests_d {
    t_comando comando;
    t_control_conexion* conexionKernel;
    t_log* logger;
    t_memoria* memoria;
    t_control_conexion* conexionLissandra;
    pthread_mutex_t* semaforoJournaling;
};

void* atenderConexiones(void* parametrosThread);

pthread_t* crearHiloConexiones(GestorConexiones* unaConexion, t_memoria* memoria, t_control_conexion* conexionKernel, t_control_conexion* conexionLissandra, t_log* logger, pthread_mutex_t* semaforoMemoriasConocidas, pthread_mutex_t* semaforoJournaling);

void atenderMensajes(Header header, void* mensaje, parametros_thread_memoria* parametros);
void atenderHandshake(Header header, Componente componente, parametros_thread_memoria* parametros);
void atenderPedidoMemoria(Header header,char* mensaje, parametros_thread_memoria* parametros);

t_paquete recibirMensajeDeLissandra(t_control_conexion *conexion);

void conectarseALissandra(t_control_conexion* conexionLissandra, char* ipLissandra, int puertoLissandra, t_log* logger);

//Gossiping
char* concatenarMemoriasConocidas(t_list* memoriasConocidas);
void agregarMemoriasRecibidas(char* memoriasRecibidas, t_list* memoriasConocidas, t_log* logger);
void enviarPedidoGossiping(int fdDestinatario, t_list* memoriasConocidas, pthread_mutex_t* semaforoMemoriasConocidas, t_log* logger);


#endif //MEMORIA_CONEXIONES_H