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
    t_log *logger = log_create("../kernel.log", "kernel", false, LOG_LEVEL_INFO);
    printf("Iniciando el proceso Kernel.\n");
    log_info(logger, "Iniciando el proceso Kernel");

    t_dictionary *supervisorDeHilos = dictionary_create();//Va a tener como KEY el PID, y la data el SEMÁFORO de c/hilo
    int contadorPIDs = 0;
    int memoriasUtilizables = 0;

    listaMetricasSC = list_create();
    listaMetricasSHC = list_create();
    listaMetricasEC = list_create();

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

    //INOTIFY
    t_datos_configuracion* datosConfiguracion = instanciarDatosConfiguracion(&configuracion);

    pthread_mutex_t *mutexDatosConfiguracion= malloc(sizeof(pthread_mutex_t));
    pthread_mutex_init(mutexDatosConfiguracion, NULL);

    mutexEstrucSupervisorHilos = (pthread_mutex_t*) malloc(sizeof(pthread_mutex_t));
    int initMutexSupHilos = pthread_mutex_init(mutexEstrucSupervisorHilos, NULL);

    mutexMetadataTablas = (pthread_mutex_t*) malloc(sizeof(pthread_mutex_t));
    int initMutexMetadata = pthread_mutex_init(mutexMetadataTablas, NULL);


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
    t_list* memoriasConocidas = list_create();

    //voy a tener relacion de criterio con un t_list* de FDs
    dictionary_put(tablaDeMemoriasConCriterios, "SC", listaDeCriteriosSC);
    dictionary_put(tablaDeMemoriasConCriterios, "SHC", listaDeCriteriosSHC);
    dictionary_put(tablaDeMemoriasConCriterios, "EC", listaDeCriteriosEC);

    t_queue *colaDeNew = queue_create();
    t_queue *colaDeReady = queue_create();
    t_queue *colaDeFinalizados = queue_create();

    GestorConexiones *misConexiones = inicializarConexion();
    int fdMemoriaPrincipal = conectarseAMemoriaPrincipal(configuracion.ipMemoria, configuracion.puertoMemoria, misConexiones, logger);
    agregarIpMemoria(configuracion.ipMemoria, configuracion.puertoMemoria, memoriasConocidas, logger);
    t_nodoMemoria *nodoMemoriaPrincipal = list_get(memoriasConocidas, 0);
    nodoMemoriaPrincipal->fdNodoMemoria = fdMemoriaPrincipal;

    pthread_t *hiloRespuestas = crearHiloConexiones(misConexiones, logger, tablaDeMemoriasConCriterios, metadataTablas, mutexJournal, supervisorDeHilos, memoriasConocidas, mutexColaDeNew, colaDeNew, cantidadProcesosEnNew, datosConfiguracion, mutexDatosConfiguracion);

//    refreshMetadata(configuracion.refreshMetadata, metadataTablas, logger);

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

    pthread_t*  hiloMonitor = (pthread_t*)crearHiloMonitor(configuracion.directorioAMonitorear, "kernel.cfg", logger, datosConfiguracion, mutexDatosConfiguracion, memoriasConocidas);
    pthread_t *hiloPlanificadorLargoPlazo = crearHiloPlanificadorLargoPlazo(parametrosPLP);
    pthread_t *hiloGossiping = (pthread_t *) crearHiloGossiping(misConexiones, memoriasConocidas, logger);

    parametros_pcp *parametrosPCP = (parametros_pcp *) malloc(sizeof(parametros_pcp));

    parametrosPCP->quantum = (int *) configuracion.quantum;
    parametrosPCP->colaDeReady = colaDeReady;
    parametrosPCP->mutexColaDeReady = mutexColaDeReady;
    parametrosPCP->mutexColaFinalizados = mutexListaFinalizados;
    parametrosPCP->cantidadProcesosEnReady = cantidadProcesosEnReady;
    parametrosPCP->colaDeFinalizados = colaDeFinalizados;
    parametrosPCP->retardoEjecucion = (int *) configuracion.retardoEjecucion;
    parametrosPCP->mutexJournal = mutexJournal;
    parametrosPCP->logger = logger;

    paramPlanificacionGeneral = (p_planificacion *) malloc(sizeof(p_planificacion));
    paramPlanificacionGeneral->parametrosConsola = pConsolaKernel;
    paramPlanificacionGeneral->parametrosPCP = parametrosPCP;
    paramPlanificacionGeneral->parametrosPLP = parametrosPLP;
    paramPlanificacionGeneral->supervisorDeHilos = supervisorDeHilos;
    paramPlanificacionGeneral->memoriasUtilizables = memoriasUtilizables;
    paramPlanificacionGeneral->memoriasConocidas = memoriasConocidas;

    pthread_t *hiloMetricas = crearHiloMetricas(paramPlanificacionGeneral);

    for (int i = 0; i < configuracion.multiprocesamiento; i++) {
//        paramPlanificacionGeneral->parametrosPCP->mutexSemaforoHilo = semaforoHilo;
        crearHiloPlanificadorCortoPlazo(paramPlanificacionGeneral);
    }

    ejecutarConsola(pConsolaKernel, configuracion, paramPlanificacionGeneral);

    pthread_join(*hiloRespuestas, NULL);
    pthread_join(*hiloGossiping, NULL);
    pthread_join(*hiloPlanificadorLargoPlazo, NULL);
    pthread_join(*hiloMonitor, NULL);

    free(pConsolaKernel);
    free(parametrosPCP);
    free(parametrosPLP);
    free(paramPlanificacionGeneral);
    return EXIT_SUCCESS;
}

/****************************
 *** COMPORTAMIENTO KERNEL***
 ****************************/

void *metricas(void *pMetricas) {
    parametrosMetricas *parametros = (parametrosMetricas *) pMetricas;
    p_planificacion *paramPlanificacionGeneral = parametros->paramPlanificacionGeneral;
    while (1) {
        sleep(30);
        calcularMetricas(false, paramPlanificacionGeneral);
    }
}

pthread_t *crearHiloMetricas(p_planificacion *paramPlanificacionGeneral) {
    pthread_t *hiloMetricas = (pthread_t *) malloc(sizeof(pthread_t));
    parametrosMetricas *parametros = (parametrosMetricas *) malloc(sizeof(parametrosMetricas));
    parametros->paramPlanificacionGeneral = paramPlanificacionGeneral;
    pthread_create(hiloMetricas, NULL, &metricas, parametros);

    return hiloMetricas;
}

void actualizarEstructurasMetricas(int fdMemoria, t_list *listadoDeCriterio, TipoRequest tipoRequest,
                                   estadisticasRequest *estadisticasRequest) {

    double tiempoInicioRequest = estadisticasRequest->inicioRequest;
    time_t tiempoFinRequest = clock();
    double tiempoTotal = (double)(tiempoFinRequest- tiempoInicioRequest)/CLOCKS_PER_SEC;
    estadisticasRequest->fdMemoria = fdMemoria;
    estadisticasRequest->tipoRequest = tipoRequest;
    estadisticasRequest->duracionEnSegundos = tiempoTotal;
    list_add(listadoDeCriterio, estadisticasRequest);
}

/*
    Read Latency / 30s: El tiempo promedio que tarda un SELECT en ejecutarse en los últimos 30 segundos.
    Write Latency / 30s: El tiempo promedio que tarda un INSERT en ejecutarse en los últimos 30 segundos.
    Reads / 30s: Cantidad de SELECT ejecutados en los últimos 30 segundos.
    Writes / 30s: Cantidad de INSERT ejecutados en los últimos 30 segundos.
    Memory Load (por cada memoria):  Cantidad de INSERT / SELECT que se ejecutaron en esa memoria respecto de las operaciones totales.

 */
double tiempoHaceTreintaSegundos() {
    time_t tiempo;
    time(&tiempo);
    return (double)tiempo - 30;
}

t_metricasDefinidas *calcularMetricaParaCriterio(char *criterio, long tiempoHace30s, int fd) {
    t_metricasDefinidas *metricas = (t_metricasDefinidas *) malloc(sizeof(t_metricasDefinidas));
    metricas->criterio = criterio;
    t_list *listaCriterio = getListaMetricasPorCriterio(criterio);
    if (fd != NULL) {
        void forFd(estadisticasRequest *elem) {
            return elem->fdMemoria == fd;
        }
        listaCriterio = list_filter(listaCriterio, forFd);
    }
    bool filtrarUltimos30s(estadisticasRequest *elemento) {
        return elemento->finRequest >= tiempoHace30s;
    }
    t_list *listaCriterio30 = list_filter(listaCriterio, filtrarUltimos30s);
    bool esSelect(estadisticasRequest *elemento) {
        return elemento->tipoRequest == SELECT;
    }
    t_list *listaSelect = list_filter(listaCriterio30, esSelect);
    bool esInsert(estadisticasRequest *elemento) {
        return elemento->tipoRequest == INSERT;
    }
    t_list *listaInsert = list_filter(listaCriterio30, esInsert);
    // SELECTS
    metricas->reads = list_count_satisfying(listaCriterio30,esSelect);
    // INSERTS
    metricas->writes = list_count_satisfying(listaCriterio30,esInsert);

    // TIEMPO PROMEDIO DE CADA SELECT/INSERT
    double getDuracion(estadisticasRequest *elemento) {
        return elemento->duracionEnSegundos;
    }
    t_list *duracionesSelect = list_map(listaSelect, getDuracion);
    t_list *duracionesInsert = list_map(listaSelect, getDuracion);
    double duracionTotalSelect = 0;
    double duracionTotalInsert = 0;
    void sumarDuracionSelect(double duracion) {
        duracionTotalSelect += duracion;
    }
    void sumarDuracionInsert(double duracion) {
        duracionTotalInsert += duracion;
    }
    if(!list_is_empty(duracionesSelect)){
        list_iterate(duracionesSelect, sumarDuracionSelect);
        metricas->readLatency = duracionTotalSelect / list_size(duracionesSelect);
    }else{
        metricas->readLatency = 0;
    }
    if(!list_is_empty(duracionesInsert)) {
        list_iterate(duracionesInsert, sumarDuracionInsert);
        metricas->writeLatency = duracionTotalInsert / list_size(duracionesInsert);
    }else{
        metricas->writeLatency = 0;
    }
    return metricas;
}

void calcularMetricas(bool mostrarPorPantalla, p_planificacion *paramPlanifGeneral) {

    t_log *logger = paramPlanifGeneral->parametrosPLP->logger;
    int cantidadSelect, cantidadInsert;
    double tiempoTotalInserts, tiempoTotalSelects;

    GestorConexiones *misConexiones = paramPlanificacionGeneral->parametrosConsola->conexiones;
    t_list *listaDeMemoriasConectadas = misConexiones->conexiones;
    int tamanioListaMemoriasConectadas = list_size(listaDeMemoriasConectadas);


    double tiempoHace30Segundos = tiempoHaceTreintaSegundos();

    t_metricasDefinidas *metricasSC = calcularMetricaParaCriterio("SC", tiempoHace30Segundos, NULL);
    t_metricasDefinidas *metricasSHC = calcularMetricaParaCriterio("SHC", tiempoHace30Segundos, NULL);
    t_metricasDefinidas *metricasEC = calcularMetricaParaCriterio("EC", tiempoHace30Segundos, NULL);
    if (mostrarPorPantalla) {
        printf("METRICAS:\n----- TOTALES -----\n");
    }
    log_info(logger, "METRICAS:\n----- TOTALES -----");

    mostrarMetricas(metricasSC, metricasSHC, metricasEC, mostrarPorPantalla, logger);

    char *fdCasteado;

    for (int i = 0; i < tamanioListaMemoriasConectadas; i++) {

        fdCasteado = (int *) list_get(listaDeMemoriasConectadas, i);
        t_metricasDefinidas *metricasMPorCriterioSC = calcularMetricaParaCriterio("SC", tiempoHace30Segundos,*fdCasteado);
        t_metricasDefinidas *metricasMPorCriterioSHC = calcularMetricaParaCriterio("SHC", tiempoHace30Segundos,*fdCasteado);
        t_metricasDefinidas *metricasMPorCriterioEC = calcularMetricaParaCriterio("EC", tiempoHace30Segundos,*fdCasteado);
        if (mostrarPorPantalla) {
            printf("----- Memoria fd: %s -----\n", string_itoa(*fdCasteado));
        }
        log_info(logger, "----- Memoria fd: %s -----", string_itoa(*fdCasteado));
        mostrarMetricas(metricasMPorCriterioSC, metricasMPorCriterioSHC, metricasMPorCriterioEC, mostrarPorPantalla,
                        logger);
    }
}


void mostrarMetricas(t_metricasDefinidas *metricasSC, t_metricasDefinidas *metricasSHC, t_metricasDefinidas *metricasEC,
                     bool mostrarPorPantalla, t_log *logger) {
    int reads = (metricasSC->reads + metricasSHC->reads + metricasEC->reads);
    int writes = (metricasSC->writes + metricasSHC->writes + metricasEC->writes);
    int readLatency = (metricasSC->readLatency + metricasSHC->readLatency + metricasEC->readLatency) / 3;
    int writeLatency = (metricasSC->writeLatency + metricasSHC->writeLatency + metricasEC->writeLatency) / 3;
    if (mostrarPorPantalla) {
        printf("Read Latency: %d.\t Write Latency: %d.\t Reads: %i.\t Writes: %i.\n", readLatency, writeLatency, reads,
               writes);
    }
    log_info(logger, "Read Latency: %d.\t Write Latency: %d.\t Reads: %i.\t Writes: %i.\n", readLatency, writeLatency,
             reads, writes);
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
                "SLEEP_EJECUCION",
                "DIRECTORIO_CONFIGURACION"
        };

        for (int i = 0; i < COUNT_OF(clavesObligatorias); i++) {
            if (!config_has_property(archivoConfig, clavesObligatorias[i]))
                return false;
        }
        return true;
    }

    if (!existenTodasLasClavesObligatorias(archivoConfig, configuracion)) {
        printf(COLOR_ERROR "Alguna de las claves obligatorias no están setteadas en el archivo de configuración.\n" COLOR_RESET);
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
        free(comandoParseado);
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
        } else if (requestParseada->tipoRequest == EXIT) {
            break;
        } else {
            printf(COLOR_ERROR "No se pudo procesar la request solicitada.\n" COLOR_RESET);
            log_error(parametros->logger, "No se pudo procesar la request solicitada.");
            }
        //free(leido);
    } while (requestParseada->tipoRequest != EXIT);
    printf(COLOR_EXITO "Ya analizamos todo lo solicitado.\n" COLOR_RESET);
    exit(-1);
}

bool analizarRequest(t_comando requestParseada, p_consola_kernel *parametros) {
    t_log *logger = parametros->logger;

    if (requestParseada.tipoRequest == INVALIDO) {
        printf(COLOR_ERROR "Comando inválido.\n" COLOR_RESET);
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

t_list *getListaMetricasPorCriterio(char *criterio) {
    if (strcmp(criterio, "SC")) {
        return listaMetricasSC;
    }
    if (strcmp(criterio, "SHC")) {
        return listaMetricasSHC;
    }
    if (strcmp(criterio, "EC")) {
        return listaMetricasEC;
    }
}

int gestionarRequestPrimitivas(t_comando requestParseada, p_planificacion *paramPlanifGeneral,
                               pthread_mutex_t *mutexDeHiloRequest, estadisticasRequest *estadisticasRequest,
                               sem_t *semConcurrenciaMetricas, int PID) {
    char *PIDCasteado = string_itoa(PID);
    p_consola_kernel *pConsolaKernel = paramPlanifGeneral->parametrosConsola;
    char *criterioConsistencia;
    int fdMemoria;
    t_log *logger = pConsolaKernel->logger;

    if (paramPlanifGeneral->memoriasUtilizables > 0) {

        pthread_mutex_lock(mutexEstrucSupervisorHilos);
        dictionary_put(paramPlanifGeneral->supervisorDeHilos, PIDCasteado, mutexDeHiloRequest);
        pthread_mutex_lock(mutexDeHiloRequest);
        pthread_mutex_unlock(mutexEstrucSupervisorHilos);

        switch (requestParseada.tipoRequest) { //Analizar si cada gestionar va a tener que encolar en NEW, en lugar de enviarPaquete
            case SELECT:
                if (dictionary_has_key(pConsolaKernel->metadataTablas, requestParseada.parametros[0])) {
                    criterioConsistencia = criterioBuscado(requestParseada, pConsolaKernel->metadataTablas);
                    int key = atoi(requestParseada.parametros[1]);
                    fdMemoria = seleccionarMemoriaIndicada(pConsolaKernel, criterioConsistencia, key);
                    int respuesta = gestionarSelectKernel(requestParseada.parametros[0], requestParseada.parametros[1],
                                                          fdMemoria, PID, estadisticasRequest);
                    t_list *listaCriterio = getListaMetricasPorCriterio(criterioConsistencia);
                    actualizarEstructurasMetricas(fdMemoria, listaCriterio, requestParseada.tipoRequest,
                                                  estadisticasRequest);
                    return respuesta;
                } else {
                    printf(COLOR_ERROR "La tabla no se encuentra dentro de la Metadata conocida.\n" COLOR_RESET);
                    log_error(logger, "La tabla no se encuentra dentro de la Metadata conocida.");
                    return -1;
                }
            case INSERT:
                if (dictionary_has_key(pConsolaKernel->metadataTablas, requestParseada.parametros[0])) {
                    criterioConsistencia = criterioBuscado(requestParseada, pConsolaKernel->metadataTablas);
                    if (criterioConsistencia != NULL) {
                        int key = atoi(requestParseada.parametros[2]);
                        fdMemoria = seleccionarMemoriaIndicada(pConsolaKernel, criterioConsistencia, key);
                        int respuesta = gestionarInsertKernel(requestParseada.parametros[0],
                                                              requestParseada.parametros[1],
                                                              requestParseada.parametros[2], fdMemoria, PID,
                                                              estadisticasRequest);
                        t_list *listaCriterio = getListaMetricasPorCriterio(criterioConsistencia);
                        actualizarEstructurasMetricas(fdMemoria, listaCriterio, requestParseada.tipoRequest,
                                                      estadisticasRequest);
                        return respuesta;
                    } else {
                        printf(COLOR_ERROR "El criterio es nulo, no se puede analizar.\n" COLOR_RESET);
                        log_error(logger, "El criterio es nulo, no se puede analizar.");
                        return -1;
                    }
                } else {
                    printf(COLOR_ERROR "La tabla no se encuentra dentro de la Metadata conocida.\n" COLOR_RESET);
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
                    printf(COLOR_ERROR "El criterio es nulo, no se puede analizar.\n" COLOR_RESET);
                    log_error(logger, "El criterio es nulo, no se puede analizar.");
                    return -1;
                }
            case DROP:
                if (dictionary_has_key(pConsolaKernel->metadataTablas, requestParseada.parametros[0])) {
                    criterioConsistencia = criterioBuscado(requestParseada, pConsolaKernel->metadataTablas);
                    fdMemoria = seleccionarMemoriaIndicada(pConsolaKernel, criterioConsistencia, NULL);
                    return gestionarDropKernel(requestParseada.parametros[0], fdMemoria, PID);
                } else {
                    printf(COLOR_ERROR "La tabla no se encuentra dentro de la Metadata conocida.\n" COLOR_RESET);
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
                        printf(COLOR_ERROR "La tabla no se encuentra dentro de la Metadata conocida.\n" COLOR_RESET);
                        log_error(logger, "La tabla no se encuentra dentro de la Metadata conocida.");
                        return -1;
                    }
                } else {
                    fdMemoria = seleccionarMemoriaParaDescribe(pConsolaKernel);
                    return gestionarDescribeGlobalKernel(fdMemoria, PID);
                }
            case JOURNAL:
                return gestionarJournalKernel(paramPlanifGeneral);
        } //fin del switch
    } else {
        errorNoHayMemoriasAsociadas(logger);
        return -1;
    }
}

void errorNoHayMemoriasAsociadas(t_log *logger) {
    printf(COLOR_ERROR "No hay memorias utilizables, asocie memorias previamente.\n" COLOR_RESET);
    log_error(logger, "No hay memorias utilizables, asocie memorias previamente.");
}

int gestionarRequestKernel(t_comando requestParseada, p_planificacion *paramPlanifGeneral) {

    p_consola_kernel *pConsolaKernel = paramPlanifGeneral->parametrosConsola;
    parametros_plp *parametrosPLP = paramPlanifGeneral->parametrosPLP;

    switch (requestParseada.tipoRequest) {
        case ADD:
            return gestionarAdd(requestParseada.parametros, paramPlanifGeneral);
        case RUN:
            if (paramPlanifGeneral->memoriasUtilizables > 0) {
                gestionarRun(requestParseada.parametros[0], pConsolaKernel, parametrosPLP);
            } else {
                errorNoHayMemoriasAsociadas(pConsolaKernel->logger);
            }
            break;
        case METRICS:
            calcularMetricas(true, paramPlanifGeneral);
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
            return printf(COLOR_ERROR "Comando inválido.\n" COLOR_RESET);
    }
}

bool validarComandosKernel(t_comando comando, t_log *logger) {
    bool esValido = esComandoValidoDeKernel(comando);
    return esValido;
}

bool esComandoValidoDeKernel(t_comando comando) {
    switch (comando.tipoRequest) {
        case ADD:
            if (comando.cantidadParametros == 4) {
                if (esString(comando.parametros[0]) && esString(comando.parametros[2]) &&
                    esString(comando.parametros[3]) && esEntero(comando.parametros[1])) {
                    string_to_upper(comando.parametros[0]);
                    string_to_upper(comando.parametros[2]);
                    if ((strcmp(comando.parametros[0], "MEMORY") == 0) && (strcmp(comando.parametros[2], "TO") == 0))
                        return true;
                }
                return false;
            } else return false;
        case RUN:
            return (comando.cantidadParametros == 1 && esString(comando.parametros[0]));//siempre recibe PATH
        case METRICS:
            return (comando.cantidadParametros == 0);
        default:
            return false;
    }
}

int
gestionarSelectKernel(char *nombreTabla, char *key, int fdMemoria, int PID, estadisticasRequest *estadisticasRequest) {
    char *request = string_from_format("SELECT %s %s", nombreTabla, key);
    enviarPaquete(fdMemoria, REQUEST, SELECT, request, PID);
    free(request);
    return 0;
}

int gestionarCreateKernel(char *tabla, char *consistencia, char *cantParticiones, char *tiempoCompactacion,
                          int fdMemoria, int PID) {
    char *request = string_from_format("CREATE %s %s %s %s", tabla, consistencia, cantParticiones, tiempoCompactacion);
    enviarPaquete(fdMemoria, REQUEST, CREATE, request, PID);
    free(request);
    return 0;
}

int gestionarInsertKernel(char *nombreTabla, char *key, char *valor, int fdMemoria, int PID,
                          estadisticasRequest *estadisticasRequest) {
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
    unLQL->PID = parametrosPLP->contadorPID;
    parametrosPLP->contadorPID++;
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
            printf(COLOR_ERROR "El archivo solicitado es incorrecto. Verifique su entrada.\n" COLOR_RESET);
            log_error(logger, "El archivo solicitado es incorrecto. Verifique su entrada.");
        }
    } else {
        printf(COLOR_ERROR "La extensión del archivo no parece ser la indicada. Verifique ingresar un archivo \".lql\".\n" COLOR_RESET);
        log_error(logger, "La extensión del archivo no parece ser la indicada. Verifique ingresar un archivo .LQL");
    }
}

int gestionarAdd(char **parametrosDeRequest, p_planificacion *paramPlanificacionGeneral) {

    p_consola_kernel *pConsolaKernel = paramPlanificacionGeneral->parametrosConsola;
    t_log *logger = pConsolaKernel->logger;
    t_list* memoriasConocidas = (t_list*)paramPlanificacionGeneral->memoriasConocidas;

    int numeroMemoria = atoi(parametrosDeRequest[1]);
    char *criterio = parametrosDeRequest[3];
    string_to_upper(criterio);
    GestorConexiones *misConexiones = pConsolaKernel->conexiones;
    t_dictionary *tablaMemoriasConCriterios = pConsolaKernel->memoriasConCriterios;
    t_nodoMemoria* unNodoMemoria = malloc(sizeof(t_nodoMemoria));

    bool haySuficientesMemorias = (list_size(misConexiones->conexiones) > 0 && (numeroMemoria > 0));

    int* fdMemoriaSolicitada = NULL;

    void esMemoriaBuscada(void* elemento){
        if (elemento != NULL){
            unNodoMemoria = (t_nodoMemoria*)elemento;
            if (numeroMemoria == unNodoMemoria->memoryNumber)
            {
                fdMemoriaSolicitada = &(unNodoMemoria->fdNodoMemoria);
                //free(unNodoMemoria);
            }

        }
    }
    list_iterate(memoriasConocidas, esMemoriaBuscada);

    if (haySuficientesMemorias && fdMemoriaSolicitada != NULL) {

        //BUSCO LA MEMORIA CORRESPONDIENTE A LA POSICION DESEADA
        //int *fdMemoriaSolicitada = (int *) list_get(misConexiones->conexiones, numeroMemoria - 1);
        // hacemos -1 por la ubicación 0

        if ((strcmp("SC", criterio) == 0) || (strcmp("SHC", criterio) == 0) || (strcmp("EC", criterio) == 0)) {

            t_list *listaFileDescriptors = dictionary_get(tablaMemoriasConCriterios, criterio);

            if (strcmp("SC", criterio) == 0) {
                int cantMemoriasSC = list_size(listaFileDescriptors);
                if (cantMemoriasSC >= 1) {
                    printf(COLOR_ERROR "Ya existe una memoria asignada al criterio SC.\n" COLOR_RESET);
                    log_error(logger, "Ya existe una memoria asignada al criterio SC.");
                    return -1;
                } else {
                    list_add(listaFileDescriptors, fdMemoriaSolicitada);
                    imprimirMensajeAdd(numeroMemoria, criterio);
                    paramPlanificacionGeneral->memoriasUtilizables++;
                    return 0;
                }
            } else if (strcmp("SHC", criterio) == 0 || strcmp("EC", criterio) == 0) {

                int *fdParaComparar;
                bool sonMismoFileDescriptor(void *elemento) {
                    if (elemento != NULL) {
                        fdParaComparar = (int *) elemento;
                        return (fdParaComparar == fdMemoriaSolicitada);
                    } else {
                        return false;
                    }
                }
                if (!list_any_satisfy(listaFileDescriptors, sonMismoFileDescriptor)) {
                    list_add(listaFileDescriptors, fdMemoriaSolicitada);
                    imprimirMensajeAdd(numeroMemoria, criterio);
                    paramPlanificacionGeneral->memoriasUtilizables++;
                    return 0;
                } else {
                    return -1;
                }
            }
        } else {
            printf(COLOR_ERROR "El criterio escrito no está dentro de los avalados.\n" COLOR_RESET);
            log_error(logger, "El criterio escrito no está dentro de los avalados.");
            return -1;
        }
    } else {
        printf(COLOR_ADVERT "La memoria no se encuentra dentro de las posibles a utilizar.\n" COLOR_RESET);
        log_warning(logger, "La memoria no se encuentra dentro de las posibles a utilizar.");
        return -1;
    }
}

void imprimirMensajeAdd(int numeroMemoria, char *criterio) {
    printf(COLOR_EXITO "Tipo de Request: %s %i \n" COLOR_RESET, "ADD MEMORY", numeroMemoria);
    printf(COLOR_EXITO "To: %s\n" COLOR_RESET, criterio);
}

int seleccionarMemoriaParaDescribe(p_consola_kernel *parametros) {
    GestorConexiones *misConexiones = parametros->conexiones;
    int *fdMemoria;

    int tamanioListaConexiones = list_size(parametros->conexiones->conexiones);

    int numeroMemoriaElegida = rand() % (tamanioListaConexiones); //me devuelve un numero entre 0 y tamListaConex
    fdMemoria = (int *) list_get(misConexiones->conexiones, numeroMemoriaElegida);

    return *fdMemoria;
}

int seleccionarMemoriaIndicada(p_consola_kernel *parametros, char *criterio, int key) {
    t_log *logger = parametros->logger;
    if (criterio != NULL) {
        if (existenMemoriasConectadas) {
            string_to_upper(criterio); //POSIBLE CONDICION DE CARRERA
            //Obtenemos memorias que responden al criterio pedido
            t_list *memoriasDelCriterioPedido = dictionary_get(parametros->memoriasConCriterios, criterio);
            //Que memorias tengo con el criterio X de la request solicitada?
            if (strcmp("SC", criterio) == 0) {
                int memoriaAsociadaASC = list_size(memoriasDelCriterioPedido);
                if (memoriaAsociadaASC == 1) {
                    int *fdMemoriaElegida = (int *) list_get(memoriasDelCriterioPedido, 0);
                    return *fdMemoriaElegida;
                } else {
                    printf(COLOR_ADVERT "No existe ninguna memoria asociada al criterio SC.\n" COLOR_RESET);
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
                    //int elementoBuscado = (cantidadFDsAsociadosEC -(cantidadFDsAsociadosEC - 1));//Siempre el primero -ya se que tiene poco sentido-
                    //int elementoBuscado = 1;
                    t_list *primeraMemoriaDeLaLista = list_take_and_remove(memoriasDelCriterioPedido, 1); //Lista nueva
                    int *fdMemoriaElegida = (int *) list_get(primeraMemoriaDeLaLista, 0);
                    list_add(memoriasDelCriterioPedido, fdMemoriaElegida);

                    list_destroy(primeraMemoriaDeLaLista);
                    return *fdMemoriaElegida;
                } else {
                    printf(COLOR_ADVERT "No existe ninguna memoria asociada al criterio EC.\n" COLOR_RESET);
                    log_error(logger, "No existen memorias asociadas al criterio EC.");
                    return -1;
                }
            }
        } else {
            printf(COLOR_ADVERT "No existen memorias conectadas para asignar requests.\n" COLOR_RESET);
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
        case CREATE:
            criterioPedido = requestParseada.parametros[1];
            return criterioPedido;
        case SELECT:
        case INSERT:
        case DROP:
            tabla = requestParseada.parametros[0];
            pthread_mutex_lock(mutexMetadataTablas);
            criterioPedido = (char *) dictionary_get(metadataTablas, tabla);//buscar en metadataTablasConocidas
            pthread_mutex_unlock(mutexMetadataTablas);
            return criterioPedido;
        case DESCRIBE:
            if (requestParseada.cantidadParametros > 0) {
                tabla = requestParseada.parametros[0];
                pthread_mutex_lock(mutexMetadataTablas);
                criterioPedido = (char *) dictionary_get(metadataTablas, tabla);
                pthread_mutex_unlock(mutexMetadataTablas);
                return criterioPedido;
            }
        case ADD:
            tabla = requestParseada.parametros[3];
            pthread_mutex_lock(mutexMetadataTablas);
            criterioPedido = (char *) dictionary_get(metadataTablas, tabla);
            pthread_mutex_unlock(mutexMetadataTablas);
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

void planificarRequest(p_planificacion *paramPlanifGeneral, t_archivoLQL *archivoLQL, pthread_mutex_t *semaforoHilo) {
    estadisticasRequest *infoRequest = (estadisticasRequest *) malloc(sizeof(estadisticasRequest));
    sem_t *semConcurrenciaMetricas = (sem_t *) malloc(sizeof(sem_t));
    sem_init(semConcurrenciaMetricas, 0, 1);
    p_consola_kernel *pConsolaKernel = paramPlanifGeneral->parametrosConsola;
    t_log *logger = paramPlanifGeneral->parametrosConsola->logger;
    int quantumMaximo = (int) paramPlanifGeneral->parametrosPCP->quantum;
    time_t tiempoInicioRequest;
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
                infoRequest->inicioRequest=clock();
                time(&infoRequest->instanteInicio);
                gestionarRequestPrimitivas(*comando, paramPlanifGeneral, semaforoHilo, infoRequest,
                                           semConcurrenciaMetricas, unLQL->PID);
            } else { //Si es 0 es comando de Kernel
                gestionarRequestKernel(*comando, paramPlanifGeneral);
            }
        } else {
            log_warning(logger,
                        "No se pudo procesar la request ubicada en la línea %s. Corríjala y vuelvala a ejecutar.",
                        lineaDeEjecucion);
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

/*
    Read Latency / 30s: El tiempo promedio que tarda un SELECT en ejecutarse en los últimos 30 segundos.
    Write Latency / 30s: El tiempo promedio que tarda un INSERT en ejecutarse en los últimos 30 segundos.
    Reads / 30s: Cantidad de SELECT ejecutados en los últimos 30 segundos.
    Writes / 30s: Cantidad de INSERT ejecutados en los últimos 30 segundos.
    Memory Load (por cada memoria):  Cantidad de INSERT / SELECT que se ejecutaron en esa memoria respecto de las operaciones totales.

 */


//GOSSIPING

void conectarseANuevasMemorias(t_list* memoriasConocidas, GestorConexiones* misConexiones, t_log* logger){

    t_nodoMemoria* unNodoMemoria;
    int fdNuevo;
    void conectarseANodoMemoria(void* elemento){

        if (elemento != NULL){
            unNodoMemoria = (t_nodoMemoria*) elemento;
            if (unNodoMemoria->fdNodoMemoria == 0){
                fdNuevo = conectarseANuevoNodoMemoria(unNodoMemoria->ipNodoMemoria, unNodoMemoria->puertoNodoMemoria, misConexiones, logger);
                if(fdNuevo > 0 && fdNuevo != NULL){
                    unNodoMemoria->fdNodoMemoria = fdNuevo;
                }
            }
        }

    }
    list_iterate(memoriasConocidas, conectarseANodoMemoria);
}
void gossiping(parametros_gossiping* parametros){

    //Solo le pide su lista de gossiping a la primera memoria agregada que corresponde con la memoria princiapal
    GestorConexiones* misConexiones = (GestorConexiones*) parametros->misConexiones;
    t_list* memoriasConocidas = (t_list*) parametros->memoriasConocidas;
    t_log* logger = (t_log*) parametros->logger;
    t_nodoMemoria* nodoMemoriaPrincipal = list_get(memoriasConocidas, 0);

    int i = 0;
    while (1){

        sleep(200); //corregir para que se pueda ingresar por archivo configuracion
        enviarPaquete(nodoMemoriaPrincipal->fdNodoMemoria, GOSSIPING, INVALIDO, "DAME_LISTA_GOSSIPING", -1);
        conectarseANuevasMemorias(memoriasConocidas, misConexiones, logger);
        i++;

    }
}
pthread_t * crearHiloGossiping(GestorConexiones* misConexiones , t_list* memoriasConocidas, t_log* logger){
    pthread_t* hiloGossiping = malloc(sizeof(pthread_t));
    parametros_gossiping* parametros = (parametros_gossiping*) malloc(sizeof(parametros_gossiping));

    parametros->logger = logger;
    parametros->memoriasConocidas = memoriasConocidas;
    parametros->misConexiones = misConexiones;

    pthread_create(hiloGossiping, NULL, &gossiping, parametros);
    return hiloGossiping;

}

//INOTIFY
t_datos_configuracion* instanciarDatosConfiguracion(t_configuracion* configuracion){
    t_datos_configuracion* datosConfiguracion = (t_datos_configuracion*) malloc(sizeof(t_datos_configuracion));

    datosConfiguracion->retardoEjecucion = configuracion->retardoEjecucion;
    datosConfiguracion->refreshMetadata = configuracion->refreshMetadata;
    datosConfiguracion->Quantum = configuracion->quantum;
    datosConfiguracion->nivelDeMultiProcesamiento = configuracion->multiprocesamiento;

    return datosConfiguracion;
}

void atenderInotify(parametros_hilo_monitor* parametros){

    char* nombreDirectorio = (char*) parametros->directorioAMonitorear;
    char* nombreArchivoDeConfiguracion = (char*) parametros->nombreArchivoDeConfiguracion;
    t_log* logger = (t_log*) parametros->logger;
    t_datos_configuracion* datosConfiguracion =(t_datos_configuracion*) parametros->datosConfiguracion;
    pthread_mutex_t* mutexDatosConfiguracion = (pthread_mutex_t*) parametros->mutexDatosConfiguracion;
    t_list* memoriasConocidas = (t_list*) parametros->memoriasConocidas;

    log_info(logger, "Inicio el hilo que atiende inofity");
    char buffer[BUF_LEN];

    int file_descriptor = inotify_init();
    if (file_descriptor < 0) {
        perror("inotify_init");
    }

    // Creamos un monitor sobre un path indicando que eventos queremos escuchar
    int watch_descriptor = inotify_add_watch(file_descriptor, "/home/utnso/operativos/tp-2019-1c-Suck-et/kernel/", IN_MODIFY);

    // El file descriptor creado por inotify, es el que recibe la información sobre los eventos ocurridos
    // para leer esta información el descriptor se lee como si fuera un archivo comun y corriente pero
    // la diferencia esta en que lo que leemos no es el contenido de un archivo sino la información
    // referente a los eventos ocurridos
    int offset = 0;
    int length = read(file_descriptor, buffer, BUF_LEN);
    if (length < 0) {
        perror("read");
    }
    // Luego del read buffer es un array de n posiciones donde cada posición contiene
    // un eventos ( inotify_event ) junto con el nombre de este.
    while (offset < length) {

        // El buffer es de tipo array de char, o array de bytes. Esto es porque como los
        // nombres pueden tener nombres mas cortos que 24 caracteres el tamaño va a ser menor
        // a sizeof( struct inotify_event ) + 24.
        struct inotify_event *event = (struct inotify_event *) &buffer[offset];

        // El campo "len" nos indica la longitud del tamaño del nombre
        if (event->len) {
            // Dentro de "mask" tenemos el evento que ocurrio y sobre donde ocurrio
            // sea un archivo o un directorio
            if (event->mask & IN_CREATE) {
                if (event->mask & IN_ISDIR) {
                    printf("Se creo el directorio %s .\n", event->name);
                } else {
                    //printf("Se creó el archivo%s.\n", event->name);
                    //printf("Se modificó el archivo%s.\n", event->name);
                    pthread_mutex_lock(mutexDatosConfiguracion);
                    //log_info(logger, string_from_format("Quantum anterior: %i. Nivel de MP anterior: %i. Metadata-refresh anterior: %i. Sleep ejecucion anterior: %i", datosConfiguracion->Quantum, datosConfiguracion->nivelDeMultiProcesamiento, datosConfiguracion->refreshMetadata, datosConfiguracion->retardoEjecucion));
                    t_configuracion nuevaConfiguracion = cargarConfiguracion(nombreArchivoDeConfiguracion, logger);

                    //Si cambió el nivel de multiprocesamiento
                    if (nuevaConfiguracion.multiprocesamiento != datosConfiguracion->nivelDeMultiProcesamiento){
                        //enviarMensajeAMemorias
                        avisoNuevoNivelDeMultiProcesamiento(string_itoa(nuevaConfiguracion.multiprocesamiento), memoriasConocidas);
                    }

                    datosConfiguracion->retardoEjecucion = nuevaConfiguracion.retardoEjecucion;
                    datosConfiguracion->nivelDeMultiProcesamiento = nuevaConfiguracion.multiprocesamiento;
                    datosConfiguracion->Quantum= nuevaConfiguracion.quantum;
                    datosConfiguracion->refreshMetadata = nuevaConfiguracion.refreshMetadata;
                    //log_info(logger, string_from_format("Quantum nuevo: %i. Nivel de MP nuevo: %i. Metadata-refresh nuevo: %i. Sleep ejecucion nuevo: %i", datosConfiguracion->Quantum, datosConfiguracion->nivelDeMultiProcesamiento, datosConfiguracion->refreshMetadata, datosConfiguracion->retardoEjecucion));
                    log_info(logger, "Datos de archivo de configuracion actualizados");
                    pthread_mutex_unlock(mutexDatosConfiguracion);
                }
            } else if (event->mask & IN_DELETE) {
                if (event->mask & IN_ISDIR) {
                    printf("Se eliminó el directorio%s.\n", event->name);
                } else {
                    //printf("Se eliminó el archivo%s.\n", event->name);
                    //printf("Se modificó el archivo%s.\n", event->name);
                    pthread_mutex_lock(mutexDatosConfiguracion);
                    //log_info(logger, string_from_format("Quantum anterior: %i. Nivel de MP anterior: %i. Metadata-refresh anterior: %i. Sleep ejecucion anterior: %i", datosConfiguracion->Quantum, datosConfiguracion->nivelDeMultiProcesamiento, datosConfiguracion->refreshMetadata, datosConfiguracion->retardoEjecucion));
                    t_configuracion nuevaConfiguracion = cargarConfiguracion(nombreArchivoDeConfiguracion, logger);
                    if (nuevaConfiguracion.multiprocesamiento != datosConfiguracion->nivelDeMultiProcesamiento){
                        //enviarMensajeAMemorias
                        avisoNuevoNivelDeMultiProcesamiento(string_itoa(nuevaConfiguracion.multiprocesamiento), memoriasConocidas);
                    }
                    datosConfiguracion->retardoEjecucion = nuevaConfiguracion.retardoEjecucion;
                    datosConfiguracion->nivelDeMultiProcesamiento = nuevaConfiguracion.multiprocesamiento;
                    datosConfiguracion->Quantum= nuevaConfiguracion.quantum;
                    datosConfiguracion->refreshMetadata = nuevaConfiguracion.refreshMetadata;
                    //log_info(logger, string_from_format("Quantum nuevo: %i. Nivel de MP nuevo: %i. Metadata-refresh nuevo: %i. Sleep ejecucion nuevo: %i", datosConfiguracion->Quantum, datosConfiguracion->nivelDeMultiProcesamiento, datosConfiguracion->refreshMetadata, datosConfiguracion->retardoEjecucion));
                    log_info(logger, "Datos de archivo de configuracion actualizados");
                    pthread_mutex_unlock(mutexDatosConfiguracion);
                }
            } else if (event->mask & IN_MODIFY) {
                if (event->mask & IN_ISDIR) {
                    //printf("Se modificó el directorio %s.\n", event->name);
                    //printf("Se modificó el archivo%s.\n", event->name);
                    pthread_mutex_lock(mutexDatosConfiguracion);
                    //log_info(logger, string_from_format("Quantum anterior: %i. Nivel de MP anterior: %i. Metadata-refresh anterior: %i. Sleep ejecucion anterior: %i", datosConfiguracion->Quantum, datosConfiguracion->nivelDeMultiProcesamiento, datosConfiguracion->refreshMetadata, datosConfiguracion->retardoEjecucion));
                    t_configuracion nuevaConfiguracion = cargarConfiguracion(nombreArchivoDeConfiguracion, logger);
                    if (nuevaConfiguracion.multiprocesamiento != datosConfiguracion->nivelDeMultiProcesamiento){
                        //enviarMensajeAMemorias
                        avisoNuevoNivelDeMultiProcesamiento(string_itoa(nuevaConfiguracion.multiprocesamiento), memoriasConocidas);
                    }
                    datosConfiguracion->retardoEjecucion = nuevaConfiguracion.retardoEjecucion;
                    datosConfiguracion->nivelDeMultiProcesamiento = nuevaConfiguracion.multiprocesamiento;
                    datosConfiguracion->Quantum= nuevaConfiguracion.quantum;
                    datosConfiguracion->refreshMetadata = nuevaConfiguracion.refreshMetadata;
                    //log_info(logger, string_from_format("Quantum nuevo: %i. Nivel de MP nuevo: %i. Metadata-refresh nuevo: %i. Sleep ejecucion nuevo: %i", datosConfiguracion->Quantum, datosConfiguracion->nivelDeMultiProcesamiento, datosConfiguracion->refreshMetadata, datosConfiguracion->retardoEjecucion));
                    log_info(logger, "Datos de archivo de configuracion actualizados");
                    pthread_mutex_unlock(mutexDatosConfiguracion);
                } else {

                    //printf("Se modificó el archivo%s.\n", event->name);
                    pthread_mutex_lock(mutexDatosConfiguracion);
                    //log_info(logger, string_from_format("Quantum anterior: %i. Nivel de MP anterior: %i. Metadata-refresh anterior: %i. Sleep ejecucion anterior: %i", datosConfiguracion->Quantum, datosConfiguracion->nivelDeMultiProcesamiento, datosConfiguracion->refreshMetadata, datosConfiguracion->retardoEjecucion));
                    t_configuracion nuevaConfiguracion = cargarConfiguracion(nombreArchivoDeConfiguracion, logger);
                    if (nuevaConfiguracion.multiprocesamiento != datosConfiguracion->nivelDeMultiProcesamiento){
                        //enviarMensajeAMemorias
                        avisoNuevoNivelDeMultiProcesamiento(string_itoa(nuevaConfiguracion.multiprocesamiento), memoriasConocidas);
                    }
                    datosConfiguracion->retardoEjecucion = nuevaConfiguracion.retardoEjecucion;
                    datosConfiguracion->nivelDeMultiProcesamiento = nuevaConfiguracion.multiprocesamiento;
                    datosConfiguracion->Quantum= nuevaConfiguracion.quantum;
                    datosConfiguracion->refreshMetadata = nuevaConfiguracion.refreshMetadata;
                    //log_info(logger, string_from_format("Quantum nuevo: %i. Nivel de MP nuevo: %i. Metadata-refresh nuevo: %i. Sleep ejecucion nuevo: %i", datosConfiguracion->Quantum, datosConfiguracion->nivelDeMultiProcesamiento, datosConfiguracion->refreshMetadata, datosConfiguracion->retardoEjecucion));
                    log_info(logger, "Datos de archivo de configuracion actualizados");
                    pthread_mutex_unlock(mutexDatosConfiguracion);
                }
            }
        }
        offset += sizeof (struct inotify_event) + event->len;
    }

    inotify_rm_watch(file_descriptor, watch_descriptor);
    close(file_descriptor);
    log_info(logger, "Finalizo el hilo que atiende inofity");
}

void monitorearDirectorio(parametros_hilo_monitor* parametros){

    while(1){
        //pthread_create(hiloInotify, NULL, &atenderInotify, parametros);

        //pthread_detach(*hiloInotify);
        atenderInotify(parametros);
    }

}

pthread_t* crearHiloMonitor(char* directorioAMonitorear, char* nombreArchivoConfiguracionConExtension, t_log* logger, t_datos_configuracion* datosConfiguracion, pthread_mutex_t* mutexDatosConfiguracion, t_list* memoriasConocidas){
    pthread_t* hiloMonitor= malloc(sizeof(pthread_t));
    parametros_hilo_monitor* parametros = (parametros_hilo_monitor*) malloc(sizeof(parametros_hilo_monitor));

    parametros->logger = logger;
    parametros->nombreArchivoDeConfiguracion= nombreArchivoConfiguracionConExtension;
    parametros->directorioAMonitorear = directorioAMonitorear;
    parametros->mutexDatosConfiguracion = mutexDatosConfiguracion;
    parametros->datosConfiguracion = datosConfiguracion;
    parametros->memoriasConocidas = memoriasConocidas;

    pthread_create(hiloMonitor, NULL, &monitorearDirectorio, parametros);
    return hiloMonitor;

}


/*void refreshMetadata(p_planificacion *paramPlanificacionGeneral, int tiempoDeRefresh, t_dictionary *metadataTablas,
                     t_log *logger) {

    int PID = paramPlanificacionGeneral->parametrosPLP->contadorPID;
    int *fdMemoria;

    while (1) {
        sleep(tiempoDeRefresh);
        p_consola_kernel *pConsolaKernel = paramPlanificacionGeneral->parametrosConsola;
        char *criterio = pConsolaKernel;
        fdMemoria = seleccionarMemoriaParaDescribe(pConsolaKernel);
        int resultado = gestionarDescribeGlobalKernel(fdMemoria, PID);
        if (resultado == 0) {
            log_info(logger, "Se actualizó correctamente la metadata del kernel.\n");
        } else {
            log_error(logger, "No se ha actualizado correctamente la metadata del kernel.\n");
        }
    }
}*/
