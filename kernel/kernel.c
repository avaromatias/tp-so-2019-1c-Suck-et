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

    log_info(logger, "IP Memoria: %s", configuracion.ipMemoria);
    log_info(logger, "Puerto Memoria: %i", configuracion.puertoMemoria);
    log_info(logger, "Quantum: %i", configuracion.quantum);
    log_info(logger, "Multiprocesamiento: %i", configuracion.multiprocesamiento);
    log_info(logger, "Refresh Metadata: %i", configuracion.refreshMetadata);
    log_info(logger, "Retardo de Ejecución : %i", configuracion.refreshMetadata);

    t_dictionary *metadataTablas = dictionary_create(); //voy a tener el nombreTabla, criterio, particiones y tpo_Compactación
    t_dictionary *tablaDeMemoriasConCriterios = dictionary_create(); //voy a tener relacion de criterio con un t_list* de FDs

    t_queue *colaDeNew = queue_create();
    t_queue *colaDeReady = queue_create();
    t_queue *colaDeExecute = queue_create();
    t_list *finalizados = list_create();

    GestorConexiones *misConexiones = inicializarConexion();
    conectarseAMemoriaPrincipal(configuracion.ipMemoria, configuracion.puertoMemoria, misConexiones, logger);

    pthread_t *hiloRespuestas = crearHiloConexiones(misConexiones, logger);

    p_consola_kernel *parametros = (p_consola_kernel *) malloc(sizeof(p_consola_kernel));

    parametros->logger = logger;
    parametros->conexiones = misConexiones;
    parametros->metadataTablas = metadataTablas;
    parametros->memoriasConCriterios = tablaDeMemoriasConCriterios;

    ejecutarConsola(gestionarRequest, parametros);

    //pthread_join(&hiloRespuestas, NULL);
    free(parametros);
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
        configuracion.ipMemoria = string_duplicate(config_get_string_value(archivoConfig, "IP_MEMORIA"));
        configuracion.puertoMemoria = config_get_int_value(archivoConfig, "PUERTO_MEMORIA");
        configuracion.quantum = config_get_int_value(archivoConfig, "QUANTUM");
        configuracion.multiprocesamiento = config_get_int_value(archivoConfig, "MULTIPROCESAMIENTO");
        configuracion.refreshMetadata = config_get_int_value(archivoConfig, "METADATA_REFRESH");
        configuracion.retardoEjecucion = config_get_int_value(archivoConfig, "SLEEP_EJECUCION");

        config_destroy(archivoConfig);

        return configuracion;
    }
}

void ejecutarConsola(int (*gestionarRequest)(t_comando, p_consola_kernel *), p_consola_kernel *parametros) {
    t_comando requestParseada;

    do {
        char *leido = readline("Kernel@suck-ets:~$ ");
        char **comandoParseado = parser(leido);
        if (comandoParseado == NULL) {
            free(comandoParseado);
            continue;
        }
        requestParseada = instanciarComando(comandoParseado);
        analizarRequest(requestParseada, parametros);
        //free(leido);
        //free(comandoParseado);
    } while (requestParseada.tipoRequest != EXIT);
    printf("Ya analizamos todo lo solicitado.\n");
}

void *analizarRequest(t_comando requestParseada, p_consola_kernel *parametros) {

    t_log *logger = parametros->logger;

    if (requestParseada.tipoRequest == INVALIDO) {
        printf("Comando inválido.\n");
    } else {
        if (validarComandosComunes(requestParseada, logger) || validarComandosKernel(requestParseada, logger)) {
            if (gestionarRequest(requestParseada, parametros) == 0) {
                log_info(logger, "Request procesada correctamente.");
            } else {
                log_error(logger, "No se pudo procesar la request solicitada.");
            }
        }
    }
}

int gestionarRequest(t_comando requestParseada, p_consola_kernel *parametros) {

    GestorConexiones *misConexiones = parametros->conexiones;
    char *criterioConsistencia = criterioBuscado(requestParseada, parametros->metadataTablas);
    //int fdMemoria = 1;// misConexiones->conexiones[0];
    int fdMemoria = seleccionarMemoriaIndicada(parametros, criterioConsistencia);

    switch (requestParseada.tipoRequest) { //Analizar si cada gestionar va a tener que encolar en NEW, en lugar de enviarPaquete
        case SELECT:
            return gestionarSelectKernel(requestParseada.parametros[0], requestParseada.parametros[1], fdMemoria);
        case INSERT:
            return gestionarInsertKernel(requestParseada.parametros[0], requestParseada.parametros[1],
                                         requestParseada.parametros[2],
                                         fdMemoria);
        case CREATE:
            return gestionarCreateKernel(requestParseada.parametros[0], requestParseada.parametros[1],
                                         requestParseada.parametros[2],
                                         requestParseada.parametros[3], fdMemoria);
        case DROP:
            return gestionarDropKernel(requestParseada.parametros[0], fdMemoria);
        case DESCRIBE:
            return 0;//gestionarDescribeKernel();
        case JOURNAL:
            return 0;//gestionarJournalKernel();
        case ADD:
            printf("Tipo de Request: %s %s %s ", "ADD",
                   requestParseada.parametros[1], // no pongo requestParseada.tipoRequest porque va a romper
                   requestParseada.parametros[2]); //nombreTabla en realidad vien
            printf("To: %s", requestParseada.parametros[3]);
            gestionarAdd(requestParseada.parametros, parametros);
            return 0;
        case RUN:
            gestionarRun(requestParseada.parametros[0], parametros);
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

int gestionarRun(char *pathArchivo, p_consola_kernel *parametros) {
    t_archivoLQL archivoLQL;
    t_log *logger = parametros->logger;
    GestorConexiones *misConexiones = parametros->conexiones;
    //t_list *tablaMemoriasConocidas = parametros->memoriasConocidas;

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
    administrarRequestsLQL(archivoLQL, parametros);
    //ver si no conviene dejar aparte cuando conviene enviar a una determinada memoria
    //asignarAMemoriaCorrecta(request, tablaDeMemorias);
    fclose(archivo);
}

void *administrarRequestsLQL(t_archivoLQL archivoLQL, p_consola_kernel *parametros) {
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

            analizarRequest(requestParseada, parametros);
        }
    } while (contadorDeRequest > archivoLQL.cantidadDeLineas);
    printf("Ya analizamos todo lo solicitado para el archivo LQL ingresado.\n");
}

int gestionarSelectKernel(char *nombreTabla, char *key, int fdMemoria) {
    //acá tengo que sacar el fd de alguna memoria que voy a tener en mi tabla de memorias, que esté libre y que tenga el mismo tipo de Consistencia
    char *request = string_from_format("SELECT %s %s", nombreTabla, key);
    //enviarPaquete(fdMemoria, REQUEST, request);
    free(request);
    return 0;
}

int
gestionarCreateKernel(char *nombreTabla, char *tipoConsistencia, char *cantidadParticiones, char *tiempoCompactacion,
                      int fdMemoria) {
    //acá tengo que sacar el fd de alguna memoria que voy a tener en mi tabla de memorias, que esté libre y que tenga el mismo tipo de Consistencia
    char *request = string_from_format("CREATE %s %s %s %s", nombreTabla, tipoConsistencia, cantidadParticiones,
                                       tiempoCompactacion);
    enviarPaquete(fdMemoria, REQUEST, request);
    free(request);
    //recibo mensaje de Memoria o directamente fallo yo?
    return 0;
}

int gestionarInsertKernel(char *nombreTabla, char *key, char *valor, int fdMemoria) {
    //acá tengo que sacar el fd de alguna memoria que voy a tener en mi tabla de memorias, que esté libre y que tenga el mismo tipo de Consistencia
    char *request = string_from_format("INSERT %s %s %s", nombreTabla, key, valor);
    //enviarPaquete(fdMemoria, REQUEST, request);
    free(request);
    //recibo mensaje de Memoria o directamente fallo yo?
    return 0;
}

int gestionarDropKernel(char *nombreTabla, int fdMemoria) {
    //acá tengo que sacar el fd de alguna memoria que voy a tener en mi tabla de memorias, que esté libre y que tenga el mismo tipo de Consistencia
    char *request = string_from_format("DROP %s %s", nombreTabla);
    //enviarPaquete(fdMemoria, REQUEST, request);
    free(request);
    //recibo mensaje de Memoria o directamente fallo yo?
    return 0;
}

int gestionarAdd(char **parametrosDeRequest, p_consola_kernel *parametros) {

    t_log *logger = parametros->logger;
    int numeroMemoria = (int) parametrosDeRequest[1];
    char *criterio = parametrosDeRequest[3];
    GestorConexiones *misConexiones = parametros->conexiones;
    t_dictionary *tablaMemoriasConCriterios = parametros->memoriasConCriterios;

    bool memoriaEncontrada = (list_size(misConexiones->conexiones) >= numeroMemoria);

    if (memoriaEncontrada) {
        int *fdMemoriaSolicitada = (int *) list_get(misConexiones->conexiones,
                                                    numeroMemoria - 1); // hacemos -1 por la ubicación 0
        t_queue *colaFileDescriptors = dictionary_get(tablaMemoriasConCriterios, criterio);
        queue_push(colaFileDescriptors, fdMemoriaSolicitada);
        log_info(logger, "La memoria ha sido agregada a la tabla de Memorias conocidas.\n");
        return 0;
    } else {
        log_warning(logger, "La memoria solicitada no se encuentra dentro de las memorias conocidas.\n");
        return -1;
    }
}

int seleccionarMemoriaIndicada(p_consola_kernel *parametros, char *criterio) {
    if (existenMemoriasConectadas) {
        t_queue *memoriasDelCriterioPedido = dictionary_get(parametros->memoriasConCriterios, criterio);
        //Que memorias tengo con el criterio X de la request solicitada?
        int fdMemoriaElegida = (int)queue_pop(memoriasDelCriterioPedido);
        return fdMemoriaElegida;
    }
    log_warning(parametros->logger, "No existen memorias conectadas para asignar requests.\n");
    return 0;
}

bool existenMemoriasConectadas(GestorConexiones *misConexiones) {
    bool hayMemorias = (list_size(misConexiones->conexiones) > 0);

    return hayMemorias;
}

char *criterioBuscado(t_comando requestParseada,
                      t_dictionary *metadataTablas) { //buscaremos el criterio de cada uno de las request ingresadas
    char *criterioPedido;
    char *tabla;
    switch (requestParseada.tipoRequest) { //Cada case va a tener que buscar en el diccionario la tabla, para obtener el criterio
        case SELECT:
            tabla = requestParseada.parametros[0];
            criterioPedido = dictionary_get(metadataTablas, tabla);//buscar en metadataTablasConocidas
            return criterioPedido;
        case INSERT:
            tabla = requestParseada.parametros[0];
            criterioPedido = dictionary_get(metadataTablas, tabla);
            return criterioPedido;
        case CREATE:
            criterioPedido = requestParseada.parametros[1];
            return criterioPedido;
        case DROP:
            tabla = requestParseada.parametros[0];
            criterioPedido = dictionary_get(metadataTablas, tabla);
            return criterioPedido;
        case DESCRIBE:
            if (requestParseada.cantidadParametros > 0) {
                tabla = requestParseada.parametros[0];
                criterioPedido = dictionary_get(metadataTablas, tabla);
                return criterioPedido;
            }
        default:
            return "NC";
    }
}