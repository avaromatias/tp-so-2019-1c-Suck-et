#ifndef CONEXIONES_H_
#define CONEXIONES_H_

#include <sys/select.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <commons/collections/list.h>
#include <sys/socket.h>
#include "sockets.h"
#include <commons/string.h>

typedef struct {
    int descriptorMaximo;
    int servidor;
    t_list * clientes;

} t_conexion;

typedef struct	{
	t_conexion conexion;
	void (*funcionRecepcionMensajes)(Header, void*);
	void (*funcionDesconexionClientes)(int);
	void (*funcionConexionClientes)(int);
} parametros_thread;

void * atenderConexiones(void*);
void crearHiloServidor(int puerto, void (*gestionarMensajes)(Header, void*), void (*gestionarDesconexiones)(int), void (*gestionarConexiones)(int));
void cargarListaClientes(t_conexion* unaConexion, fd_set* solicitantes, fd_set* respondidos);
Message* empaquetarTexto(char* texto, unsigned int length);
void desconectarCliente(int fdCliente, t_conexion unaConexion);

Header armarHeader(int tamanioDelMensaje);
void* serializarHeader(Header header);
Header deserializarHeader(void* headerSerializado);
void* empaquetar(void* headerSerializado, char* mensaje);
void enviarPaquete(int fdDestinatario, char* mensaje);

#endif /* CONEXIONES_H_ */
