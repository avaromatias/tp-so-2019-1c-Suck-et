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

typedef struct {
    int tamanioMensaje;
    int fdRemitente;
} __attribute__((packed)) Header;

typedef struct {
    int descriptorMaximo;
    int servidor;
    t_list * clientes;

} t_conexion;

typedef struct	{
    t_conexion conexion;
    void (*funcionRecepcionMensajes)(Header, char*);
    void (*funcionDesconexionClientes)(int);
    void (*funcionConexionClientes)(int);
} parametros_thread;

int crearSocketServidor(int);
int crearSocketEscucha (int);
void escucharSocketsEn(int);
int aceptarCliente(int);

//Funciones Sockets Clientes
int crearSocketCliente(char*, int);
void cerrarSocket(int);

Header armarHeader(int tamanioDelMensaje);
void* serializarHeader(Header header);
Header deserializarHeader(void* headerSerializado);
void* empaquetar(void* headerSerializado, char* mensaje);
void enviarPaquete(int fdDestinatario, char* mensaje);

void desconectarCliente(int, t_conexion);

void* atenderConexiones(void*);

void crearHiloServidor(int puerto, void (*gestionarMensajes)(Header, char*), void (*gestionarDesconexiones)(int), void (*gestionarConexiones)(int));

#endif //LIBS_SOCKETS_H
