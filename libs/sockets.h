#ifndef LIBS_SOCKETS_H

#define LIBS_SOCKETS_H
#define ERROR -1 // Las system-calls de sockets retornan -1 en caso de error
#define conexionesMaximasPermitidas 100

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <arpa/inet.h> //INADDR_ANY e INET_ADDR
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <commons/log.h>
#include <pthread.h>
#include <commons/collections/list.h>
#include <stdarg.h>
#include "generales.h"

typedef enum  {
    REQUEST,
    RESPUESTA,
    HANDSHAKE,
    GOSSIPING,
    RESPUESTA_GOSSIPING,
    RESPUESTA_GOSSIPING_2,
    ERR,
    CONEXION_ACEPTADA,
    CONEXION_RECHAZADA,
    NIVEL_MULTIPROCESAMIENTO,
    JOURNALING
} TipoMensaje;

typedef enum {
    KERNEL,
    MEMORIA,
    LISSANDRA
} Componente;

typedef struct {
    int tamanioMensaje;
    int fdRemitente;
    TipoMensaje tipoMensaje;
    TipoRequest tipoRequest;
    int pid;
} __attribute__((packed)) Header;

typedef struct  {
    int descriptorMaximo;
    int servidor;
    t_list* conexiones;
} GestorConexiones;

typedef struct	{
    t_log* logger;
    GestorConexiones* conexion;
} parametros_thread;

int crearSocketServidor(int, t_log*);
int crearSocketEscucha (int, t_log*);
void escucharSocketsEn(int, t_log*);
int aceptarCliente(int, t_log*);

//Funciones Sockets Clientes
int crearSocketCliente(char*, int, t_log*);
void cerrarSocket(int, t_log*);

Header armarHeader(int fdDestinatario, int tamanioDelMensaje, TipoMensaje tipoMensaje, TipoRequest tipoRequest, int pid);
void* serializarHeader(Header header);
Header deserializarHeader(void* headerSerializado);
void* empaquetar(void* headerSerializado, char* mensaje);
void enviarPaquete(int fdDestinatario, TipoMensaje tipoMensaje, TipoRequest tipoRequest, char* mensaje, int pid);

void desconectarCliente(int fdCliente, GestorConexiones* unaConexion, t_log* logger);

void levantarServidor(int puerto, GestorConexiones* conexion, t_log* logger);

bool hayNuevoMensaje(GestorConexiones* unaConexion, fd_set* emisores);

void setDescriptorMaximo(GestorConexiones* conexion);

void cargarListaClientes(GestorConexiones* unaConexion, fd_set* solicitantes);

GestorConexiones* inicializarConexion();

int conectarseAServidor(char* ip, int puerto, GestorConexiones* conexion, t_log* logger);

int getFdMaximo(GestorConexiones* conexion);

void eliminarFdDeListaDeConexiones(int fdCliente, GestorConexiones* unaConexion);

void hacerHandshake(int fdDestinatario, Componente componente);

#endif //LIBS_SOCKETS_H