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
    t_log *logger = log_create("../kernel.log", "kernel", true, LOG_LEVEL_INFO);
    log_info(logger, "Iniciando el proceso Kernel");

    t_configuracion configuracion = cargarConfiguracion("kernel.cfg", logger);
    //t_dictionary *tablaDeMemoriasConocidas = dictionary_create();
    //t_memoria_conocida memoriaConocida = {.fdMemoria =0};

    log_info(logger, "IP Memoria: %s", configuracion.ipMemoria);
    log_info(logger, "Puerto Memoria: %i", configuracion.puertoMemoria);
    log_info(logger, "Quantum: %i", configuracion.quantum);
    log_info(logger, "Multiprocesamiento: %i", configuracion.multiprocesamiento);
    log_info(logger, "Refresh Metadata: %i", configuracion.refreshMetadata);
    log_info(logger, "Retardo de Ejecución : %i", configuracion.refreshMetadata);

    GestorConexiones *misConexiones= inicializarConexion();

    int fdMemoria = conectarseAServidor(configuracion.ipMemoria, configuracion.puertoMemoria, misConexiones, logger);
    //todo probamos con una sola memoria por ahora
    //conectarseAMemoriaPrincipal(&memoriaConocida, configuracion.ipMemoria, configuracion.puertoMemoria, logger);
    //memoriasConectadas(fdMemoria);
    pthread_t *hiloRespuestas = crearHiloConexiones(misConexiones, logger);

    ejecutarConsola(gestionarRequest, logger, fdMemoria);

    pthread_join(&hiloRespuestas, NULL);
    //ejecutarConsola(gestionarRequest, logger, tablaDeMemoriasConocidas);

    return EXIT_SUCCESS;
}

t_configuracion cargarConfiguracion(char *pathArchivoConfiguracion, t_log *logger) {
    t_configuracion configuracion;

    t_config *archivoConfig = abrirArchivoConfiguracion(pathArchivoConfiguracion,
                                                        logger); //nos devuelve un archivoConfig

    bool existenTodasLasClavesObligatorias(t_config *archivoConfig, t_configuracion configuracion) {
        char *clavesObligatorias[6] = {
                "IP_MEMORIA",
                "PUERTO_MEMORIA",
                "QUANTUM",
                "MULTIPROCESAMIENTO",
                "METADATA_REFRESH",
                "SLEEP_EJECUCION"
        };

        for (int i = 0; i < COUNT_OF(clavesObligatorias); i++) {
            if (!config_has_property(archivoConfig, clavesObligatorias[i]))
                return false;
        }
        return true;
    }

    if (!existenTodasLasClavesObligatorias(archivoConfig, configuracion)) {
        log_error(logger, "Alguna de las claves obligatorias no están setteadas en el archivo de configuración.");
        config_destroy(archivoConfig);
        exit(1); // settear algún código de error para cuando falte alguna key
    } else {
        char *ipMemoria = config_get_string_value(archivoConfig, "IP_MEMORIA");
        configuracion.ipMemoria = (char *) malloc(sizeof(char) * strlen(ipMemoria));
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

void ejecutarConsola(int (*gestionarRequest)(t_comando, int), t_log *logger, int fdMemoria) {
    t_comando requestParseada;
    do {
        char *leido = readline("Kernel@suck-ets:~$ ");
        char **comandoParseado = parser(leido);
        if (comandoParseado == NULL) { //acordarse de tener en cuenta el " "
            free(comandoParseado);
            continue;
        }
        requestParseada = instanciarComando(comandoParseado);
        analizarRequest(requestParseada, logger, fdMemoria);
        //free(leido);
        //free(comandoParseado);
    } while (requestParseada.tipoRequest != EXIT);
    printf("Ya analizamos todo lo solicitado.\n");
}

void *analizarRequest(t_comando requestParseada, t_log *logger, int fdMemoria) {
    if (requestParseada.tipoRequest == INVALIDO) {
        printf("Comando inválido.\n");
    } else {
        if (validarComandosComunes(requestParseada, logger) || validarComandosKernel(requestParseada, logger)) {
            if (gestionarRequest(requestParseada, fdMemoria) == 0) {
                log_info(logger, "Request procesada correctamente.");
            } else {
                log_error(logger, "No se pudo procesar la request solicitada.");
            }
        }
    }
}

int gestionarRequest(t_comando requestParseada, int fdMemoria) {
    switch (requestParseada.tipoRequest) {
        case SELECT:
            return gestionarSelectKernel(requestParseada.parametros[0], requestParseada.parametros[1], fdMemoria);
        case INSERT:
            return gestionarInsertKernel(requestParseada.parametros[0], requestParseada.parametros[1], requestParseada.parametros[2],
                                         fdMemoria);
        case CREATE:
            return gestionarCreateKernel(requestParseada.parametros[0], requestParseada.parametros[1], requestParseada.parametros[2],
                                         requestParseada.parametros[3], fdMemoria);
        case DROP:
            return gestionarDropKernel(requestParseada.parametros[0], fdMemoria);
        case DESCRIBE:
            return 0;//gestionarDescribeKernel();
        case JOURNAL:
            return 0;//gestionarJournalKernel();
        case ADD:
            //printf("Tipo de Request: % %s %s ", requestParseada.tipoRequest, requestParseada.parametros[1],
            //       requestParseada.parametros[2]); //nombreTabla en realidad vien
            //printf("To: %s", requestParseada.parametros[3]);
            //gestionarAdd(requestParseada.parametros, tablaDeMemoriasConocidas);
            return 0;
        case RUN:
            gestionarRun(requestParseada.parametros[0], fdMemoria);
            break;
        case METRICS:
            //gestionarMetricas();
            break;
        case HELP:
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
        case EXIT:
            return 0;
        default:
            return printf("Comando inválido.\n");
    }
}

/*int gestionarComando(char **requestParseada) {
    char *tipoDeRequest = request[0];
    char *nombreTabla = request[1];
    char *param1 = request[2];
    char *param2 = request[3];
    char *param3 = request[4];
    if (validarComandosKernel(tipoDeRequest, nombreTabla, param1, param2, param3) == 1) {
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
            printf("Tipo de Request: %s %s %s ", tipoDeRequest, nombreTabla, param1); //nombreTabla en realidad vien
            printf("To: %s", param3);
            return 0;

        } else if (strcmp(tipoDeRequest, "RUN") == 0) {
            printf("Tipo de Request: %s\n", tipoDeRequest); //nombreTabla en realidad vien
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
}*/

//para hacer los comandosKernel

bool validarComandosKernel(t_comando comando, t_log *logger) {
    bool esValido = esComandoValidoDeKernel(comando);

    if (!esValido)
        log_warning(logger, "Alguno de los parámetros ingresados es incorrecto. Por favor verifique su entrada.");

    return esValido;
}

bool esComandoValidoDeKernel(t_comando comando) {
    switch (comando.tipoRequest) {
        case ADD:
            return (esString(comando.parametros[0]) && esString(comando.parametros[2]) &&
                    esString(comando.parametros[3]) && esEntero(comando.parametros[1])) &&
                   (comando.cantidadParametros == 4);
        case RUN:
            return (esString(comando.parametros[0]) && comando.cantidadParametros == 1);
        case METRICS:
            return (comando.cantidadParametros == 0);
    }
}

int gestionarRun(char *pathArchivo, int fdMemoria) {
    t_archivoLQL archivoLQL;
    t_log *logger;
    char *linea = NULL;
    int lineaLeida = 0;
    int caracter, contador;

    FILE *archivo = fopen(pathArchivo, "r");
    caracter = fgetc(archivo);

    if (!sePuedeLeerElArchivo(pathArchivo)) {
        log_error(logger, "El PATH recibido es inválido.\n");
    }
    while (!feof(archivo)) {
        linea = (char *) realloc(NULL, sizeof(char));
        contador = 0;
        while (caracter != '\n') {
            linea[contador] = caracter;
            contador++;
            linea = (char *) realloc(linea, (contador + 1) * sizeof(char));
            caracter = fgetc(archivo);
        }
        linea = (char *) realloc(linea, (contador + 2) * sizeof(char)); //agrego el \n por las dudas
        linea[contador] = caracter;
        //Asignamos la linea al primer elemento de la lista de request
        archivoLQL.listaDeRequests[lineaLeida] = linea;
        lineaLeida++;
        archivoLQL.cantidadDeLineas++;
    }
    administrarRequestsLQL(archivoLQL, logger, fdMemoria);
    //ver si no conviene dejar aparte cuando conviene enviar a una determinada memoria
    //asignarAMemoriaCorrecta(request, tablaDeMemorias);
    fclose(archivo);
}

void *administrarRequestsLQL(t_archivoLQL archivoLQL, t_log *logger, int fdMemoria) {
    t_comando requestParseada;
    int contadorDeRequest = 0;
    do {
        char *unaRequest = archivoLQL.listaDeRequests[contadorDeRequest]; //primer elemento de la lista de request, es una request
        if (strcmp(unaRequest, " ") != 0) {
            char **comandoParseado = parser(unaRequest); //cada request, es un conjunto de palabras
            requestParseada = instanciarComando(comandoParseado);
            free(comandoParseado);
            free(unaRequest);
            free(comandoParseado);

            analizarRequest(requestParseada, logger, fdMemoria);
        }
    } while (contadorDeRequest > archivoLQL.cantidadDeLineas);
    printf("Ya analizamos todo lo solicitado para el archivo LQL ingresado.\n");
}

int gestionarSelectKernel(char *nombreTabla, char *key, int fdMemoria) {
    //acá tengo que sacar el fd de alguna memoria que voy a tener en mi tabla de memorias, que esté libre y que tenga el mismo tipo de Consistencia
    char *request = string_from_format("SELECT %s %s", nombreTabla, key);
    enviarPaquete(fdMemoria, REQUEST, request);
    free(request);
    //recibo mensaje de Memoria o directamente fallo yo?
    return 0;
}

int gestionarCreateKernel(char *nombreTabla, char *tipoConsistencia, char *cantidadParticiones, char *tiempoCompactacion, int fdMemoria) {
    //acá tengo que sacar el fd de alguna memoria que voy a tener en mi tabla de memorias, que esté libre y que tenga el mismo tipo de Consistencia
    char *request = string_from_format("CREATE %s %s %s %s", nombreTabla, tipoConsistencia, cantidadParticiones, tiempoCompactacion);
    enviarPaquete(fdMemoria, REQUEST, request);
    free(request);
    //recibo mensaje de Memoria o directamente fallo yo?
    return 0;
}

int gestionarInsertKernel(char *nombreTabla, char *key, char *valor, int fdMemoria) {
    fdMemoria = 0; //acá tengo que sacar el fd de alguna memoria que voy a tener en mi tabla de memorias, que esté libre y que tenga el mismo tipo de Consistencia
    char *request = string_from_format("INSERT %s %s %s", nombreTabla, key, valor);
    enviarPaquete(fdMemoria, REQUEST, request);
    free(request);
    //recibo mensaje de Memoria o directamente fallo yo?
    return 0;
}

int gestionarDropKernel(char *nombreTabla, int fdMemoria) {
    fdMemoria = 0; //acá tengo que sacar el fd de alguna memoria que voy a tener en mi tabla de memorias, que esté libre y que tenga el mismo tipo de Consistencia
    char *request = string_from_format("DROP %s %s", nombreTabla);
    enviarPaquete(fdMemoria, REQUEST, request);
    free(request);
    //recibo mensaje de Memoria o directamente fallo yo?
    return 0;
}

/*int administrarAdd(char **parametrosNecesarios, t_dictionary tablaDeMemoriasConocidas) {
    char *palabraMemory = parametrosNecesarios[0];
    char *numeroMemoria = parametrosNecesarios[1];
    char *palabraTo = parametrosNecesarios[2];
    char *consistencia = parametrosNecesarios[3];

    return fdMemoria;
}

int gestionarAdd(int fdMemoria)

*/