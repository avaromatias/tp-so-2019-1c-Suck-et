#ifndef SUCKET_CONFIG_H
#define SUCKET_CONFIG_H
#define COUNT_OF(x) ((sizeof(x)/sizeof(0[x])) / ((size_t)(!(sizeof(x) % sizeof(0[x])))))

#include <unistd.h>
#include <commons/config.h>
#include <commons/log.h>

bool existeElArchivo(char* path);
bool sePuedeLeerElArchivo(char* path);
bool sePuedeEscribirElArchivo(char* path);
t_config* abrirArchivoConfiguracion(char* pathArchivo, t_log* logger);

#endif //SUCKET_CONFIG_H
