//
// Created by utnso on 18/04/19.
//

#ifndef LIBS_CONSOLA_H
#define LIBS_CONSOLA_H

#include <commons/string.h>
#include <stdlib.h>
#include <string.h>
#include "../libs/generales.h"
#include <readline/readline.h>

int validarComandosComunes(char** comando);
void ejecutarConsola(void (*gestionarComando)(char**), char* nombreDelProceso);
char *obtenerPathTabla(char *nombreTabla);
char *obtenerPathMetadata(char *nombreTabla);

#endif //LIBS_CONSOLA_H
