//
// Created by utnso on 12/04/19.
//

#include "config.h"

bool existeElArchivo(char* path)    {
    return access(path, F_OK) != -1;
}

bool sePuedeLeerElArchivo(char* path)  {
    return access(path, R_OK) != -1;
}

bool sePuedeEscribirElArchivo(char* path)  {
    return access(path, W_OK) != -1;
}

t_config* abrirArchivoConfiguracion(char* pathArchivo, t_log* logger)  {
    t_config* archivoConfig = NULL;
    if(sePuedeLeerElArchivo(pathArchivo))
        archivoConfig = config_create(pathArchivo);
    else    {
        log_error(logger, "El archivo de configuración %s no existe o no tiene permisos de lectura.", pathArchivo);
    }
    return archivoConfig;
}