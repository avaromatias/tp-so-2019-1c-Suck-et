/*
 ============================================================================
 Name        : LFS.h
 Author      : Suck et
 	 UTN FRBA - Sistemas Operativos
 	 	 TP-1C-2019-Lissandra
 ============================================================================
 */


#ifndef LFS_H_
#define LFS_H_


#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <commons/config.h>
#include <commons/log.h>

#include "../libs/config.h"
#include "../libs/sockets.h"
#include "../libs/consola.h"


//Variables y estructuras
typedef struct {
    int puertoEscucha;
    char *puntoMontaje;
    int retardo;
    int tamanioValue;
    int tiempoDump;
} t_configuracion;

t_configuracion configuracion;
t_log *logger;
char *rutaTablas;


//Header de funciones
t_configuracion cargarConfiguracion(char *path, t_log *logger);

void atenderMensajes(Header header, char *mensaje);

void lfsInsert(char *nombreTabla, char *key, char *valor, time_t timestamp);

void lfsSelect(char *nombreTabla, char *key);

char *obtenerPathTabla(char *nombreTabla);

char *obtenerPathArchivo(char *nombreTabla, char *nombreArchivo);

char *armarLinea(char *key, char *valor, time_t timestamp);

/**
* @NAME: gestionarRequest
* @DESC: Gestiona los comandos recibidos por consola y decide como proceder
*
* @RETURN:
* -2: Comando invalido
* -1: Numero de parametros invalido
* 0: Ejecucion exitosa
*/
int gestionarRequest(char **request);

/**
* @NAME: existeTabla
* @DESC: Valida si existe el directorio de la tabla especificada por parametro
*
* @RETURN:
* -1: No se encontro o no se pudo acceder a la tabla especificada
* 0: Existe el directorio especificado
*/
int existeTabla(char *path);


#endif /* LFS_H_ */
