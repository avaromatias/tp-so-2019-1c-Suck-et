/*
 ============================================================================
 Name        : Kernel.h
 Author      : Suck et
 	 UTN FRBA - Sistemas Operativos
 	 	 TP-1C-2019-Lissandra
 ============================================================================
 */


#ifndef KERNEL_H_
#define KERNEL_H_

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <commons/config.h>
#include <commons/log.h>
#include <commons/collections/queue.h>
#include "../libs/config.h"
#include "../libs/sockets.h"
#include "../libs/consola.h"
#include "conexiones.h"

typedef struct {
    char* ipMemoria;
    int puertoMemoria;
    int quantum;
    int multiprocesamiento;
    int refreshMetadata;
    int retardoEjecucion;
} t_configuracion;

//******Vars locales******
t_configuracion configuracion;
t_log* logger;

//Para Planificador
t_queue* colaDeNew;
t_queue* colaDeReady;
t_queue* colaDeExecute;
t_list* finalizados;

//Funciones propias de Kernel
t_configuracion cargarConfiguracion(char*, t_log*);
int gestionarComando(char **);
int validarComandosKernel(char* tipoDeRequest, char* nombreTabla, char* param1, char* param2, char* param3);
void ejecutarConsola(int (*gestionarComando)(char**), Componente nombreDelProceso, t_log *logger);

#endif /* KERNEL_H_ */
