/*
 ============================================================================
 Name        : Memoria.c
 Author      : Suck et
 	 UTN FRBA - Sistemas Operativos
 	 	 TP-1C-2019-Lissandra
 ============================================================================
 */

#include <sockets.h>
#include "memoria.h"



t_configuracion cargarConfiguracion(char* pathArchivoConfiguracion, t_log* logger)	{
	t_configuracion configuracion;

	t_config* archivoConfig = abrirArchivoConfiguracion(pathArchivoConfiguracion, logger);

    bool existenTodasLasClavesObligatorias(t_config* archivoConfig, t_configuracion configuracion)	{
        char* clavesObligatorias[11] = {
                "PUERTO",
                "IP_FS",
                "PUERTO_FS",
                "IP_SEEDS",
                "PUERTO_SEEDS",
                "RETARDO_MEM",
                "RETARDO_FS",
                "TAM_MEM",
                "RETARDO_JOURNAL",
                "RETARDO_GOSSIPING",
                "MEMORY_NUMBER"
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
	else	{
		configuracion.puerto = config_get_int_value(archivoConfig, "PUERTO");
		configuracion.ipFileSystem = config_get_string_value(archivoConfig, "IP_FS");
		configuracion.puertoFileSystem = config_get_int_value(archivoConfig, "PUERTO_FS");
		configuracion.ipSeeds = config_get_array_value(archivoConfig, "IP_SEEDS");
		configuracion.puertoSeeds = (int*) config_get_array_value(archivoConfig, "PUERTO_SEEDS");
		configuracion.retardoMemoria = config_get_int_value(archivoConfig, "RETARDO_MEM");
		configuracion.retardoFileSystem = config_get_int_value(archivoConfig, "RETARDO_FS");
		configuracion.tamanioMemoria = config_get_int_value(archivoConfig, "TAM_MEM");
		configuracion.retardoJournal = config_get_int_value(archivoConfig, "RETARDO_JOURNAL");
		configuracion.retardoGossiping = config_get_int_value(archivoConfig, "RETARDO_GOSSIPING");
		configuracion.cantidadDeMemorias = config_get_int_value(archivoConfig, "MEMORY_NUMBER");

		return configuracion;
	}
}

void atenderMensajes(Header header, char* mensaje)    {
    printf("Estoy recibiendo un mensaje del file descriptor %i: %s", header.fdRemitente, mensaje);

    char** arrayMensaje = parser(mensaje);

    //char** arrayMensaje = parser("SELECT amigues");


    if (strcmp(arrayMensaje[0], "SELECT") == 0){
        printf("Recibi un select");
    }else if (strcmp(arrayMensaje[0], "INSERT") == 0){
        printf("Recibi un insert");
    }else if (strcmp(arrayMensaje[0], "CREATE") == 0){
        printf("Recibi un create");
    }else if (strcmp(arrayMensaje[0], "DESCRIBE") == 0){
        printf("Recibi un describe");
    }else if (strcmp(arrayMensaje[0], "DROP") == 0){
        printf("Recibi un drop");
    }else if (strcmp(arrayMensaje[0], "JOURNAL") == 0){
        printf("Recibi un journal");
    }else{
        printf("No entendi tu mensaje bro");
    }
    printf("\n");
    fflush(stdout);
}

int main(void) {
    t_log* logger = log_create("../memoria.log", "memoria", false, LOG_LEVEL_INFO);

	t_configuracion configuracion = cargarConfiguracion("../memoria.cfg", logger);


	//pthread_t* hiloConexiones = crearHiloServidor(configuracion.puerto, &atenderMensajes, NULL, NULL);
    int cliente = crearSocketCliente("127.0.0.1", 5003);
    enviarPaquete(cliente, "Hola, soy la memoria");
    Header *header = (Header*)malloc(sizeof(Header));
    recv(cliente, header, sizeof(Header), NULL);
    char* mensaje = malloc(header->tamanioMensaje);
    recv(cliente, mensaje, header->tamanioMensaje, NULL);
    printf("%s", mensaje);
    fflush(stdout);

	char* linea;
	while(1){
	    /* Imprimo lo que ingreso por pantalla
	     * queda comentado porque el crearHiloServido me bloquea el proceso*/
	    linea = readline(">");
        if (!linea) {
            break;
        }
        printf("%s\n", linea);
        free(linea);
	};

	return 0;
}
