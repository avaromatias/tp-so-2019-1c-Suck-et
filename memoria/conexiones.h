//
// Created by utnso on 04/05/19.
//

#ifndef MEMORIA_CONEXIONES_H
#define MEMORIA_CONEXIONES_H

#include "../libs/sockets.h"
#include "../libs/generales.h"

void* atenderConexiones(void* parametrosThread);

pthread_t* crearHiloConexiones(GestorConexiones* conexion, t_log* logger);

void atenderMensajes(Header header, char* mensaje);

#endif //MEMORIA_CONEXIONES_H