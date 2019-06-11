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
    int* fdMemoria;
} parametros_thread_memoria;

typedef enum {
    SC,
    SHC,
    EC,
    NC
} Criterio; //NC = NO CRITERY

/*typedef struct {
    int fdMemoria;
    Criterio consistencia;
    int utilizacion;
} t_memoria_conocida;*/

typedef struct  {
    TipoMensaje tipoMensaje;
    char* mensaje;
} t_paquete;

//Conexión con Memoria
pthread_t* crearHiloConexiones(GestorConexiones* unaConexion, t_log* logger);
void* atenderConexiones(void* parametrosThread);
void atenderMensajes(Header header, char* mensaje);

#endif //KERNEL_CONEXIONES_H
