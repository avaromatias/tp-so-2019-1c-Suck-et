/*
 ============================================================================
 Name        : LFS.c
 Author      : Suck et
 	 UTN FRBA - Sistemas Operativos
 	 	 TP-1C-2019-Lissandra
 ============================================================================
 */

#include "lfs.h"

t_configuracion cargarConfiguracion(char* pathArchivoConfiguracion, t_log* logger)	{

	t_config* archivoConfig = abrirArchivoConfiguracion(pathArchivoConfiguracion, logger);

    bool existenTodasLasClavesObligatorias(t_config* archivoConfig, t_configuracion configuracion)	{
        char* clavesObligatorias[5] = {
                "PUERTO_ESCUCHA",
                "PUNTO_MONTAJE",
                "RETARDO",
                "TAMAÑO_VALUE",
                "TIEMPO_DUMP"
        };
        
        for(int i = 0; i < COUNT_OF(clavesObligatorias); i++)	{
            if(!config_has_property(archivoConfig, clavesObligatorias[i]))
                return false;
        }

        return true;
    }

	if(!existenTodasLasClavesObligatorias(archivoConfig, configuracion)){
		log_error(logger, "Alguna de las claves obligatorias no están setteadas en el archivo de configuración.");
		exit(1); // settear algún código de error para cuando falte alguna key
	}
	else{
		configuracion.puertoEscucha = config_get_int_value(archivoConfig, "PUERTO_ESCUCHA");
		configuracion.puntoMontaje = config_get_string_value(archivoConfig, "PUNTO_MONTAJE");
		configuracion.retardo = config_get_int_value(archivoConfig, "RETARDO");
		configuracion.tamanioValue = config_get_int_value(archivoConfig, "TAMAÑO_VALUE");
		configuracion.tiempoDump = config_get_int_value(archivoConfig, "TIEMPO_DUMP");

		return configuracion;
	}
}

void atenderMensajes(Header header, char* mensaje)    {
    printf("Estoy recibiendo un mensaje del file descriptor %i: %s", header.fdRemitente, mensaje);
}

int main(void) {
    t_log* logger = log_create("../lfs.log", "lfs", true, LOG_LEVEL_INFO);

    log_info(logger, "Iniciando proceso Lissandra File System");

	configuracion = cargarConfiguracion("../lfs.cfg", logger);

    log_info(logger, "Puerto Escucha: %i", configuracion.puertoEscucha);
    log_info(logger, "Punto Montaje: %s", configuracion.puntoMontaje);
    log_info(logger, "Retardo: %i", configuracion.retardo);
    log_info(logger, "Tamaño value: %i", configuracion.tamanioValue);
    log_info(logger, "Tiempo dump: %i", configuracion.tiempoDump);

    crearHiloServidor(configuracion.puertoEscucha, &atenderMensajes, NULL, NULL);

	return 0;
}
