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
#include "../libs/config.h"
#include <regex.h>

char *obtenerPathTabla(char *nombreTabla, char *puntoMontaje);

char *obtenerPathMetadata(char *nombreTabla, char *puntoMontaje);

void imprimirErrorParametros();

int parametrosValidos(t_comando comando);

void comandoInvalido();

void imprimirErrorParametros();

void cantidadIncorrectaParametros();

bool matchea(char *palabra, char *expresion);

bool esEntero(char *palabra);

bool esString(char *palabra);

bool validarComandosComunes(t_comando comando, t_log* logger);

typedef struct {
    t_log *logger;
    Componente unComponente;

    char* (*gestionarComando)(t_comando);

} parametros_consola;

#endif //LIBS_CONSOLA_H
