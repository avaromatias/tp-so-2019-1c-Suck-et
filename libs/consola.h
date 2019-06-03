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

int validarComandosComunes(char** comando);
char *obtenerPathTabla(char *nombreTabla);
char *obtenerPathMetadata(char *nombreTabla);
void imprimirErrorParametros();

typedef struct{
    t_log* logger;
    Componente unComponente;
    int (*gestionarComando)(char**);

}parametros_consola;

#endif //LIBS_CONSOLA_H
