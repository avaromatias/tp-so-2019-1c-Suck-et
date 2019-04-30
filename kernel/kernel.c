/*
 ============================================================================
 Name        : Kernel.c
 Author      : Suck et
 	 UTN FRBA - Sistemas Operativos
 	 	 TP-1C-2019-Lissandra
 ============================================================================
 */

#include "kernel.h"

int main(void) {
    puts("¡Hola! Soy Kernel!");

    logger = log_create("../kernel.log", "kernel", false, LOG_LEVEL_INFO);
    log_info(logger, "Iniciando el proceso Kernel...");

	configuracion = cargarConfiguracion("kernel.cfg", logger);

    log_info(logger, "IP Memoria: %s", configuracion.ipMemoria;
    log_info(logger, "Puerto Memoria: %i", configuracion.puertoMemoria);
    log_info(logger, "Quantum: %i", configuracion.quantum);
    log_info(logger, "Multiprocesamiento: %i", configuracion.multiprocesamiento);
    log_info(logger, "Refresh Metadata: %i", configuracion.refreshMetadata);
    log_info(logger, "Retardo de Ejecución : %i", configuracion.refreshMetadata);

   	ejecutarConsola(&gestionarComando,"kernel");

	//conectar con memoria y luego el paso de abajo
	int fdDestinatario = crearSocketCliente(configuracion.ipMemoria, configuracion.puertoMemoria);

	/*while(1)	{
    enviarPaquete(fdDestinatario, "DESCRIBE TABLE 2\n");
    sleep(3);}*/

	return EXIT_SUCCESS;
}
t_configuracion cargarConfiguracion(char* pathArchivoConfiguracion, t_log* logger)	{
	t_configuracion configuracion;

	t_config* archivoConfig = abrirArchivoConfiguracion(pathArchivoConfiguracion, logger); //nos devuelve un archivoConfig

	bool existenTodasLasClavesObligatorias(t_config* archivoConfig, t_configuracion configuracion)	{
		char* clavesObligatorias[6] = {
				"IP_MEMORIA",
				"PUERTO_MEMORIA",
				"QUANTUM",
				"MULTIPROCESAMIENTO",
				"METADATA_REFRESH",
				"SLEEP_EJECUCION"
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
	}	else	{
		configuracion.ipMemoria = config_get_string_value(archivoConfig, "IP_MEMORIA");
		configuracion.puertoMemoria = config_get_int_value(archivoConfig, "PUERTO_MEMORIA");
		configuracion.quantum = config_get_int_value(archivoConfig, "QUANTUM");
		configuracion.multiprocesamiento = config_get_int_value(archivoConfig, "MULTIPROCESAMIENTO");
		configuracion.refreshMetadata = config_get_int_value(archivoConfig, "METADATA_REFRESH");
		configuracion.retardoEjecucion = config_get_int_value(archivoConfig, "SLEEP_EJECUCION");

		return configuracion;
	}
}

int gestionarComando(char **request) {
   //Comportamiento similar a las otras consolas, se diferencia en entender Journal, ADD, Metrics y RUN

}