//
// Created by utnso on 18/04/19.
//

#ifndef LIBS_CONSOLA_H
#define LIBS_CONSOLA_H

#include <commons/string.h>
#include <commons/log.h>
#include <stdlib.h>
#include <string.h>
#include "../libs/sockets.h"
#include <readline/readline.h>

bool validarComandosComunes(t_comando comando);
//int validarComandosKernel(char** comando);
//void ejecutarConsola(int (*gestionarComando)(char**), Componente nombreDelProceso, t_log *logger);
//void ejecutarConsola(void *);
char *obtenerPathTabla(char *nombreTabla, char* puntoMontaje);
char *obtenerPathMetadata(char *nombreTabla,char* puntoMontaje);
void imprimirErrorParametros();

typedef struct{
    t_log* logger;
    Componente unComponente;
    int (*gestionarComando)(char**);

}parametros_consola;

#endif //LIBS_CONSOLA_H
