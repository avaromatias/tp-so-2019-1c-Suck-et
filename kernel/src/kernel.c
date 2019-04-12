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
	t_configuracion configuracion = cargarConfiguracion();
	puts(configuracion.ipMemoria);

	//conectar con memoria y luego el paso de abajo
	//levantarAPI();

	return EXIT_SUCCESS;
}

t_configuracion cargarConfiguracion()	{
	t_config* archivoConfig = config_create("kernel.cfg");
	t_configuracion configuracion;

	char* clavesObligatorias[6] = {"IP_MEMORIA", "PUERTO_MEMORIA", "QUANTUM", "MULTIPROCESAMIENTO", "METADATA_REFRESH", "RETARDO_EJECUCION"};

	if(existenTodasLasClavesObligatorias(archivoConfig, configuracion, clavesObligatorias))
		exit(1);
	else	{
		configuracion.ipMemoria = config_get_int_value(archivoConfig, "IP_MEMORIA");
		configuracion.puertoMemoria = config_get_string_value(archivoConfig, "PUERTO_MEMORIA");
		configuracion.quantum = config_get_int_value(archivoConfig, "QUANTUM");
		configuracion.multiprocesamiento = config_get_array_value(archivoConfig, "MULTIPROCESAMIENTO");
		configuracion.refreshMetadata = config_get_array_value(archivoConfig, "METADATA_REFRESH");
		configuracion.retardoEjecucion = config_get_int_value(archivoConfig, "RETARDO_EJECUCION");

		return configuracion;
	}
}

bool existenTodasLasClavesObligatorias(t_config* archivoConfig, t_configuracion configuracion, char* clavesObligatorias[])	{
	for(int i = 0; sizeof(clavesObligatorias); i++)	{
		//vamos a ir recorriendo el array con las configuraciones del archivoConfig
		if(!config_has_property(archivoConfig, clavesObligatorias[i]))
			return false;
	}

	return true;
}

void levantarAPI()	{

}
