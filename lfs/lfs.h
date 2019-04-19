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
#include <commons/config.h>
#include <commons/log.h>

#include "../libs/config.h"
#include "../libs/sockets.h"
#include "../libs/generales.h"


//Variables y estructuras
typedef struct {
    int puertoEscucha;
    char* puntoMontaje;
    int retardo;
    int tamanioValue;
    int tiempoDump;
} t_configuracion;

t_configuracion configuracion;


//Header de funciones
t_configuracion cargarConfiguracion(char* path, t_log* logger);


#endif /* LFS_H_ */
