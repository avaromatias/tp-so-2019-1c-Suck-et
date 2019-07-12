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

    sem_t *mutexColaDeNew = (sem_t *) malloc(sizeof(sem_t));
    sem_t *mutexColaDeReady = (sem_t *) malloc(sizeof(sem_t));
    sem_t *cantidadProcesosEnNew = (sem_t *) malloc(sizeof(sem_t));
    sem_t *cantidadProcesosEnReady = (sem_t *) malloc(sizeof(sem_t));
    sem_t *mutexListaFinalizados = (sem_t *) malloc(sizeof(sem_t));
    sem_init(mutexColaDeNew, 0, 1);
    sem_init(mutexColaDeReady, 0, 0);
    sem_init(mutexListaFinalizados, 0, 1);
    sem_init(cantidadProcesosEnNew, 0, 0);
    sem_init(cantidadProcesosEnReady, 0, 0);

    t_configuracion configuracion = cargarConfiguracion("kernel.cfg", logger);

    log_info(logger, "IP Memoria: %s", configuracion.ipMemoria);
    log_info(logger, "Puerto Memoria: %i", configuracion.puertoMemoria);
    log_info(logger, "Quantum: %i", configuracion.quantum);
    log_info(logger, "Multiprocesamiento: %i", configuracion.multiprocesamiento);
    log_info(logger, "Refresh Metadata: %i", configuracion.refreshMetadata);
    log_info(logger, "Retardo de Ejecución : %i", configuracion.refreshMetadata);

    t_dictionary *metadataTablas = dictionary_create(); //voy a tener el nombreTabla, criterio, particiones y tpo_Compactación
    t_dictionary *tablaDeMemoriasConCriterios = dictionary_create();//tendremos por cada criterio una lista de memorias

    t_list *listaDeCriteriosSC = list_create();
    t_list *listaDeCriteriosSHC = list_create();
    t_list *listaDeCriteriosEC = list_create();

    //voy a tener relacion de criterio con un t_list* de FDs
    dictionary_put(tablaDeMemoriasConCriterios, "SC", listaDeCriteriosSC);
    dictionary_put(tablaDeMemoriasConCriterios, "SHC", listaDeCriteriosSHC);
    dictionary_put(tablaDeMemoriasConCriterios, "EC", listaDeCriteriosEC);

    t_queue *colaDeNew = queue_create();
    t_queue *colaDeReady = queue_create();
    t_queue *colaDeFinalizados = queue_create();

    GestorConexiones *misConexiones = inicializarConexion();
    conectarseAMemoriaPrincipal(configuracion.ipMemoria, configuracion.puertoMemoria, misConexiones, logger);

    pthread_t *hiloRespuestas = crearHiloConexiones(misConexiones, logger, tablaDeMemoriasConCriterios);

    p_consola_kernel *parametros = (p_consola_kernel *) malloc(sizeof(p_consola_kernel));

    parametros->logger = logger;
    parametros->conexiones = misConexiones;
    parametros->metadataTablas = metadataTablas;
    parametros->memoriasConCriterios = tablaDeMemoriasConCriterios;

    parametros_plp *parametrosPLP = (parametros_plp *) malloc(sizeof(parametros_plp));

    parametrosPLP->colaDeNew = colaDeNew;
    parametrosPLP->colaDeReady = colaDeReady;
    parametrosPLP->mutexColaDeNew = mutexColaDeNew;
    parametrosPLP->mutexColaDeReady = mutexColaDeReady;
    parametrosPLP->cantidadProcesosEnNew = cantidadProcesosEnNew;
    parametrosPLP->cantidadProcesosEnReady = cantidadProcesosEnReady;
    parametrosPLP->logger = logger;

    pthread_t *hiloPlanificadorLargoPlazo = crearHiloPlanificadorLargoPlazo(parametrosPLP);

    parametros_pcp *parametrosPCP = (parametros_pcp *) malloc(sizeof(parametros_pcp));

    parametrosPCP->quantum = (int *) configuracion.quantum;
    parametrosPCP->colaDeReady = colaDeReady;
    parametrosPCP->mutexColaDeReady = mutexColaDeReady;
    parametrosPCP->mutexListaFinalizados = mutexListaFinalizados;
    parametrosPCP->cantidadProcesosEnReady = cantidadProcesosEnReady;
    parametrosPCP->logger = logger;
    parametrosPCP->colaDeFinalizados = colaDeFinalizados;

    for (int i = 0; i < configuracion.multiprocesamiento; i++) {
        crearHiloPlanificadorCortoPlazo(parametrosPCP);
    }

    ejecutarConsola(parametros, configuracion, parametrosPLP);

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

void ejecutarConsola(p_consola_kernel *parametros, t_configuracion configuracion, parametros_plp *parametrosPLP) {
    t_comando requestParseada;
    bool requestEsValida, seEncola;
    sem_t *semaforo_colaDeNew = parametrosPLP->mutexColaDeNew;

    do {
        char *leido = readline("Kernel@suck-ets:~$ ");
        char **comandoParseado = parser(leido);
        if (comandoParseado == NULL) {
            free(comandoParseado);
            continue;
        }
        requestParseada = instanciarComando(comandoParseado);
        requestEsValida = analizarRequest(requestParseada, parametros);
        if (requestEsValida == true) {
            seEncola = encolarDirectoNuevoPedido(requestParseada);
            if (seEncola == true) {
                t_archivoLQL *unLQL = convertirRequestALQL(requestParseada);
                sem_wait(semaforo_colaDeNew);
                queue_push(parametrosPLP->colaDeNew, unLQL);
                sem_post(parametrosPLP->cantidadProcesosEnNew);
                sem_post(semaforo_colaDeNew);
            } else {
                gestionarRequestKernel(requestParseada, parametros, parametrosPLP);
            }
        } else {
            log_error(parametros->logger, "No se pudo procesar la request solicitada.");
        }
        //free(leido);
        //free(comandoParseado);
    } while (requestParseada.tipoRequest != EXIT);
    printf("Ya analizamos todo lo solicitado.\n");
}

bool analizarRequest(t_comando requestParseada, p_consola_kernel *parametros) {
    t_log *logger = parametros->logger;

    if (requestParseada.tipoRequest == INVALIDO) {
        log_error(logger, "Comando inválido.");
        return false;
    } else {
        if (validarComandosComunes(requestParseada, logger) || validarComandosKernel(requestParseada, logger)) {
            return true;
        } else {
            return false;
        }
    }
}

int gestionarRequestPrimitivas(t_comando requestParseada, p_consola_kernel *parametros) {

    char *criterioConsistencia;
    int fdMemoria;

    switch (requestParseada.tipoRequest) { //Analizar si cada gestionar va a tener que encolar en NEW, en lugar de enviarPaquete
        case SELECT:
            criterioConsistencia = criterioBuscado(requestParseada, parametros->metadataTablas);
            fdMemoria = seleccionarMemoriaIndicada(parametros, criterioConsistencia);
            return gestionarSelectKernel(requestParseada.parametros[0], requestParseada.parametros[1], fdMemoria);
        case INSERT:
            criterioConsistencia = criterioBuscado(requestParseada, parametros->metadataTablas);
            fdMemoria = seleccionarMemoriaIndicada(parametros, criterioConsistencia);
            return gestionarInsertKernel(requestParseada.parametros[0], requestParseada.parametros[1],
                                         requestParseada.parametros[2],
                                         fdMemoria);
        case CREATE:
            criterioConsistencia = criterioBuscado(requestParseada, parametros->metadataTablas);
            fdMemoria = seleccionarMemoriaIndicada(parametros, criterioConsistencia);
            dictionary_put(parametros->metadataTablas, requestParseada.parametros[0], requestParseada.parametros[1]);
            gestionarCreateKernel(requestParseada.parametros[0], requestParseada.parametros[1],
                                  requestParseada.parametros[2],
                                  requestParseada.parametros[3], fdMemoria);
            return 0;
        case DROP:
            criterioConsistencia = criterioBuscado(requestParseada, parametros->metadataTablas);
            fdMemoria = seleccionarMemoriaIndicada(parametros, criterioConsistencia);
            return gestionarDropKernel(requestParseada.parametros[0], fdMemoria);
        case DESCRIBE:
            return 0;//gestionarDescribeKernel(); //solamente trabaja con una tabla en particular
    }
}

int gestionarRequestKernel(t_comando requestParseada, p_consola_kernel *parametros, parametros_plp *parametrosPLP) {

    switch (requestParseada.tipoRequest) {
        case DESCRIBE:
            //gestionarDescribeGlobal();
            return 0;
        case ADD:
            printf("Tipo de Request: %s %s %s \n", "ADD", requestParseada.parametros[0], requestParseada.parametros[1]);
            printf("To: %s \n", requestParseada.parametros[3]);
            gestionarAdd(requestParseada.parametros, parametros);
            return 0;
        case RUN:
            gestionarRun(requestParseada.parametros[0], parametros, parametrosPLP);
            break;
        case METRICS:
            //gestionarMetricas();
            break;
        case JOURNAL:
            //gestionarJournalKernel(parametros);
            return 0;
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

bool validarComandosKernel(t_comando comando, t_log *logger) {
    bool esValido = esComandoValidoDeKernel(comando);

    if (!esValido)
        //log_warning(logger, "Alguno de los parámetros ingresados es incorrecto. Por favor verifique su entrada.");

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

int gestionarSelectKernel(char *nombreTabla, char *key, int fdMemoria) {
    char *request = string_from_format("SELECT %s %s", nombreTabla, key);
    enviarPaquete(fdMemoria, REQUEST, SELECT, request);
    free(request);
    return 0;
}

int
gestionarCreateKernel(char *tabla, char *consistencia, char *cantParticiones, char *tiempoCompactacion, int fdMemoria) {
    char *request = string_from_format("CREATE %s %s %s %s", tabla, consistencia, cantParticiones, tiempoCompactacion);
    enviarPaquete(fdMemoria, REQUEST, CREATE, request);
    free(request);
    //recibo mensaje de Memoria o directamente fallo yo?
    return 0;
}

int gestionarInsertKernel(char *nombreTabla, char *key, char *valor, int fdMemoria) {
    char *request = string_from_format("INSERT %s %s %s", nombreTabla, key, valor);
    enviarPaquete(fdMemoria, REQUEST, INSERT, request);
    free(request);
    //recibo mensaje de Memoria o directamente fallo yo?
    return 0;
}

int gestionarDropKernel(char *nombreTabla, int fdMemoria) {
    char *request = string_from_format("DROP %s %s", nombreTabla);
    enviarPaquete(fdMemoria, REQUEST, DROP, request);
    free(request);
    //recibo mensaje de Memoria o directamente fallo yo?
    return 0;
}

/*int gestionarJournalKernel(p_consola_kernel *parametros) {
    char *request = string_from_format("JOURNAL");
    enviarPaquete(fdMemoria, REQUEST, JOURNAL, request);
    return 0;
}*///Corregir Journal

int gestionarRun(char *pathArchivo, p_consola_kernel *parametros, parametros_plp *parametrosPLP) {
    t_archivoLQL *unLQL = (t_archivoLQL *) malloc(sizeof(t_archivoLQL));
    t_log *logger = parametros->logger;
    sem_t *semaforo_colaDeNew = parametrosPLP->mutexColaDeNew;

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
        //Asignamos la linea al primer elemento de la cola de request
        queue_push(unLQL->colaDeRequests, linea);
        lineaLeida++;
    }
    unLQL->cantidadDeLineas = queue_size(unLQL->colaDeRequests);
    sem_wait(semaforo_colaDeNew);
    queue_push(parametrosPLP->colaDeNew, unLQL);
    sem_post(semaforo_colaDeNew);
    fclose(archivo);
}

int gestionarAdd(char **parametrosDeRequest, p_consola_kernel *parametros) {

    t_log *logger = parametros->logger;
    int numeroMemoria = atoi(parametrosDeRequest[1]);
    char *criterio = parametrosDeRequest[3];
    GestorConexiones *misConexiones = parametros->conexiones;
    t_dictionary *tablaMemoriasConCriterios = parametros->memoriasConCriterios;

    bool memoriaEncontrada = (list_size(misConexiones->conexiones) >= numeroMemoria);

    if (memoriaEncontrada) {
        int *fdMemoriaSolicitada = (int *) list_get(misConexiones->conexiones,
                                                    numeroMemoria - 1); // hacemos -1 por la ubicación 0
        t_list *listaFileDescriptors = dictionary_get(tablaMemoriasConCriterios, criterio);

        if (criterio == "SC") {
            int cantMemoriasSC = list_size(listaFileDescriptors);
            if (cantMemoriasSC >= 1) {
                log_error(logger, "Ya existe una memoria asignada al criterio SC.");
                return -1;
            }
        } else if ((criterio == "SHC") || (criterio == "EC")) {
            list_add(listaFileDescriptors, fdMemoriaSolicitada);
            log_info(logger, "La memoria ha sido agregada a la tabla de Memorias conocidas.\n");
            return 0;
        }
    } else {
        log_error(logger, "La memoria solicitada no se encuentra dentro de las memorias conocidas.\n");
        return -1;
    }
}

int seleccionarMemoriaIndicada(p_consola_kernel *parametros, char *criterio) {
    if (criterio != NULL) {
        if (existenMemoriasConectadas) {
            //Obtenemos memorias que responden al criterio pedido
            t_list *memoriasDelCriterioPedido = dictionary_get(parametros->memoriasConCriterios, criterio);
            //Que memorias tengo con el criterio X de la request solicitada?
            if (criterio == "SC") {
                int memoriaAsociadaASC = list_size(memoriasDelCriterioPedido);
                if (memoriaAsociadaASC == 1) {
                    int *fdMemoriaElegida = list_get(memoriasDelCriterioPedido, 0);
                    return *fdMemoriaElegida;
                } else {
                    log_error(parametros->logger, "No existe ninguna memoria asociada al criterio SC. \n");
                    return -1;
                }
            } else if (criterio == "SHC") {
                //FUNCION HASH
                //int cantidadFDsAsociadosSHC = list_size(memoriasDelCriterioPedido);
                //if (cantidadFDsAsociadosSHC > 0) {
                //}
            } else if (criterio == "EC") {
                int cantidadFDsAsociadosEC = list_size(memoriasDelCriterioPedido);
                if (cantidadFDsAsociadosEC > 0) {
                    int elementoBuscado = (cantidadFDsAsociadosEC -
                                           (cantidadFDsAsociadosEC -
                                            1));//Siempre el primero -ya se que tiene poco sentido-
                    t_list *memoriasDisponibles = list_take_and_remove(memoriasDelCriterioPedido,
                                                                       elementoBuscado); //Lista nueva
                    int *fdMemoriaElegida = list_get(memoriasDisponibles, 0);
                    list_add(memoriasDelCriterioPedido, fdMemoriaElegida);

                    list_destroy(memoriasDisponibles);
                    return *fdMemoriaElegida;
                } else {
                    log_error(parametros->logger, "No existen memorias asociadas al criterio EC.\n");
                    return -1;
                }
            }
        } else {
            log_warning(parametros->logger, "No existen memorias conectadas para asignar requests.\n");
            return -1;
        }
    }
}

bool existenMemoriasConectadas(GestorConexiones *misConexiones) {
    bool hayMemorias = (list_size(misConexiones->conexiones) > 0);

    return hayMemorias;
}

char *criterioBuscado(t_comando requestParseada, t_dictionary *metadataTablas) {
    //buscaremos el criterio de cada uno de las request ingresadas
    char *criterioPedido;
    char *tabla;
    switch (requestParseada.tipoRequest) { //Cada case va a tener que buscar en el diccionario la tabla, para obtener el criterio
        case SELECT:
            tabla = requestParseada.parametros[0];
            criterioPedido = (char *) dictionary_get(metadataTablas, tabla);//buscar en metadataTablasConocidas
            return criterioPedido;
        case INSERT:
            tabla = requestParseada.parametros[0];
            criterioPedido = (char *) dictionary_get(metadataTablas, tabla);
            return criterioPedido;
        case CREATE:
            criterioPedido = requestParseada.parametros[1];
            return criterioPedido;
        case DROP:
            tabla = requestParseada.parametros[0];
            criterioPedido = (char *) dictionary_get(metadataTablas, tabla);
            return criterioPedido;
        case DESCRIBE:
            if (requestParseada.cantidadParametros > 0) {
                tabla = requestParseada.parametros[0];
                criterioPedido = (char *) dictionary_get(metadataTablas, tabla);
                return criterioPedido;
            }
        case ADD:
            tabla = requestParseada.parametros[3];
            criterioPedido = (char *) dictionary_get(metadataTablas, tabla);
            return criterioPedido;
        default:
            return "NC";
    }
}

char **obtenerDatosDeConexion(char *datosConexionMemoria) { //Para Gossipping
    datosConexionMemoria = "192.168.0.52:8001;192.168.0.45:8003;192.168.0.99:13002;";
    string_trim(&datosConexionMemoria);
    if (string_is_empty(datosConexionMemoria))
        return NULL;
    char **direcciones = string_split(datosConexionMemoria, ";");
    //"192.168.0.52:8001""192.168.0.45:8003""192.168.0.99:13002"
    int cantidadDireccionesRecibidas = tamanioDeArrayDeStrings(direcciones);//3
    char **direccionesDeMemorias = (char **) malloc(sizeof(char *) * cantidadDireccionesRecibidas);
    int memoria = 0;

    while (memoria < cantidadDireccionesRecibidas) {
        direccionesDeMemorias[memoria] = direcciones[memoria];
        memoria++;
    }
    return direccionesDeMemorias;
}

/****************************
 ****** PLANIFICACIÓN *******
 ****************************/

pthread_t *crearHiloPlanificadorLargoPlazo(parametros_plp *parametros) {
    pthread_t *hiloPLP = (pthread_t *) malloc(sizeof(pthread_t));

    pthread_create(hiloPLP, NULL, &sincronizacionPLP, parametros);

    return hiloPLP;
}

void *sincronizacionPLP(void *parametrosPLP) {
    //Haciendo como ahora, estaría modificando las cosas dentro de la funcion, deberia ser asi?
    parametros_plp *parametros_PLP = (parametros_plp *) parametrosPLP;
    sem_t *semaforo_colaDeNew = parametros_PLP->mutexColaDeNew;
    sem_t *semaforo_colaDeReady = parametros_PLP->mutexColaDeReady;

    while (1) {
        sem_wait(parametros_PLP->cantidadProcesosEnNew);
        sem_wait(semaforo_colaDeNew);
        t_archivoLQL *unLQL = queue_pop(parametros_PLP->colaDeNew);
        sem_post(semaforo_colaDeNew);
        sem_wait(semaforo_colaDeReady);
        queue_push(parametros_PLP->colaDeReady, unLQL);
        sem_post(semaforo_colaDeReady);
        sem_post(parametros_PLP->cantidadProcesosEnReady);
    }
}

bool encolarDirectoNuevoPedido(t_comando requestParseada) {
    switch (requestParseada.tipoRequest) {
        case SELECT:
        case CREATE:
        case INSERT:
        case DROP:
            return true;
        case DESCRIBE:
            if ((requestParseada.parametros[0]) != NULL) return true;
            else return false;
        case ADD:
        case RUN:
        case METRICS:
        case JOURNAL: //definir si la encolamos o decidimos ejecutarla instantaneamente
        case EXIT:
            return false;
    }
}

t_archivoLQL *convertirRequestALQL(t_comando requestParseada) {
    t_archivoLQL *unLQL = (t_archivoLQL *) malloc(sizeof(t_archivoLQL));
    char *requestDelLQL = string_new();

    switch (requestParseada.tipoRequest) {
        case SELECT:
            string_append(&requestDelLQL, "SELECT");
        case INSERT:
            string_append(&requestDelLQL, "INSERT");
        case CREATE:
            string_append(&requestDelLQL, "CREATE");
        case DROP:
            string_append(&requestDelLQL, "DROP");
        case DESCRIBE:
            string_append(&requestDelLQL, "DESCRIBE");
    }
    for (int i = 0; i < requestParseada.cantidadParametros; i++) {
        string_append(&requestDelLQL, requestParseada.parametros[i]);
    }

    unLQL->cantidadDeLineas = queue_size(unLQL->colaDeRequests);
    queue_push(unLQL->colaDeRequests, requestDelLQL);

    return unLQL;
}

pthread_t *crearHiloPlanificadorCortoPlazo(parametros_pcp *parametros) {
    pthread_t *hiloPCP = (pthread_t *) malloc(sizeof(pthread_t));

    pthread_create(hiloPCP, NULL, &instanciarPCPs, parametros);

    return hiloPCP;
}

void *planificarRequest(p_planificacion *paramPlanifGeneral, t_archivoLQL *archivoLQL) {
    p_consola_kernel *pConsolaKernel = paramPlanifGeneral->parametrosConsola;
    parametros_plp *parametrosPLP = paramPlanifGeneral->parametrosPLP;
    int quantumMaximo = (int) paramPlanifGeneral->parametrosPCP->quantum;
    t_archivoLQL *unLQL = archivoLQL;
    t_comando requestParseada;
    bool requestEsValida;

    for (int quantumsConsumidos = 0; quantumsConsumidos < quantumMaximo; quantumsConsumidos++) {
        int lineaDeEjecucion = 0;//dejamos por si queremos hacer un lineaDeEjecución++
        char *unaRequest = queue_pop(unLQL->colaDeRequests);
        char **comandoParseado = parser(unaRequest);
        if (comandoParseado == NULL) {
            free(comandoParseado);
            break;
        }
        requestParseada = instanciarComando(comandoParseado);
        requestEsValida = analizarRequest(requestParseada, pConsolaKernel);
        if (requestEsValida == true) {
            if ((diferenciarRequest(requestParseada) == 1)) { //Si es 1 es primitiva
                gestionarRequestPrimitivas(requestParseada, pConsolaKernel);
                actualizarCantRequest(archivoLQL, requestParseada); //para métricas
            } else { //Si es 0 es comando de Kernel
                gestionarRequestKernel(requestParseada, pConsolaKernel, parametrosPLP);
            }
        } else {
            log_warning(pConsolaKernel->logger,
                        "No se pudo procesar la request ubicada en la línea %s. Corríjala y vuelvala a ejecutar.",
                        lineaDeEjecucion);
            sem_wait(paramPlanifGeneral->parametrosPCP->mutexListaFinalizados);
            queue_push(paramPlanifGeneral->parametrosPCP->colaDeFinalizados, unLQL);
            sem_post(paramPlanifGeneral->parametrosPCP->mutexListaFinalizados);
            break;
            //TODO: ADEMAS DEBEMOS SALIR DE LA EJECUCION DE ESTE LQL -CONSULTAR FER
        }
    }
    if (requestEsValida == true) {
        sem_wait(paramPlanifGeneral->parametrosPCP->mutexColaDeReady);
        queue_push(paramPlanifGeneral->parametrosPCP->colaDeReady, unLQL);
        sem_post(paramPlanifGeneral->parametrosPCP->mutexColaDeReady);
        sem_post(parametrosPLP->cantidadProcesosEnReady);
    }
}

void actualizarCantRequest(t_archivoLQL *archivoLQL, t_comando requestParseada) {
    switch (requestParseada.tipoRequest) {
        case SELECT:
            archivoLQL->cantidadDeSelectProcesados++;
        case INSERT:
            archivoLQL->cantidadDeInsertProcesados++;
    }
}

void
instanciarPCPs(parametros_pcp *parametrosPCP, parametros_plp *parametrosPLP, p_consola_kernel *parametrosConsola) {
    sem_t *semaforo_cantidadProcesosEnReady = parametrosPCP->cantidadProcesosEnReady;
    sem_t *semaforo_mutexColaDeReady = parametrosPCP->mutexColaDeReady;

    while (1) {
        sem_wait(semaforo_cantidadProcesosEnReady);
        sem_wait(semaforo_mutexColaDeReady);
        t_archivoLQL *nuevoLQL = queue_pop(parametrosPCP->colaDeReady);
        sem_post(semaforo_mutexColaDeReady);

        p_planificacion *paramPlanificacionGeneral = (p_planificacion *) malloc(sizeof(p_planificacion));
        paramPlanificacionGeneral->parametrosConsola = parametrosConsola;
        paramPlanificacionGeneral->parametrosPCP = parametrosPCP;
        paramPlanificacionGeneral->parametrosPLP = parametrosPLP;

        planificarRequest(paramPlanificacionGeneral, nuevoLQL);
    }
}

int diferenciarRequest(t_comando requestParseada) {
    switch (requestParseada.tipoRequest) {
        case SELECT:
        case CREATE:
        case INSERT:
        case DROP:
        case DESCRIBE:
            return 1;
        case ADD:
        case RUN:
        case METRICS:
        case JOURNAL:
        case EXIT:
            return 0;
    }
}

