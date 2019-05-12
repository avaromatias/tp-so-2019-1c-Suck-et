/*
 ============================================================================
 Name        : Memoria.c
 Author      : Suck et
 	 UTN FRBA - Sistemas Operativos
 	 	 TP-1C-2019-Lissandra
 ============================================================================
 */

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
        config_destroy(archivoConfig);
		exit(1); // settear algún código de error para cuando falte alguna key
	}
	else	{
		configuracion.puerto = config_get_int_value(archivoConfig, "PUERTO");
		char* ipFileSystem = config_get_string_value(archivoConfig, "IP_FS");
		configuracion.ipFileSystem = (char*) malloc(pesoString("255.255.255.255"));
        strcpy(configuracion.ipFileSystem, ipFileSystem);
		configuracion.puertoFileSystem = config_get_int_value(archivoConfig, "PUERTO_FS");
		configuracion.ipSeeds = config_get_array_value(archivoConfig, "IP_SEEDS");
		configuracion.puertoSeeds = (int*) config_get_array_value(archivoConfig, "PUERTO_SEEDS");
		configuracion.retardoMemoria = config_get_int_value(archivoConfig, "RETARDO_MEM");
		configuracion.retardoFileSystem = config_get_int_value(archivoConfig, "RETARDO_FS");
		configuracion.tamanioMemoria = config_get_int_value(archivoConfig, "TAM_MEM");
		configuracion.retardoJournal = config_get_int_value(archivoConfig, "RETARDO_JOURNAL");
		configuracion.retardoGossiping = config_get_int_value(archivoConfig, "RETARDO_GOSSIPING");
		configuracion.cantidadDeMemorias = config_get_int_value(archivoConfig, "MEMORY_NUMBER");

        config_destroy(archivoConfig);

		return configuracion;
	}
}

//void startUp(){
//    FD_FS = crearSocketCliente(configuracion.ipFileSystem, configuracion.puertoFileSystem);
//    //TODO Pedirle el TAMAÑO_VALUE al FS
//    /*enviarPaquete(FD_FS, "DAME EL TAM_VALUE");
//    Header *header = (Header*)malloc(sizeof(Header));
//    recv(cliente, header, sizeof(Header), NULL);
//    char* mensaje = malloc(header->tamanioMensaje);
//    recv(cliente, mensaje, header->tamanioMensaje, NULL);
//    printf("%s", mensaje);*/
//
//    printf("Reservo bloque de memoria ");
//
//    memoria.direcciones = (void*) malloc(configuracion.tamanioMemoria);
//    memoria.tamanioMemoria = configuracion.tamanioMemoria;
//
//}

int main(void) {
    t_log* logger = log_create("memoria.log", "memoria", true, LOG_LEVEL_INFO);

	t_configuracion configuracion = cargarConfiguracion("memoria.cfg", logger);

	GestorConexiones* misConexiones = inicializarConexion();

    levantarServidor(configuracion.puerto, misConexiones, logger);

	int fdKernel = 0;

    sem_t kernelConectado;
    sem_t lissandraConectada;

	sem_init(&kernelConectado, 0, 0);
	sem_init(&lissandraConectada, 0 , 0);

    int fdLissandra = crearSocketCliente(configuracion.ipFileSystem, configuracion.puertoFileSystem, logger);
    if(fdLissandra > 0)
    	sem_post(&lissandraConectada);

    pthread_t* hiloConexiones = crearHiloConexiones(misConexiones, &fdKernel, &kernelConectado, logger);

    while(1)	{
		sem_wait(&kernelConectado);
		sem_wait(&lissandraConectada);
		if(fdKernel > 0)	{
			char* request = recibirMensaje(&fdKernel);
			if(request == NULL) {
				continue;
			}
			else    {
				sem_post(&kernelConectado);
				enviarPaquete(fdLissandra, REQUEST, request);
                char* respuestaLissandra = recibirMensaje(&fdLissandra);
                if(respuestaLissandra == NULL)	{
                	continue;
                }
                else	{
                	if(fdKernel > 0)
                		enviarPaquete(fdKernel, REQUEST, respuestaLissandra);
                }
			}
		}
    }


//	char* linea;
//	while(1){
//	    /* Imprimo lo que ingreso por pantalla
//	     * queda comentado porque el crearHiloServido me bloquea el proceso*/
//	    linea = readline(">");
//        if (!linea) {
//            break;
//        }
//        printf("%s\n", linea);
//        free(linea);
//	};
    pthread_join(*hiloConexiones, NULL);

	return 0;
}
