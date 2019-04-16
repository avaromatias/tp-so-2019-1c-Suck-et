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

//Funciones Sockets
//Para Socket Servidor

int crearSocketServidor(int);
int crearSocketEscucha (int);
void escucharSocketsEn(int);
int aceptarCliente(int);

//Funciones Sockets Clientes
int crearSocketCliente(char*, int);
void cerrarSocket(int);


#endif //LIBS_SOCKETS_H
