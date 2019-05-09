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
    log_info(logger, "Iniciando el proceso Kernel");

	configuracion = cargarConfiguracion("kernel.cfg", logger);

    log_info(logger, "IP Memoria: %s", configuracion.ipMemoria);
    log_info(logger, "Puerto Memoria: %i", configuracion.puertoMemoria);
    log_info(logger, "Quantum: %i", configuracion.quantum);
    log_info(logger, "Multiprocesamiento: %i", configuracion.multiprocesamiento);
    log_info(logger, "Refresh Metadata: %i", configuracion.refreshMetadata);
    log_info(logger, "Retardo de Ejecución : %i", configuracion.refreshMetadata);

    GestorConexiones* conexion = inicializarConexion();
	//conectar con memoria y luego el paso de abajo
	//int fdDestinatario = crearSocketCliente(configuracion.ipMemoria, configuracion.puertoMemoria);
    int fdMemoria = conectarseAServidor(configuracion.ipMemoria, configuracion.puertoMemoria, conexion, logger);

    pthread_t* hiloRespuestas = crearHiloConexiones(conexion, logger);

    ejecutarConsola(&gestionarComando,"kernel",logger);

	while(1)	{
    enviarPaquete(fdMemoria, "DESCRIBE TABLE 2\n");
    sleep(3);}

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
        config_destroy(archivoConfig);
		exit(1); // settear algún código de error para cuando falte alguna key
	}	else	{
	    char* ipMemoria = config_get_string_value(archivoConfig, "IP_MEMORIA");
	    configuracion.ipMemoria = (char*) malloc(sizeof(char) * strlen(ipMemoria));
	    strcpy(configuracion.ipMemoria, ipMemoria);
		configuracion.puertoMemoria = config_get_int_value(archivoConfig, "PUERTO_MEMORIA");
		configuracion.quantum = config_get_int_value(archivoConfig, "QUANTUM");
		configuracion.multiprocesamiento = config_get_int_value(archivoConfig, "MULTIPROCESAMIENTO");
		configuracion.refreshMetadata = config_get_int_value(archivoConfig, "METADATA_REFRESH");
		configuracion.retardoEjecucion = config_get_int_value(archivoConfig, "SLEEP_EJECUCION");

        config_destroy(archivoConfig);

		return configuracion;
	}
}

int gestionarComando(char **request) {
    char *tipoDeRequest = request[0];
    char *nombreTabla = request[1];
    char *param1 = request[2];
    char *param2 = request[3];
    char *param3 = request[4];
    string_to_upper(tipoDeRequest);

    if (strcmp(tipoDeRequest, "SELECT") == 0) {
        printf("Tipo de Request: %s\n", tipoDeRequest);
        printf("Tabla: %s\n", nombreTabla);
        printf("Key: %s\n", param1);
        //kernelSelect(nombreTabla, param1);
        return 0;

    } else if (strcmp(tipoDeRequest, "INSERT") == 0) {
        printf("Tipo de Request: %s\n", tipoDeRequest);
        printf("Tabla: %s\n", nombreTabla);
        printf("Key: %s\n", param1);
        printf("Valor: %s\n", param2);
        time_t timestamp;
        if (param3 != NULL) {
            timestamp = (time_t) strtol(param3, NULL, 10);
        } else {
            timestamp = (time_t) time(NULL);
        }
        printf("Timestamp: %i\n", (int) timestamp);
        //kernelInsert(nombreTabla, param1, param2, timestamp);
        return 0;

    } else if (strcmp(tipoDeRequest, "CREATE") == 0) {
        printf("Tipo de Request: %s\n", tipoDeRequest);
        printf("Tabla: %s\n", nombreTabla);
        printf("TIpo de consistencia: %s\n", param1);
        printf("Numero de particiones: %s\n", param2);
        printf("Tiempo de compactacion: %s\n", param3);
        //kernelCreate(nombreTabla, param1, param2, param3)
        return 0;

    } else if (strcmp(tipoDeRequest, "DESCRIBE") == 0) {
        printf("Tipo de Request: %s\n", tipoDeRequest);
        if (nombreTabla == NULL) {
            // Hacer describe global
        } else {
            printf("Tabla: %s\n", nombreTabla);
            // Hacer describe de una tabla especifica
        }
        return 0;

    } else if (strcmp(tipoDeRequest, "DROP") == 0) {
        printf("Tipo de Request: %s\n", tipoDeRequest);
        printf("Tabla: %s\n", nombreTabla);
        //kernelDrop(nombreTabla);
        return 0;

    } else if (strcmp(tipoDeRequest, "ADD") == 0) {
        printf("Tipo de Request: %s %s %s", tipoDeRequest, nombreTabla, param1); //nombreTabla en realidad vien
        printf("To: %s", param3);
        return 0;

    } else if (strcmp(tipoDeRequest, "RUN") == 0) {
        printf("Tipo de Request: %s", tipoDeRequest); //nombreTabla en realidad vien
        printf("Path: %s\n", nombreTabla);
        return 0;
    }
    else if (strcmp(tipoDeRequest, "HELP") == 0) {
        printf("************ Comandos disponibles ************\n");
        printf("- SELECT [NOMBRE_TABLA] [KEY]\n");
        printf("- INSERT [NOMBRE_TABLA] [KEY] “[VALUE]” [Timestamp](Opcional)\n");
        printf("- CREATE [NOMBRE_TABLA] [TIPO_CONSISTENCIA] [NUMERO_PARTICIONES] [COMPACTION_TIME]\n");
        printf("- DESCRIBE [NOMBRE_TABLA](Opcional)\n");
        printf("- DROP [NOMBRE_TABLA]\n");
        printf("- ADD MEMORY [NUMERO_MEMORIA] TO [TIPO_CONSISTENCIA]\n");
        printf("- RUN [PATH]\n");
        printf("- JOURNAL\n");
        printf("- METRICS\n");
        printf("- EXIT\n");
        return 0;

    } else {
        printf("Ingrese un comando válido.");
        return -2;
    }
}

pthread_t* crearHiloConexiones(GestorConexiones* unaConexion, t_log* logger)    {
    pthread_t* hiloConexiones = malloc(sizeof(pthread_t));

    parametros_thread* parametros = (parametros_thread*) malloc(sizeof(parametros_thread));

    parametros->conexion = unaConexion;
    parametros->logger = logger;

    pthread_create(hiloConexiones, NULL, &atenderConexiones, parametros);

    return hiloConexiones;
}

void* atenderConexiones(void* parametrosThread)    {
    parametros_thread* parametros = (parametros_thread*) parametrosThread;
    GestorConexiones* unaConexion = parametros->conexion;
    t_log* logger = parametros->logger;

    fd_set emisores;

    while(1)    {
        if(hayNuevoMensaje(unaConexion, &emisores))    {
            // voy a recorrer todos los FD a los cuales estoy conectado para saber cuál de todos es el que tiene un nuevo mensaje
            for(int i=0; i < list_size(unaConexion->conexiones); i++)   {
                Header headerSerializado;
                int fdConectado = *((int*) list_get(unaConexion->conexiones, i));

                if(FD_ISSET(fdConectado, &emisores))    {
                    int bytesRecibidos = recv(fdConectado, &headerSerializado, sizeof(Header), MSG_DONTWAIT);

                    switch(bytesRecibidos)  {
                        // hubo un error al recibir los datos
                        case -1:
                            log_warning(logger, "Hubo un error al recibir el header proveniente del socket %i", fdConectado);
                            break;
                            // se desconectó
                        case 0:
                            // acá cada uno setea una maravillosa función que hace cada uno cuando se le desconecta alguien
                            // nombre_maravillosa_funcion();
                            desconectarCliente(fdConectado, unaConexion, logger);
                            break;
                            // recibí algún mensaje
                        default: ; // esto es lo más raro que vi pero tuve que hacerlo
                            Header header = deserializarHeader(&headerSerializado);
                            header.fdRemitente = fdConectado;
                            int pesoMensaje = header.tamanioMensaje * sizeof(char);
                            char* mensaje = (char*) malloc(pesoMensaje);
                            bytesRecibidos = recv(fdConectado, mensaje, pesoMensaje, MSG_DONTWAIT);
                            if(bytesRecibidos == -1 || bytesRecibidos < pesoMensaje)
                                log_warning(logger, "Hubo un error al recibir el mensaje proveniente del socket %i", fdConectado);
                            else if(bytesRecibidos == 0)	{
                                // acá cada uno setea una maravillosa función que hace cada uno cuando se le desconecta alguien
                                // nombre_maravillosa_funcion();
                                desconectarCliente(fdConectado, unaConexion, logger);
                            }
                            else	{
                                // acá cada uno setea una maravillosa función que hace cada uno cuando le llega un nuevo mensaje
                                // nombre_maravillosa_funcion();
                                int tamanioMensaje = strlen(mensaje);
                                atenderMensajes(header, mensaje);
                            }
                            break;
                    }
                }
            }

            // me fijo si hay algún nuevo conectado
            if(FD_ISSET(unaConexion->servidor, &emisores))	{
                int* fdNuevoCliente = malloc(sizeof(int));
                *fdNuevoCliente = aceptarCliente(unaConexion->servidor, logger);
                list_add(unaConexion->conexiones, fdNuevoCliente);

                //me fijo si hay que actualizar el file descriptor máximo con el del nuevo cliente
                unaConexion->descriptorMaximo = getFdMaximo(unaConexion);

                // acá cada uno setea una maravillosa función que hace cada uno cuando se le conecta un nuevo cliente
                // nombre_maravillosa_funcion();
            }
        }
    }
}

void atenderMensajes(Header header, char* mensaje)    {
    printf("Estoy recibiendo un mensaje del file descriptor %i: %s", header.fdRemitente, mensaje);
    fflush(stdout);

//    char** arrayMensaje = parser(mensaje);
//
//    //char** arrayMensaje = parser("SELECT");
//
//
//    if (strcmp(arrayMensaje[0], "SELECT") == 0){
//        printf("Recibi un select");
//
//        //Todo chequear que las queries traigan la cantidad correcta de parámetros
//        //TODO crear un segmento y una página
//        //TODO buscar dentro del segmento lo que se pidió ELSE
//
//        enviarPaquete(FD_FS, mensaje);
//        Header *header = (Header*)malloc(sizeof(Header));
//        recv(FD_CLIENTE, header, sizeof(Header), NULL);
//        char* respuesta = malloc(header->tamanioMensaje);
//        recv(FD_CLIENTE, respuesta, header->tamanioMensaje, NULL);
//        printf("%s", mensaje);
//
//    }else if (strcmp(arrayMensaje[0], "INSERT") == 0){
//        printf("Recibi un insert");
//    }else if (strcmp(arrayMensaje[0], "CREATE") == 0){
//        printf("Recibi un create");
//    }else if (strcmp(arrayMensaje[0], "DESCRIBE") == 0){
//        printf("Recibi un describe");
//    }else if (strcmp(arrayMensaje[0], "DROP") == 0){
//        printf("Recibi un drop");
//    }else if (strcmp(arrayMensaje[0], "JOURNAL") == 0){
//        printf("Recibi un journal");
//    }else{
//        printf("No entendi tu mensaje bro");
//    }
//    printf("\n");
//    fflush(stdout);
}