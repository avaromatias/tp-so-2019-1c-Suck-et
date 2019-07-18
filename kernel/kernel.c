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

    t_dictionary *supervisorDeHilos = dictionary_create();//Va a tener como KEY el PID, y la data el SEMÁFORO de c/hilo
    int contadorPIDs = 0;
    int memoriasUtilizables = 0;

    pthread_mutex_t *mutexJournal = malloc(sizeof(pthread_mutex_t));
    pthread_mutex_init(mutexJournal, NULL);
    sem_t *mutexColaDeNew = (sem_t *) malloc(sizeof(sem_t));
    sem_t *mutexColaDeReady = (sem_t *) malloc(sizeof(sem_t));
    sem_t *cantidadProcesosEnNew = (sem_t *) malloc(sizeof(sem_t));
    sem_t *cantidadProcesosEnReady = (sem_t *) malloc(sizeof(sem_t));
    sem_t *mutexListaFinalizados = (sem_t *) malloc(sizeof(sem_t));
    sem_init(mutexColaDeNew, 0, 1);
    sem_init(mutexColaDeReady, 0, 1);
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

    pthread_t *hiloRespuestas = crearHiloConexiones(misConexiones, logger, tablaDeMemoriasConCriterios, metadataTablas,
                                                    mutexJournal, supervisorDeHilos);

    p_consola_kernel *pConsolaKernel = (p_consola_kernel *) malloc(sizeof(p_consola_kernel));

    pConsolaKernel->conexiones = misConexiones;
    pConsolaKernel->metadataTablas = metadataTablas;
    pConsolaKernel->memoriasConCriterios = tablaDeMemoriasConCriterios;
    pConsolaKernel->logger = logger;

    parametros_plp *parametrosPLP = (parametros_plp *) malloc(sizeof(parametros_plp));

    parametrosPLP->colaDeNew = colaDeNew;
    parametrosPLP->colaDeReady = colaDeReady;
    parametrosPLP->mutexColaDeNew = mutexColaDeNew;
    parametrosPLP->mutexColaDeReady = mutexColaDeReady;
    parametrosPLP->cantidadProcesosEnNew = cantidadProcesosEnNew;
    parametrosPLP->cantidadProcesosEnReady = cantidadProcesosEnReady;
    parametrosPLP->contadorPID = contadorPIDs;
    parametrosPLP->logger = logger;

    pthread_t *hiloPlanificadorLargoPlazo = crearHiloPlanificadorLargoPlazo(parametrosPLP);

    parametros_pcp *parametrosPCP = (parametros_pcp *) malloc(sizeof(parametros_pcp));

    parametrosPCP->quantum = (int *) configuracion.quantum;
    parametrosPCP->colaDeReady = colaDeReady;
    parametrosPCP->mutexColaDeReady = mutexColaDeReady;
    parametrosPCP->mutexColaFinalizados = mutexListaFinalizados;
    parametrosPCP->cantidadProcesosEnReady = cantidadProcesosEnReady;
    parametrosPCP->colaDeFinalizados = colaDeFinalizados;
    parametrosPCP->mutexJournal = mutexJournal;
    parametrosPCP->logger = logger;

    paramPlanificacionGeneral = (p_planificacion *) malloc(sizeof(p_planificacion));

    paramPlanificacionGeneral->parametrosConsola = pConsolaKernel;
    paramPlanificacionGeneral->parametrosPCP = parametrosPCP;
    paramPlanificacionGeneral->parametrosPLP = parametrosPLP;
    paramPlanificacionGeneral->supervisorDeHilos = supervisorDeHilos;
    paramPlanificacionGeneral->memoriasUtilizables = memoriasUtilizables;

    for (int i = 0; i < configuracion.multiprocesamiento; i++) {
//        paramPlanificacionGeneral->parametrosPCP->mutexSemaforoHilo = semaforoHilo;
        crearHiloPlanificadorCortoPlazo(paramPlanificacionGeneral);
    }

    ejecutarConsola(pConsolaKernel, configuracion, paramPlanificacionGeneral);

    pthread_join(*hiloRespuestas, NULL);

    free(pConsolaKernel);
    free(parametrosPCP);
    free(parametrosPLP);
    free(paramPlanificacionGeneral);
    return EXIT_SUCCESS;
}

/****************************
 *** COMPORTAMIENTO KERNEL***
 ****************************/


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

void ejecutarConsola(p_consola_kernel *parametros, t_configuracion configuracion, p_planificacion *paramPlanifGral) {
    parametros_plp *parametrosPLP = paramPlanifGral->parametrosPLP;
    t_comando *requestParseada = (t_comando *) malloc(sizeof(t_comando));
    bool requestEsValida = true;
    bool seEncola = true;
    sem_t *semaforo_colaDeNew = parametrosPLP->mutexColaDeNew;

    do {
        char *leido = readline("Kernel@suck-ets:~$ ");
        char **comandoParseado = parser(leido);
        if (comandoParseado == NULL) {
            free(comandoParseado);
            continue;
        }
        *requestParseada = instanciarComando(comandoParseado);
        requestEsValida = analizarRequest(*requestParseada, parametros);
        if (requestEsValida == true) {
            seEncola = encolarDirectoNuevoPedido(*requestParseada);
            if (seEncola == true) {
                t_archivoLQL *unLQL = convertirRequestALQL(requestParseada);
                sem_wait(semaforo_colaDeNew);
                queue_push(parametrosPLP->colaDeNew, unLQL);
                sem_post(parametrosPLP->cantidadProcesosEnNew);
                sem_post(semaforo_colaDeNew);
            } else {
                gestionarRequestKernel(*requestParseada, paramPlanifGral);
            }
        } else {
            log_error(parametros->logger, "No se pudo procesar la request solicitada.");
        }
        //free(leido);
        //free(comandoParseado);
    } while (requestParseada->tipoRequest != EXIT);
    printf("Ya analizamos todo lo solicitado.\n");
}

bool analizarRequest(t_comando requestParseada, p_consola_kernel *parametros) {
    t_log *logger = parametros->logger;

    if (requestParseada.tipoRequest == INVALIDO) {
        log_error(logger, "Comando inválido.");
        return false;
    } else {
        if ((validarComandosKernel(requestParseada, logger)) == true) {
            return true;
        } else if ((validarComandosComunes(requestParseada, logger))) {
            return true;
        } else {
            return false;
        }
    }
}

int gestionarRequestPrimitivas(t_comando requestParseada, p_planificacion *paramPlanifGeneral,
                               pthread_mutex_t *mutexDeHiloRequest) {
    int PID = paramPlanifGeneral->parametrosPLP->contadorPID;
    char *PIDCasteado = string_itoa(PID);
    dictionary_put(paramPlanifGeneral->supervisorDeHilos, PIDCasteado, mutexDeHiloRequest);
    pthread_mutex_lock(mutexDeHiloRequest);
    p_consola_kernel *pConsolaKernel = paramPlanifGeneral->parametrosConsola;

    char *criterioConsistencia;
    int fdMemoria;
    t_log *logger = pConsolaKernel->logger;
    if(paramPlanifGeneral->memoriasUtilizables > 0) {
        switch (requestParseada.tipoRequest) { //Analizar si cada gestionar va a tener que encolar en NEW, en lugar de enviarPaquete
            case SELECT:
                if (dictionary_has_key(pConsolaKernel->metadataTablas, requestParseada.parametros[0])) {
                    criterioConsistencia = criterioBuscado(requestParseada, pConsolaKernel->metadataTablas);
                    int key = atoi(requestParseada.parametros[2]);
                    fdMemoria = seleccionarMemoriaIndicada(pConsolaKernel, criterioConsistencia, key);
                    return gestionarSelectKernel(requestParseada.parametros[0], requestParseada.parametros[1],
                                                 fdMemoria,
                                                 PID);
                } else {
                    log_error(logger, "La tabla no se encuentra dentro de la Metadata conocida.");
                    return -1;
                }
            case INSERT:
                if (dictionary_has_key(pConsolaKernel->metadataTablas, requestParseada.parametros[0])) {
                    criterioConsistencia = criterioBuscado(requestParseada, pConsolaKernel->metadataTablas);
                    if (criterioConsistencia != NULL) {
                        int key = atoi(requestParseada.parametros[2]);
                        fdMemoria = seleccionarMemoriaIndicada(pConsolaKernel, criterioConsistencia, key);
                        return gestionarInsertKernel(requestParseada.parametros[0], requestParseada.parametros[1],
                                                     requestParseada.parametros[2], fdMemoria, PID);
                    } else {
                        log_error(logger, "El criterio es nulo, no se puede analizar.");
                        return -1;
                    }
                } else {
                    log_error(logger, "La tabla no se encuentra dentro de la Metadata conocida.");
                    return -1;
                }
            case CREATE:
                criterioConsistencia = criterioBuscado(requestParseada, pConsolaKernel->metadataTablas);
                if (criterioConsistencia != NULL) {
                    fdMemoria = seleccionarMemoriaIndicada(pConsolaKernel, criterioConsistencia, NULL);
                    return gestionarCreateKernel(requestParseada.parametros[0], requestParseada.parametros[1],
                                                 requestParseada.parametros[2], requestParseada.parametros[3],
                                                 fdMemoria, PID);
                } else {
                    log_error(logger, "El criterio es nulo, no se puede analizar.");
                    return -1;
                }
            case DROP:
                if (dictionary_has_key(pConsolaKernel->metadataTablas, requestParseada.parametros[0])) {
                    criterioConsistencia = criterioBuscado(requestParseada, pConsolaKernel->metadataTablas);
                    fdMemoria = seleccionarMemoriaIndicada(pConsolaKernel, criterioConsistencia, NULL);
                    return gestionarDropKernel(requestParseada.parametros[0], fdMemoria, PID);
                } else {
                    log_error(logger, "La tabla no se encuentra dentro de la Metadata conocida.");
                    return -1;
                }
            case DESCRIBE:
                if (requestParseada.cantidadParametros == 1) {
                    //Estamos hablando del Describe de una tabla en particular
                    if (dictionary_has_key(pConsolaKernel->metadataTablas, requestParseada.parametros[0])) {
                        criterioConsistencia = criterioBuscado(requestParseada, pConsolaKernel->metadataTablas);
                        fdMemoria = seleccionarMemoriaIndicada(pConsolaKernel, criterioConsistencia, NULL);
                        return gestionarDescribeTablaKernel(requestParseada.parametros[0], fdMemoria, PID);
                    } else {
                        log_error(logger, "La tabla no se encuentra dentro de la Metadata conocida.");
                        return -1;
                    }
                } else {
                    fdMemoria = seleccionarMemoriaParaDescribe(pConsolaKernel);
                    return gestionarDescribeGlobalKernel(fdMemoria, PID);
                }
            case JOURNAL:
                return gestionarJournalKernel(paramPlanifGeneral);
        }
    } else {
        log_error(pConsolaKernel->logger, "No hay memorias utilizables, asocie memorias previamente.");
    }
}

int gestionarRequestKernel(t_comando requestParseada, p_planificacion *paramPlanifGeneral) {

    p_consola_kernel *pConsolaKernel = paramPlanifGeneral->parametrosConsola;
    parametros_plp *parametrosPLP = paramPlanifGeneral->parametrosPLP;

    switch (requestParseada.tipoRequest) {
        case ADD:
           return gestionarAdd(requestParseada.parametros, paramPlanifGeneral);
        case RUN:
            if(paramPlanifGeneral->memoriasUtilizables > 0) {
                gestionarRun(requestParseada.parametros[0], pConsolaKernel, parametrosPLP);
            } else {
                log_error(pConsolaKernel->logger, "No hay memorias utilizables, asocie memorias previamente.");
                break;
            }
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

bool validarComandosKernel(t_comando comando, t_log *logger) {
    bool esValido = esComandoValidoDeKernel(comando);
    return esValido;
}

bool esComandoValidoDeKernel(t_comando comando) {
    switch (comando.tipoRequest) {
        case ADD:
            if(comando.cantidadParametros == 4){
                if(esString(comando.parametros[0]) && esString(comando.parametros[2]) &&
                   esString(comando.parametros[3]) && esEntero(comando.parametros[1])) {
                    string_to_upper(comando.parametros[0]);
                    string_to_upper(comando.parametros[2]);
                    if((strcmp(comando.parametros[0],"MEMORY") == 0) && (strcmp(comando.parametros[2],"TO") == 0))
                        return true;
                } return false;
            } else return false;
        case RUN:
            return (comando.cantidadParametros == 1 && esString(comando.parametros[0]));
        case METRICS:
            return (comando.cantidadParametros == 0);
        default:
            return false;
    }
}

int gestionarSelectKernel(char *nombreTabla, char *key, int fdMemoria, int PID) {
    char *request = string_from_format("SELECT %s %s", nombreTabla, key);
    enviarPaquete(fdMemoria, REQUEST, SELECT, request, PID);
    free(request);
    return 0;
}

int
gestionarCreateKernel(char *tabla, char *consistencia, char *cantParticiones, char *tiempoCompactacion, int fdMemoria,
                      int PID) {
    char *request = string_from_format("CREATE %s %s %s %s", tabla, consistencia, cantParticiones, tiempoCompactacion);
    enviarPaquete(fdMemoria, REQUEST, CREATE, request, PID);
    free(request);
    return 0;
}

int gestionarInsertKernel(char *nombreTabla, char *key, char *valor, int fdMemoria, int PID) {
    char *request = string_from_format("INSERT %s %s %s", nombreTabla, key, valor);
    enviarPaquete(fdMemoria, REQUEST, INSERT, request, PID);
    free(request);
    return 0;
}

int gestionarDropKernel(char *nombreTabla, int fdMemoria, int PID) {
    char *request = string_from_format("DROP %s", nombreTabla);
    enviarPaquete(fdMemoria, REQUEST, DROP, request, PID);
    free(request);
    return 0;
}

int gestionarDescribeTablaKernel(char *nombreTabla, int fdMemoria, int PID) {
    char *request = string_from_format("DESCRIBE %s", nombreTabla);
    enviarPaquete(fdMemoria, REQUEST, DESCRIBE, request, PID);
    free(request);
    return 0;
}

int gestionarDescribeGlobalKernel(int fdMemoria, int PID) {
    char *request = string_from_format("DESCRIBE");
    enviarPaquete(fdMemoria, REQUEST, DESCRIBE, request, PID);
    free(request);
    return 0;
}

int gestionarJournalKernel(p_planificacion *paramPlanifGeneral) {
    p_consola_kernel *pConsolaKernel = paramPlanifGeneral->parametrosConsola;
    t_list *memoriasConectadas = pConsolaKernel->conexiones->conexiones;
    int cantidadMemoriasConectadas = list_size(memoriasConectadas);

    for (int i = 0; i < cantidadMemoriasConectadas; i++) {
        int *memoriaSeleccionada = list_get(memoriasConectadas, i);
        enviarJournal(*memoriaSeleccionada, paramPlanifGeneral);
    }
    return 0;
}

void enviarJournal(int fdMemoria, p_planificacion *paramPlanifGeneral) {
    int PID = paramPlanifGeneral->parametrosPLP->contadorPID;
    enviarPaquete(fdMemoria, REQUEST, JOURNAL, "JOURNAL", PID);
    char *PIDCasteado = string_itoa(PID);
    pthread_mutex_t *mutexDeHiloRequest;
    if (dictionary_has_key(paramPlanifGeneral->supervisorDeHilos, PIDCasteado)) {
        mutexDeHiloRequest = dictionary_get(paramPlanifGeneral->supervisorDeHilos, PIDCasteado);
    } else {
        mutexDeHiloRequest = paramPlanificacionGeneral->parametrosPCP->mutexSemaforoHilo;
        dictionary_put(paramPlanifGeneral->supervisorDeHilos, PIDCasteado, mutexDeHiloRequest);
    }
    pthread_mutex_lock(mutexDeHiloRequest);
}

int extensionCorrecta(char *direccionAbsoluta) {
    direccionAbsoluta = strrchr(direccionAbsoluta, '.'); //

    if (direccionAbsoluta != NULL) {
        return (strcmp(direccionAbsoluta, ".lql"));
    }
}

t_archivoLQL *crearLQL(parametros_plp *parametrosPLP) {
    t_archivoLQL *unLQL = (t_archivoLQL *) malloc(sizeof(t_archivoLQL));
    unLQL->colaDeRequests = queue_create();
    unLQL->cantidadDeLineas = 0;
    unLQL->cantidadDeSelectProcesados = 0;
    unLQL->cantidadDeInsertProcesados = 0;
    unLQL->PID = (parametrosPLP->contadorPID)++;
    return unLQL;
}

int gestionarRun(char *pathArchivo, p_consola_kernel *parametros, parametros_plp *parametrosPLP) {
    t_archivoLQL *unLQL = crearLQL(parametrosPLP);
    t_log *logger = parametros->logger;

    if (extensionCorrecta(pathArchivo) == 0) {

        sem_t *semaforo_colaDeNew = parametrosPLP->mutexColaDeNew;

        char *linea = NULL;
        int lineaLeida = 0;
        int caracter, contador;

        if (sePuedeLeerElArchivo(pathArchivo)) {
            FILE *archivo = fopen(pathArchivo, "r");
            int tamanioArchivo = getFileSize(archivo);
            char *textoDelArchivo = (char *) malloc(tamanioArchivo + 1);

            fread(textoDelArchivo, 1, tamanioArchivo, archivo);
            fclose(archivo);

            textoDelArchivo[tamanioArchivo] = 0;
            char **lineas = string_split(textoDelArchivo, "\n");
            int cantidadRequests = tamanioDeArrayDeStrings(lineas);
            for (int i = 0; i < cantidadRequests; i++) {
                t_comando *requestParseada = (t_comando *) malloc(sizeof(t_comando));
                char **lineaParseada = parser(lineas[i]);
                *requestParseada = instanciarComando(lineaParseada);
                queue_push(unLQL->colaDeRequests, requestParseada);
            }
            unLQL->cantidadDeLineas = queue_size(unLQL->colaDeRequests);
            sem_wait(semaforo_colaDeNew);
            queue_push(parametrosPLP->colaDeNew, unLQL);
            sem_post(parametrosPLP->cantidadProcesosEnNew);
            sem_post(semaforo_colaDeNew);
        } else {
            log_error(logger, "El archivo solicitado es incorrecto. Verifique su entrada.");
        }
    } else {
        log_error(logger, "La extensión del archivo no parece ser la indicada. Verifique ingresar un archivo .LQL");
    }
}

int gestionarAdd(char **parametrosDeRequest, p_planificacion *paramPlanificacionGeneral) {

    p_consola_kernel *pConsolaKernel = paramPlanificacionGeneral->parametrosConsola;
    t_log *logger = pConsolaKernel->logger;

    int numeroMemoria = atoi(parametrosDeRequest[1]);
    char *criterio = parametrosDeRequest[3];
    string_to_upper(criterio);
    GestorConexiones *misConexiones = pConsolaKernel->conexiones;
    t_dictionary *tablaMemoriasConCriterios = pConsolaKernel->memoriasConCriterios;

    bool memoriaEncontrada = (list_size(misConexiones->conexiones) >= numeroMemoria);

    if (memoriaEncontrada) {

        int *fdMemoriaSolicitada = (int *) list_get(misConexiones->conexiones, numeroMemoria - 1);
        // hacemos -1 por la ubicación 0

        if ((strcmp("SC", criterio) == 0) || (strcmp("SHC", criterio) == 0) || (strcmp("EC", criterio) == 0)) {

            t_list *listaFileDescriptors = dictionary_get(tablaMemoriasConCriterios, criterio);

            if (strcmp("SC", criterio) == 0) {
                int cantMemoriasSC = list_size(listaFileDescriptors);
                if (cantMemoriasSC >= 1) {
                    log_error(logger, "Ya existe una memoria asignada al criterio SC.");
                    return -1;
                } else {
                    list_add(listaFileDescriptors, fdMemoriaSolicitada);
                    imprimirMensajeAdd(numeroMemoria, criterio);
                    paramPlanificacionGeneral->memoriasUtilizables++;
                    return 0;
                }
            } else if (strcmp("SHC", criterio) == 0 || strcmp("EC", criterio) == 0) {
                list_add(listaFileDescriptors, fdMemoriaSolicitada);
                imprimirMensajeAdd(numeroMemoria, criterio);
                paramPlanificacionGeneral->memoriasUtilizables++;
                return 0;
            }
        } else {
            log_error(logger, "El criterio escrito no está dentro de los avalados.");
            return -1;
        }
    } else {
        log_warning(logger, "La memoria no se encuentra dentro de las posibles a utilizar.");
        return -1;
    }
}

void imprimirMensajeAdd(int numeroMemoria, char* criterio) {
    printf("Tipo de Request: %s %i \n", "ADD MEMORY", numeroMemoria);
    printf("To: %s\n", criterio);
}

int seleccionarMemoriaParaDescribe(p_consola_kernel *parametros) {
    GestorConexiones *misConexiones = parametros->conexiones;
    int *fdMemoria;

    int tamanioListaConexiones = list_size(parametros->conexiones->conexiones);

    int numeroMemoriaElegida = rand() % (tamanioListaConexiones);//me devuelve un numero entre 0 y tamListaConex
    fdMemoria = (int *) list_get(misConexiones->conexiones, numeroMemoriaElegida);

    return *fdMemoria;
}

int seleccionarMemoriaIndicada(p_consola_kernel *parametros, char *criterio, int key) {
    t_log *logger = parametros->logger;
    if (criterio != NULL) {
        if (existenMemoriasConectadas) {
            string_to_upper(criterio);
            //Obtenemos memorias que responden al criterio pedido
            t_list *memoriasDelCriterioPedido = dictionary_get(parametros->memoriasConCriterios, criterio);
            //Que memorias tengo con el criterio X de la request solicitada?
            if (strcmp("SC", criterio) == 0) {
                int memoriaAsociadaASC = list_size(memoriasDelCriterioPedido);
                if (memoriaAsociadaASC == 1) {
                    int *fdMemoriaElegida = (int *) list_get(memoriasDelCriterioPedido, 0);
                    return *fdMemoriaElegida;
                } else {
                    log_error(logger, "No existe ninguna memoria asociada al criterio SC.");
                    return -1;
                }
            } else if (strcmp("SHC", criterio) == 0) {
                //FUNCION HASH
                int indice;
                int cantidadFDsAsociadosSHC = list_size(memoriasDelCriterioPedido);

                if (cantidadFDsAsociadosSHC > 0) {
                    if (key != NULL) {
                        indice = key % cantidadFDsAsociadosSHC;
                    } else {
                        long tiempo;
                        time_t tiempoActual;
                        tiempo = (long) time(&tiempoActual);

                        indice = tiempo % cantidadFDsAsociadosSHC;
                    }
                    int *fdMemoriaElegida = list_get(memoriasDelCriterioPedido, indice);

                    return *fdMemoriaElegida;
                }
            } else if (strcmp("EC", criterio) == 0) {
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
                    log_error(logger, "No existen memorias asociadas al criterio EC.");
                    return -1;
                }
            }
        } else {
            log_warning(logger, "No existen memorias conectadas para asignar requests.");
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
        default:;
    }
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
        t_archivoLQL *unLQL = (t_archivoLQL *) queue_pop(parametros_PLP->colaDeNew);
        sem_post(semaforo_colaDeNew);
        sem_wait(semaforo_colaDeReady);
        queue_push(parametros_PLP->colaDeReady, unLQL);
        sem_post(parametros_PLP->cantidadProcesosEnReady);
        sem_post(semaforo_colaDeReady);
    }
}

bool encolarDirectoNuevoPedido(t_comando requestParseada) {
    switch (requestParseada.tipoRequest) {
        case SELECT:
        case CREATE:
        case INSERT:
        case DROP:
        case JOURNAL:
        case DESCRIBE:
            return true;
        case ADD:
        case RUN:
        case METRICS: //definir si la encolamos o decidimos ejecutarla instantaneamente
        case EXIT:
            return false;
    }
}

t_archivoLQL *convertirRequestALQL(t_comando *requestParseada) {
    t_archivoLQL *unLQL = (t_archivoLQL *) malloc(sizeof(t_archivoLQL));

    unLQL->colaDeRequests = queue_create();
    unLQL->cantidadDeInsertProcesados = 0;
    unLQL->cantidadDeSelectProcesados = 0;
    unLQL->PID = 0;

    queue_push(unLQL->colaDeRequests, requestParseada);
    unLQL->cantidadDeLineas = queue_size(unLQL->colaDeRequests);

    return unLQL;
}

pthread_t *crearHiloPlanificadorCortoPlazo(p_planificacion *paramPlanificacionGeneral) {
    pthread_t *hiloPCP = (pthread_t *) malloc(sizeof(pthread_t));

    pthread_create(hiloPCP, NULL, &instanciarPCPs, paramPlanificacionGeneral);

    return hiloPCP;
}

void
planificarRequest(p_planificacion *paramPlanifGeneral, t_archivoLQL *archivoLQL, pthread_mutex_t *semaforoHilo) {
    p_consola_kernel *pConsolaKernel = paramPlanifGeneral->parametrosConsola;
    t_log *logger = paramPlanifGeneral->parametrosConsola->logger;
    int quantumMaximo = (int) paramPlanifGeneral->parametrosPCP->quantum;
    t_archivoLQL *unLQL = archivoLQL;
    bool requestEsValida = true;

    for (int quantumsConsumidos = 0; quantumsConsumidos < quantumMaximo; quantumsConsumidos++) {
        int lineaDeEjecucion = 0;//dejamos por si queremos hacer un lineaDeEjecución++
        t_comando *comando = (t_comando *) queue_pop(unLQL->colaDeRequests);
        if (comando == NULL)
            break;
        requestEsValida = analizarRequest(*comando, pConsolaKernel);
        if (requestEsValida) {
            if ((diferenciarRequest(*comando) == 1)) { //Si es 1 es primitiva
                gestionarRequestPrimitivas(*comando, paramPlanifGeneral, semaforoHilo);
                actualizarCantRequest(archivoLQL, *comando); //para métricas
            } else { //Si es 0 es comando de Kernel
                gestionarRequestKernel(*comando, paramPlanifGeneral);
            }
        } else {
            log_warning(logger,
                        "No se pudo procesar la request ubicada en la línea %s. Corríjala y vuelvala a ejecutar.",
                        lineaDeEjecucion);
            sem_wait(paramPlanifGeneral->parametrosPCP->mutexColaFinalizados);
            queue_push(paramPlanifGeneral->parametrosPCP->colaDeFinalizados, unLQL);
            sem_post(paramPlanifGeneral->parametrosPCP->mutexColaFinalizados);
            break;
        }
    }
    if (requestEsValida == true && queue_size(unLQL->colaDeRequests) > 0) {
        sem_wait(paramPlanifGeneral->parametrosPCP->mutexColaDeReady);
        queue_push(paramPlanifGeneral->parametrosPCP->colaDeReady, unLQL);
        sem_post(paramPlanifGeneral->parametrosPLP->cantidadProcesosEnReady);
        sem_post(paramPlanifGeneral->parametrosPCP->mutexColaDeReady);
    } else {
        sem_wait(paramPlanifGeneral->parametrosPCP->mutexColaFinalizados);
        queue_push(paramPlanifGeneral->parametrosPCP->colaDeFinalizados, unLQL);
        sem_post(paramPlanifGeneral->parametrosPCP->mutexColaFinalizados);
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

void instanciarPCPs(p_planificacion *paramPlanificacionGeneral) {
    parametros_pcp *parametrosPCP = paramPlanificacionGeneral->parametrosPCP;
    sem_t *semaforo_cantidadProcesosEnReady = parametrosPCP->cantidadProcesosEnReady;
    sem_t *semaforo_mutexColaDeReady = parametrosPCP->mutexColaDeReady;

    pthread_mutex_t *mutexJournal = parametrosPCP->mutexJournal;

    pthread_mutex_t *semaforoHilo = (pthread_mutex_t *) malloc(sizeof(pthread_mutex_t));
    int init = pthread_mutex_init(semaforoHilo, NULL);

    while (1) {
        sem_wait(semaforo_cantidadProcesosEnReady);
        sem_wait(semaforo_mutexColaDeReady);
        t_archivoLQL *nuevoLQL = (t_archivoLQL *) queue_pop(parametrosPCP->colaDeReady);
        sem_post(semaforo_mutexColaDeReady);

        planificarRequest(paramPlanificacionGeneral, nuevoLQL, semaforoHilo);
    }
}

int diferenciarRequest(t_comando requestParseada) {
    switch (requestParseada.tipoRequest) {
        case SELECT:
        case CREATE:
        case INSERT:
        case DROP:
        case JOURNAL:
        case DESCRIBE:
            return 1;
        case ADD:
        case RUN:
        case METRICS:
        case EXIT:
            return 0;
    }
}

//METRICAS

/*long tiempoHaceTreintaSegundos(){
    long tiempo;
    time_t tiempoActual;
    tiempo = (long) time(&tiempoActual);

    return tiempo - 30*10000000;
}
float convertirMicroSegundosASegundos(long microsegundos){
    return microsegundos/1000000;
}*/

/*
    Read Latency / 30s: El tiempo promedio que tarda un SELECT en ejecutarse en los últimos 30 segundos.
    Write Latency / 30s: El tiempo promedio que tarda un INSERT en ejecutarse en los últimos 30 segundos.
    Reads / 30s: Cantidad de SELECT ejecutados en los últimos 30 segundos.
    Writes / 30s: Cantidad de INSERT ejecutados en los últimos 30 segundos.
    Memory Load (por cada memoria):  Cantidad de INSERT / SELECT que se ejecutaron en esa memoria respecto de las operaciones totales.

 */


/*t_list* filtrarRequestUltimosTreintaSegundosSegunCriterio(t_list* listaRequestsDeAlgunCriterio, char* tipoRequest){
    long hacetreintasegundos = tiempoHaceTreintaSegundos();
    t_estadistica_request* nodoEstadistica;
    bool seEjecutoEnLosUltimosTreintaSegundos(void* elemento){
        if (elemento != NULL){
            nodoEstadistica = (t_estadistica_request*) elemento;
            return nodoEstadistica->inicioRequest > tiempoHaceTreintaSegundos && strcmp(nodoEstadistica->tipoRequest, tipoRequest);
        }else{
            return false;
        }

    };
    return list_filter(listaRequestsDeAlgunCriterio, seEjecutoEnLosUltimosTreintaSegundos);
}
int obtenerLatenciaSegunTipoDeRequest(t_list* listaRequestsDeAlgunCriterio, char* tipoRequest){

    if (!list_is_empty(listaRequestsDeAlgunCriterio)){

        long hacetreintasegundos = tiempoHaceTreintaSegundos();
        t_list* filtrados = filtrarRequestUltimosTreintaSegundosSegunCriterio(listaRequestsDeAlgunCriterio, tipoRequest);

        int latenciaTotal = 0;
        void sumarDuraciones(void* elemento){
            t_estadistica_request* nodoEstadistica;
            if (elemento != NULL){
                nodoEstadistica = (t_estadistica_request*) elemento;
                latenciaTotal+= nodoEstadistica->duracionEnSegundos;
            }
        }
        list_iterate(filtrados, sumarDuraciones);
        return latenciaTotal/list_size(filtrados);
    } else {
        return 0;
    }

*/