/*
 ============================================================================
 Name        : Memoria.c
 Author      : Suck et
 	 UTN FRBA - Sistemas Operativos
 	 	 TP-1C-2019-Lissandra
 ============================================================================
 */

#include "memoria.h"

bool existenTodasLasClavesObligatorias(t_config* archivoConfig, t_configuracion configuracion, char* clavesObligatorias[])	{
	for(int i = 0; sizeof(clavesObligatorias); i++)	{
		if(!config_has_property(archivoConfig, clavesObligatorias[i]))
			return false;
	}

	return true;
}

t_configuracion cargarConfiguracion()	{
	t_config* archivoConfig = config_create("memoria.cfg");
	t_configuracion configuracion;

	char* clavesObligatorias[11] = {"PUERTO", "IP_FS", "PUERTO_FS", "IP_SEEDS", "PUERTO_SEEDS", "RETARDO_MEM", "RETARDO_FS", "TAM_MEM", "RETARDO_JOURNAL", "RETARDO_GOSSIPING", "MEMORY_NUMBER"};

	if(existenTodasLasClavesObligatorias(archivoConfig, configuracion, clavesObligatorias))
		exit(1);
	else	{
		configuracion.puerto = config_get_int_value(archivoConfig, "PUERTO");
		configuracion.ipFileSystem = config_get_string_value(archivoConfig, "IP_FS");
		configuracion.puertoFileSystem = config_get_int_value(archivoConfig, "PUERTO_FS");
		configuracion.ipSeeds = config_get_array_value(archivoConfig, "IP_SEEDS");
		configuracion.puertoSeeds = config_get_array_value(archivoConfig, "PUERTO_SEEDS");
		configuracion.retardoMemoria = config_get_int_value(archivoConfig, "RETARDO_MEM");
		configuracion.retardoFileSystem = config_get_int_value(archivoConfig, "RETARDO_FS");
		configuracion.tamanioMemoria = config_get_int_value(archivoConfig, "TAM_MEM");
		configuracion.retardoJournal = config_get_int_value(archivoConfig, "RETARDO_JOURNAL");
		configuracion.retardoGossiping = config_get_int_value(archivoConfig, "RETARDO_GOSSIPING");
		configuracion.cantidadDeMemorias = config_get_int_value(archivoConfig, "MEMORY_NUMBER");

		return configuracion;
	}
}

int main(void) {
	puts("¡Hola! Soy Memory!");

	t_configuracion configuracion = cargarConfiguracion();

	puts(configuracion.ipFileSystem);

	return 1;
}
