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

    parametros_consola* parametros = (parametros_consola*) malloc(sizeof(parametros_consola));

    parametros->logger = logger;
    parametros->unComponente = KERNEL;
    parametros->gestionarComando = gestionarComando;

    ejecutarConsola(parametros);

	/*while(1)	{
    enviarPaquete(fdMemoria, REQUEST, "DESCRIBE TABLE 2\n");
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
    if (validarComandosKernel(tipoDeRequest, nombreTabla, param1, param2, param3)== 1){
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
        } else if (strcmp(tipoDeRequest, "HELP") == 0) {
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
            printf("Ingrese un comando válido.\n");
            return -2;
        }
    }
}

int validarComandosKernel(char* tipoDeRequest, char* nombreTabla, char* param1, char* param2, char* param3){
    char * palabraMemory = nombreTabla;
    if(palabraMemory != NULL) string_to_upper(palabraMemory);//palabra "MEMORY" necesaria para hacer el ADD o "PATH" para el comando RUN
    char * palabraTO = param2;
    //el TO necesario para hacer el ADD
    char * criterio = param3;
    if(criterio == "sc" || criterio == "shc" || criterio == "ec") string_to_upper(criterio);//tipoDeConsistencia
    if (strcmp(tipoDeRequest, "ADD") == 0) {
        if(palabraMemory != "MEMORY" || param1 == NULL || palabraTO != "TO" ||
            (criterio != "SC" || criterio != "SHC" || criterio != "EC")){
            imprimirErrorParametros();
            return 0;
        }
    } else if (strcmp(tipoDeRequest, "RUN") == 0) {
        char * path = palabraMemory;
        if(path == NULL){
            printf("El PATH recibido es inválido.\n");
            return 0;
        }
    } else if (strcmp(tipoDeRequest, "JOURNAL") == 0) {
        if (palabraMemory != NULL || param1 != NULL || palabraTO != NULL || criterio != NULL) {
            printf("Los parámetros son innecesarios.\n");
            return 0;
        }
    } else if (strcmp(tipoDeRequest, "METRICS") == 0) {
        if(palabraMemory != NULL || param1 != NULL || palabraTO != NULL || criterio != NULL){
            printf("Los parámetros son innecesarios.\n");
            return 0;
        }
    }
}