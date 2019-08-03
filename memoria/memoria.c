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
        char* clavesObligatorias[12] = {
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
                "MEMORY_NUMBER",
                "IP_MEMORIA"
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
        configuracion.ipFileSystem = string_duplicate(config_get_string_value(archivoConfig, "IP_FS"));
		configuracion.puertoFileSystem = config_get_int_value(archivoConfig, "PUERTO_FS");
		configuracion.ipSeeds = config_get_array_value(archivoConfig, "IP_SEEDS");
		configuracion.puertoSeeds = config_get_array_value(archivoConfig, "PUERTO_SEEDS");
		configuracion.retardoMemoria = config_get_int_value(archivoConfig, "RETARDO_MEM");
		configuracion.retardoFileSystem = config_get_int_value(archivoConfig, "RETARDO_FS");
		configuracion.tamanioMemoria = config_get_int_value(archivoConfig, "TAM_MEM");
		configuracion.retardoJournal = config_get_int_value(archivoConfig, "RETARDO_JOURNAL");
		configuracion.retardoGossiping = config_get_int_value(archivoConfig, "RETARDO_GOSSIPING");
		configuracion.memoryNumber = config_get_int_value(archivoConfig, "MEMORY_NUMBER");
        configuracion.ipMemoria = string_duplicate(config_get_string_value(archivoConfig, "IP_MEMORIA"));

        config_destroy(archivoConfig);

		return configuracion;
	}
}

t_memoria* inicializarMemoriaPrincipal(t_configuracion configuracion, int tamanioValue, t_log* logger)    {
    log_info(logger, "Inicializamos la memoria princial");
    t_memoria* memoriaPrincipal = (t_memoria*) malloc(sizeof(t_memoria));
    memoriaPrincipal->tamanioMemoria = configuracion.tamanioMemoria;
    log_info(logger, "Tamaño de memoria: %i", memoriaPrincipal->tamanioMemoria);
    memoriaPrincipal->direcciones = (char*) malloc(sizeof(char) * configuracion.tamanioMemoria);
    memoriaPrincipal->tablaDeSegmentos = dictionary_create();
    memoriaPrincipal->marcosOcupados = 0;
    memoriaPrincipal->tamanioValue = tamanioValue;
    memoriaPrincipal->tamanioPagina = calcularTamanioDePagina(tamanioValue);
    log_info(logger, "Tamaño de página: %i", memoriaPrincipal->tamanioPagina);
    memoriaPrincipal->cantidadTotalMarcos = cantidadTotalMarcosMemoria(*memoriaPrincipal);
    log_info(logger, "Cantidad total de marcos: %i", memoriaPrincipal->cantidadTotalMarcos);
    memoriaPrincipal->memoriasConocidas = list_create();
    memoriaPrincipal->nodosMemoria = list_create();
    pthread_mutex_init(&memoriaPrincipal->control.tablaDeSegmentosEnUso, NULL);
    pthread_mutex_init(&memoriaPrincipal->control.tablaDeMarcosEnUso, NULL);
    pthread_mutex_init(&memoriaPrincipal->control.mutexMarcosOcupados, NULL);

    inicializarTablaDeMarcos(memoriaPrincipal);
    return memoriaPrincipal;
}

void inicializarTablaDeMarcos(t_memoria* memoriaPrincipal)  {
    memoriaPrincipal->tablaDeMarcos = (t_marco*) malloc(sizeof(t_marco) * memoriaPrincipal->cantidadTotalMarcos);

    char* comienzoPagina = memoriaPrincipal->direcciones;

    for(int i = 0; i < memoriaPrincipal->cantidadTotalMarcos; i++)  {
        memoriaPrincipal->tablaDeMarcos[i].base = comienzoPagina;
        memoriaPrincipal->tablaDeMarcos[i].ocupado = false;
        comienzoPagina += memoriaPrincipal->tamanioPagina;
    }
}

t_paquete gestionarInsert(char *nombreTabla, char *key, char *valueConComillas, t_memoria *memoria, t_log *logger,
                          int conexionLissandra, t_sincro_journaling *semaforoJournaling)    {
    char* value = string_substring(valueConComillas, 1, strlen(valueConComillas) - 2);
    free(valueConComillas);
    t_pagina* nuevaPagina = insert(nombreTabla, key, value, memoria, NULL, logger, conexionLissandra, semaforoJournaling);
    t_paquete respuesta = {.tipoMensaje = RESPUESTA, .mensaje = string_from_format("INSERT %s %s \"%s\" realizado con exito.", nombreTabla, key, getValueFromContenidoPagina(nuevaPagina->marco->base)) };
    return respuesta;
}

t_paquete gestionarSelect(char *nombreTabla, char *key, int conexionLissandra, t_memoria *memoria, t_log *logger,
                          t_sincro_journaling *semaforoJournaling) {
    t_paquete respuesta;
    t_pagina* paginaEncontrada = cmdSelect(nombreTabla, key, memoria);
    if(paginaEncontrada != NULL)    {
        respuesta.tipoMensaje = RESPUESTA;
        unsigned long long timestamp = getTimestampFromContenidoPagina(paginaEncontrada->marco->base);
        char *valuePagina = getValueFromContenidoPagina(paginaEncontrada->marco->base);
        respuesta.mensaje = string_from_format("SELECT %s %s. Value: %llu;%s;%s", nombreTabla, key, timestamp, key, valuePagina);
        return respuesta;
    }
    char* request = string_from_format("SELECT %s %s", nombreTabla, key);
//    printf("La request: %s se ha recibido con éxito.\n", request);
    log_info(logger, "Valor no encontrado en memoria. Se enviará la siguiente request a Lissandra: %s", request);
    respuesta = pedidoLissandra(conexionLissandra, SELECT, request, 0, logger);
//    printf("Se ha enviado la request: %s con éxito.\n", request);
//    printf(COLOR_ADVERT"Se ha recibido la siguiente respuesta: %s.\n"COLOR_RESET, respuesta.mensaje);
    free(request);
    if(respuesta.tipoMensaje == RESPUESTA)   {
        char** componentesSelect = string_split(respuesta.mensaje, ";");
        t_pagina* paginaNueva = insert(nombreTabla, key, componentesSelect[2], memoria, componentesSelect[0], logger, conexionLissandra, semaforoJournaling);
        free(respuesta.mensaje);
        unsigned long long timestamp = getTimestampFromContenidoPagina(paginaNueva->marco->base);
        respuesta.mensaje = string_from_format("SELECT %s %s. Value: %llu;%s;%s", nombreTabla, key, timestamp, key, componentesSelect[2]);
        freeArrayDeStrings(componentesSelect);
    }

    return respuesta;
}

t_paquete gestionarCreate(char *nombreTabla, char *tipoConsistencia, char *cantidadParticiones,
                          char *tiempoCompactacion, int conexionLissandra, t_log *logger)   {
    char* request = string_from_format("CREATE %s %s %s %s", nombreTabla, tipoConsistencia, cantidadParticiones, tiempoCompactacion);
//    printf("La request: %s se ha recibido con éxito.\n", request);
    enviarPaquete(conexionLissandra, REQUEST, CREATE, request, -1);
//    printf("Se ha enviado la request: %s con éxito.\n", request);
    free(request);
    t_paquete respuesta = recibirMensajeDeLissandra(conexionLissandra);
//    printf(COLOR_ADVERT"Se ha recibido la siguiente respuesta: %s.\n"COLOR_RESET, respuesta.mensaje);
    return respuesta;
}

t_paquete gestionarDrop(char *nombreTabla, int conexionLissandra, t_memoria *memoria, t_log *logger, t_sincro_journaling* semaforoJournaling)   {
    char* resultado = drop(nombreTabla, memoria, semaforoJournaling);
    log_info(logger, resultado);
    char* request = string_from_format("DROP %s", nombreTabla);
//    printf("La request: %s se ha recibido con éxito.\n", request);
    enviarPaquete(conexionLissandra, REQUEST, DROP, request, -1);
//    printf("Se ha enviado la request: %s con éxito.\n", request);
    free(request);
    t_paquete respuesta = recibirMensajeDeLissandra(conexionLissandra);
    //    printf(COLOR_ADVERT"Se ha recibido la siguiente respuesta: %s.\n"COLOR_RESET, respuesta.mensaje);
    pthread_mutex_lock(&semaforoJournaling->mutexNivel);
    sem_post_n(&semaforoJournaling->semaforoJournaling, semaforoJournaling->cantidadRequestsEnParalelo);
    pthread_mutex_unlock(&semaforoJournaling->mutexNivel);
    if(respuesta.tipoMensaje == ERR)  {
        free(respuesta.mensaje);
        respuesta.mensaje = resultado;
    } else
        free(resultado);
    return respuesta;
}

void mi_dictionary_iterator(parametros_journal* parametrosJournal, t_dictionary *self, void(*closure)(parametros_journal*, char*,void*)) {
    int table_index;
    for (table_index = 0; table_index < self->table_max_size; table_index++) {
        t_hash_element *element = self->elements[table_index];

        while (element != NULL) {
            closure(parametrosJournal, element->key, element->data);
            element = element->next;
        }
    }
}

void enviarInsertLissandra(parametros_journal* parametrosJournal, char* key, char* value, unsigned long long timestamp){

    char* request =string_from_format("INSERT %s %s \"%s\" %llu", parametrosJournal->nombreTabla, key, value, timestamp);
    string_trim(&request);
    t_log* logger = parametrosJournal->logger;


    log_info(logger, request);
//    printf("La request: %s se ha recibido con éxito.", request);
    enviarPaquete(parametrosJournal->conexionLissandra, REQUEST, INSERT, request, -1);
//    printf("Se ha enviado la request: %s con éxito.", request);
    free(request);

    t_paquete respuesta = recibirMensajeDeLissandra(parametrosJournal->conexionLissandra);
//    printf(COLOR_ADVERT"Se ha recibido la siguiente respuesta: %s.\n"COLOR_RESET, respuesta.mensaje);
    if(respuesta.tipoMensaje == RESPUESTA)   {
        log_info(logger, respuesta.mensaje);
    } else{
        log_error(logger, respuesta.mensaje);
    }

    //free(logger);
    free(respuesta.mensaje);


}

//la key es la key correspondiente a la pagina
//el value es un t_pagina
void iterarSobrePaginas(parametros_journal* parametrosJournal, char* key, void* value){

    //TODO buscar una forma mas linda de obtener el timestamp y luego el value
    t_pagina* unaPagina = (t_pagina*) value;
    char* contenidoPagina = unaPagina->marco->base;
    unsigned long long timestamp = getTimestampFromContenidoPagina(contenidoPagina);
    char* unValue = getValueFromContenidoPagina(contenidoPagina);
    if ((int) unaPagina->modificada){

        //(&enviarInsertLissandra)(conexionConLissandra,key, unValue, timestamp);
        (&enviarInsertLissandra)(parametrosJournal, key, unValue, timestamp);
//        enviarPaquete(conexionLissandra->fd, REQUEST, request);
    }
    //free(unaPagina);
    //free(timestamp);
//    free(value);

}
//key es nombre del segmento
//el value es t_segmento
void iterarSegmentos(parametros_journal* parametrosJournal, char* key, void* value){

    parametrosJournal->nombreTabla = key;
    t_segmento* segmento = (t_segmento*) value;
    //accedo a la tabla de paginas correspondiente al segmento (es un t_dictionary*)
    //t_dictionary* tablaDePaginas = (t_dictionary*) segmento->tablaDePaginas;
    mi_dictionary_iterator(parametrosJournal,segmento->tablaDePaginas,&iterarSobrePaginas);
    /*free(segmento);
    free(tablaDePaginas);*/
}


t_paquete gestionarJournal(int conexionConLissandra, t_memoria *memoria, t_log *logger, t_sincro_journaling* semaforoJournaling){
    pthread_mutex_lock(&semaforoJournaling->ejecutandoJournaling);
    parametros_journal* parametrosJournal = (parametros_journal*)malloc(sizeof(parametros_journal));
    parametrosJournal->logger = logger;
    parametrosJournal->conexionLissandra = conexionConLissandra;

    pthread_mutex_lock(&memoria->control.tablaDeSegmentosEnUso);
    mi_dictionary_iterator(parametrosJournal, memoria->tablaDeSegmentos, &iterarSegmentos);
    pthread_mutex_unlock(&memoria->control.tablaDeSegmentosEnUso);

    vaciarMemoria(memoria, logger, semaforoJournaling);

    pthread_mutex_unlock(&semaforoJournaling->ejecutandoJournaling);
    t_paquete resultado = {.mensaje = "Se gestionó el journal", .tipoMensaje = RESPUESTA };

    return resultado;
}

t_paquete gestionarDescribe(char *nombreTabla, int conexionLissandra)  {
    char* request = string_from_format("DESCRIBE");
    if(nombreTabla != NULL) {
        request = string_from_format("DESCRIBE %s", nombreTabla);
    }
    enviarPaquete(conexionLissandra, REQUEST, DESCRIBE, request, -1);
    free(request);
    t_paquete respuesta = recibirMensajeDeLissandra(conexionLissandra);
    return respuesta;
}

t_paquete gestionarRequest(t_comando comando, t_memoria* memoria, int conexionLissandra, t_log* logger, t_sincro_journaling* semaforoJournaling) {
    t_paquete respuesta;
    switch(comando.tipoRequest) {
        case SELECT:
            sem_wait(&semaforoJournaling->semaforoJournaling);
            respuesta = gestionarSelect(comando.parametros[0], comando.parametros[1], conexionLissandra, memoria, logger, semaforoJournaling);
            sem_post(&semaforoJournaling->semaforoJournaling);
            break;
        case INSERT:
            sem_wait(&semaforoJournaling->semaforoJournaling);
            respuesta = gestionarInsert(comando.parametros[0], comando.parametros[1], comando.parametros[2], memoria, logger, conexionLissandra, semaforoJournaling);
            sem_post(&semaforoJournaling->semaforoJournaling);
            break;
        case DROP:
            respuesta = gestionarDrop(comando.parametros[0], conexionLissandra, memoria, logger, semaforoJournaling);
            break;
        case CREATE:
            sem_wait(&semaforoJournaling->semaforoJournaling);
            respuesta = gestionarCreate(comando.parametros[0], comando.parametros[1], comando.parametros[2], comando.parametros[3], conexionLissandra, logger);
            sem_post(&semaforoJournaling->semaforoJournaling);
            break;
        case DESCRIBE:
            sem_wait(&semaforoJournaling->semaforoJournaling);
            respuesta = comando.cantidadParametros == 0? gestionarDescribe(NULL, conexionLissandra) : gestionarDescribe(comando.parametros[0], conexionLissandra);
            sem_post(&semaforoJournaling->semaforoJournaling);
            break;
        case JOURNAL:
            respuesta = gestionarJournal(conexionLissandra, memoria, logger, semaforoJournaling);
            break;
        default:;
            respuesta.tipoMensaje = ERR;
            respuesta.mensaje = "Comando inválido";
            break;
    }

    return respuesta;
}

void* ejecutarConsola(void* parametrosConsola){
    parametros_consola_memoria* parametros = (parametros_consola_memoria*) parametrosConsola;

    t_memoria* memoria = parametros->memoria;
    t_log* logger = parametros->logger;
    t_comando comando;
    t_sincro_journaling* semaforoJournaling = (t_sincro_journaling*)parametros->semaforoJournaling;
    t_parametros_conexion_lissandra parametrosLissandra = parametros->conexionLissandra;

    int conexionLissandra = crearSocketCliente(parametrosLissandra.ip, parametrosLissandra.puerto, logger);

    do {
        char* leido = readline("\nMemoria@suck-ets:~$ ");
        char** comandoParseado = parser(leido);
        if(comandoParseado == NULL)
            continue;

        comando = instanciarComando(comandoParseado);
        free(comandoParseado);
        if(validarComandosComunes(comando, logger)) {
            pthread_mutex_lock(&semaforoJournaling->ejecutandoJournaling);
            pthread_mutex_unlock(&semaforoJournaling->ejecutandoJournaling);
            char* respuesta = gestionarRequest(comando, memoria, conexionLissandra, logger, semaforoJournaling).mensaje;
            log_info(logger, respuesta);
            printf("%s", respuesta);
        }

    } while(comando.tipoRequest != EXIT);
    log_info(logger, "Ya analizamos todo los solicitado");
    cerrarSocket(conexionLissandra, logger);
}

pthread_t* crearHiloConsola(t_memoria* memoria, t_log* logger, t_parametros_conexion_lissandra conexionLissandra, t_sincro_journaling* semaforoJournaling){

    pthread_t* hiloConsola = malloc(sizeof(pthread_t));
    parametros_consola_memoria* parametros = (parametros_consola_memoria*) malloc(sizeof(parametros_consola_memoria));

    parametros->logger = logger;
    parametros->memoria = memoria;
    parametros->conexionLissandra = conexionLissandra;
    parametros->semaforoJournaling = semaforoJournaling;

    pthread_create(hiloConsola, NULL, &ejecutarConsola, parametros);

    return hiloConsola;
}

void* journaling(void* parametrosJournal){
    parametros_hilo_journal* parametros = (parametros_hilo_journal*) parametrosJournal;

    t_memoria* memoria = parametros->memoria;
    t_log* logger = parametros->logger;
    t_retardos_memoria* retardos= parametros->retardos;
    t_sincro_journaling* semaforoJournaling = parametros->semaforoJournaling;
    t_parametros_conexion_lissandra parametrosLissandra = parametros->conexionLissandra;
    pthread_mutex_t* semaforoRetardos = parametros->semaforoRetardos;

    int conexionLissandra = crearSocketCliente(parametrosLissandra.ip, parametrosLissandra.puerto, logger);

    while (1){

//        sleep(15);
        pthread_mutex_lock(semaforoRetardos);
        sleep(retardos->retardoJournaling / 1000);
        pthread_mutex_unlock(semaforoRetardos);
        gestionarJournal(conexionLissandra , memoria, logger, semaforoJournaling);
    }
}
pthread_t* crearHiloJournal(t_memoria* memoria, t_log* logger, t_parametros_conexion_lissandra conexionLissandra, t_retardos_memoria* retardos, t_sincro_journaling* semaforoJournaling, pthread_mutex_t* semaforoRetardos){
    pthread_t* hiloJournal = malloc(sizeof(pthread_t));

    parametros_hilo_journal* parametros = (parametros_hilo_journal*) malloc(sizeof(parametros_hilo_journal));

    parametros->logger = logger;
    parametros->memoria = memoria;
    parametros->conexionLissandra = conexionLissandra;
    parametros->semaforoJournaling = semaforoJournaling;
    parametros->semaforoRetardos = semaforoRetardos;
    //retardo Journal
    parametros->retardos= retardos;

    pthread_create(hiloJournal, NULL, &journaling, parametros);

    return hiloJournal;
}

void agregarIpMemoria(char* ipMemoriaSeed, char* puertoMemoriaSeed, t_list* memoriasConocidas, t_log* logger){
    char* ipAAgregar= string_new();
    //Agregar direccion ip:puerto de memoria conocida a la lista
    string_append(&ipAAgregar, ipMemoriaSeed);
    string_append(&ipAAgregar, ":");
    string_append(&ipAAgregar, puertoMemoriaSeed);
    char* ipNuevaMemoria = string_duplicate(ipAAgregar);

    bool _sonMismoString(void* elemento){
        if (elemento != NULL){
            return (strcmp(ipNuevaMemoria, (char*) elemento) == 0);
        }else{
            return false;
        }
    }
    if (!list_any_satisfy(memoriasConocidas, _sonMismoString)){
        list_add(memoriasConocidas, ipNuevaMemoria);
        log_info(logger, "Nueva memoria agregada a lista de memorias conocidas");
    }else{
        log_info(logger, "Memoria ya conocida");
    }
}

void mostrarMemoriasConocidasAlMomento(t_list* memoriasConocidas, pthread_mutex_t semaforoMemoriasConocidas){
    //pthread_mutex_lock(&semaforoMemoriasConocidas);
    void _mostrarPorPantalla(void* elemento){
        if (elemento != NULL){
//            printf("Conozco a la memoria: %s\n", (char*) elemento);
        }
    }
    if (!list_is_empty(memoriasConocidas)){
        list_iterate(memoriasConocidas, _mostrarPorPantalla);
    }
    //pthread_mutex_unlock(&semaforoMemoriasConocidas);

}
void eliminarMemoriaConocida(t_memoria* memoria, nodoMemoria* unNodoMemoria){
    t_list* memoriasConocidas = memoria->memoriasConocidas;

    char* ipNodo = string_new();

    //Agregar direccion ip:puerto de memoria conocida a la lista
    string_append(&ipNodo, (char*)unNodoMemoria->ipNodoMemoria);
    string_append(&ipNodo, ":");
    string_append(&ipNodo, string_itoa(unNodoMemoria->puertoNodoMemoria));
    bool _esNodoMemoria(void* elemento){
        return (strcmp(ipNodo, (char*) elemento) == 0);
        //nodo->
    }
    list_remove_by_condition(memoriasConocidas, _esNodoMemoria);


}
void eliminarNodoMemoria(int fdNodoMemoria, t_list* nodosMemoria){
    nodoMemoria* nodo;
    bool mismoNodo(void* elemento){
        nodo = (nodoMemoria*) elemento;
        return  fdNodoMemoria == nodo->fdNodoMemoria;
    }
    list_remove_by_condition(nodosMemoria, mismoNodo);

    free(nodo);
}
bool esNodoMemoria(int fdConectado, t_list* nodosMemoria){
    nodoMemoria* nodo;
    bool mismoNodo(void* elemento){
        nodo = (nodoMemoria*) elemento;
        return fdConectado == nodo->fdNodoMemoria;
    }
    return list_any_satisfy(nodosMemoria, mismoNodo);
    //free(nodo);

}
void intercambiarListaGossiping(t_memoria* memoria, pthread_mutex_t* semaforoMemoriasConocidas, t_log* logger, GestorConexiones* misConexiones){
    pthread_mutex_lock(semaforoMemoriasConocidas);
    void enviarPedidoListaGossiping(void* elemento){
        if (elemento != NULL){
            nodoMemoria* unNodo = (nodoMemoria*)elemento;
            enviarPedidoGossiping(unNodo, memoria, semaforoMemoriasConocidas, logger, misConexiones);
        }
    }
    list_iterate(memoria->nodosMemoria, enviarPedidoListaGossiping);
    pthread_mutex_unlock(semaforoMemoriasConocidas);
}

void conectarYAgregarNuevaMemoria(char* ipNuevaMemoria, GestorConexiones* misConexiones, t_log* logger, t_memoria* memoria){
    int fdNodoMemoria;
    bool _sonMismoString(void* elemento){
        if (elemento != NULL){
            return (strcmp(ipNuevaMemoria, (char*) elemento) == 0);
        }else{
            return false;
        }
    }
    if(!list_any_satisfy(memoria->memoriasConocidas, _sonMismoString)){
        char** direccionIp = string_split(ipNuevaMemoria, ":");
        fdNodoMemoria = crearSocketCliente(direccionIp[0], atoi(direccionIp[1]), logger);
        if (fdNodoMemoria > 0){
            log_info(logger, "Nueva conexion establecida con la memoria %s:%s", direccionIp[0], direccionIp[1]);

            nodoMemoria* unNodoMemoria = (nodoMemoria*)malloc(sizeof(nodoMemoria));
            unNodoMemoria->fdNodoMemoria = fdNodoMemoria;
            unNodoMemoria->ipNodoMemoria = direccionIp[0];
            unNodoMemoria->puertoNodoMemoria = atoi(direccionIp[1]);
            list_add(memoria->nodosMemoria, unNodoMemoria);
            agregarIpMemoria(direccionIp[0], direccionIp[1], memoria->memoriasConocidas, logger);

        }
    }
}

void gestionarGossiping(GestorConexiones* misConexiones ,char** ipSeeds, char** puertoSeeds, t_log* logger, t_memoria* memoria, pthread_mutex_t* semaforoMemoriasConocidas){
    int i = 0;
    //t_list* memoriasConocidas = (t_list*) memoria->memoriasConocidas;

    while (ipSeeds[i] != NULL){

        char* ipNuevaMemoria = string_from_format("%s:%s", ipSeeds[i], puertoSeeds[i]);

        pthread_mutex_lock(semaforoMemoriasConocidas);

        conectarYAgregarNuevaMemoria(ipNuevaMemoria, misConexiones, logger, memoria);
        pthread_mutex_unlock(semaforoMemoriasConocidas);
        i++;
    }

}
void* gossiping(void* parametrosGossiping){
    parametros_gossiping* parametros = (parametros_gossiping*) parametrosGossiping;

    t_memoria* memoria = (t_memoria*) parametros->memoria;
    t_log* logger = (t_log*)parametros->logger;
    t_configuracion configuracion = (t_configuracion) parametros->archivoDeConfiguracion;
    GestorConexiones* misConexiones = (GestorConexiones*) parametros->misConexiones;
    pthread_mutex_t* semaforoMemoriasConocidas = parametros->semaforoMemoriasConocidas;
    pthread_mutex_t* semaforoRetardos = (pthread_mutex_t*)parametros->semaforoRetardos;
    while (1){
        //pthread_mutex_lock(semaforoRetardos);
        sleep(configuracion.retardoGossiping/1000);
        //pthread_mutex_unlock(semaforoRetardos);
        //Se conecta a memorias y las agrega como nodos de memoria
        gestionarGossiping(misConexiones, configuracion.ipSeeds, configuracion.puertoSeeds, logger, memoria, semaforoMemoriasConocidas);
        intercambiarListaGossiping(memoria, semaforoMemoriasConocidas, logger, misConexiones);
        //mostrarMemoriasConocidasAlMomento(memoria->memoriasConocidas, semaforoMemoriasConocidas);
        //Avisar a Kernel sobre las memorias que conozco


    }
}
pthread_t * crearHiloGossiping(GestorConexiones* misConexiones , t_memoria* memoria, t_log* logger, t_configuracion configuracion, pthread_mutex_t* semaforoMemoriasConocidas, t_sincro_journaling* semaforoJournaling, t_retardos_memoria* retardos, pthread_mutex_t* semaforoRetardos){
    pthread_t* hiloGossiping = malloc(sizeof(pthread_t));
    parametros_gossiping* parametros = (parametros_gossiping*) malloc(sizeof(parametros_gossiping));

    parametros->logger = logger;
    parametros->memoria = memoria;
    parametros->misConexiones = misConexiones;
    parametros->archivoDeConfiguracion = configuracion;
    parametros->semaforoMemoriasConocidas = semaforoMemoriasConocidas;
    parametros->semaforoJournaling = semaforoJournaling;
    parametros->retardosMemoria = retardos;
    parametros->semaforoRetardos = semaforoRetardos;

    pthread_create(hiloGossiping, NULL, &gossiping, parametros);

    return hiloGossiping;

}

unsigned long long getTimestampFromContenidoPagina(char* contenidoPagina)    {
    return *(unsigned long long*) contenidoPagina;
}

char* getKeyFromContenidoPagina(char* contenidoPagina)  {
    char* puntero = contenidoPagina;
    puntero += sizeof(unsigned long long);
    u_int16_t key = *(u_int16_t*) puntero;
    return string_from_format("%i", key);
}

char* getValueFromContenidoPagina(char* contenidoPagina)    {
    char* puntero = contenidoPagina;
    puntero += sizeof(unsigned long long) + sizeof(u_int16_t);
    return puntero;
}

char* formatearPagina(char* key, char* value, char* timestamp, t_memoria* memoria)   {
    char* contenidoPagina = (char*) malloc(memoria->tamanioPagina);
    char* puntero = contenidoPagina;
    unsigned long long tiempo;
    if(timestamp == NULL)   {
        tiempo = getCurrentTime();
    } else
        tiempo = strtoull(timestamp, NULL, 10);
    memcpy(puntero, &tiempo, sizeof(unsigned long long));
    puntero += sizeof(unsigned long long);
    u_int16_t keyCasteada = (u_int16_t) atoi(key);
    memcpy(puntero, &keyCasteada, sizeof(u_int16_t));
    puntero += sizeof(u_int16_t);
    memcpy(puntero, value, (strlen(value) + 1) * sizeof(char));

    return contenidoPagina;
}

t_pagina* eliminarPaginaLruEInsertarNueva(t_reemplazo_pagina* tipoReemplazoPaginaLRU, char* keyNueva, char* nuevoValue,t_dictionary* tablaDePaginasParaInsertar, t_memoria* memoria, bool recibiTimestamp){
    t_pagina* paginaNueva;
//    printf("Elimina pagina con key: %s\n", paginaLRU->key);

    //Tabla de donde se elimina la pagina
    pthread_mutex_lock(&memoria->control.tablaDeSegmentosEnUso);
    t_segmento* unSegmento = (t_segmento*)dictionary_get(memoria->tablaDeSegmentos, tipoReemplazoPaginaLRU->nombreTabla);
    pthread_mutex_lock(&unSegmento->enUso);
    t_dictionary* tablaDePaginasParaBorrar = (t_dictionary*)unSegmento->tablaDePaginas;

    //la pagina que obtiene está llegando sin un char* key
    //dictionary_remove_and_destroy(tablaDePaginas, paginaLRU->key, &eliminarPagina);
    t_pagina* unaPagina = dictionary_remove(tablaDePaginasParaBorrar, tipoReemplazoPaginaLRU->paginaLRU->key);
    unaPagina->marco->ocupado = false;
    memoria->marcosOcupados--;
    free(unaPagina);
//    printf("Inserto nueva pagina con key %s y contenido %s\n", keyNueva, nuevoValue);
    paginaNueva = insertarNuevaPagina(keyNueva, nuevoValue, tablaDePaginasParaInsertar, memoria, recibiTimestamp);
    pthread_mutex_unlock(&unSegmento->enUso);
    pthread_mutex_unlock(&memoria->control.tablaDeSegmentosEnUso);
    return paginaNueva;

}

t_pagina* reemplazarPagina(char* key, char* nuevoValor, int tamanioPagina, t_dictionary* tablaDePaginas) {

    t_pagina* pagina = dictionary_get(tablaDePaginas, key);
    t_pagina* nuevaPagina = (t_pagina*) malloc(sizeof(t_pagina));
    nuevaPagina->marco = pagina->marco;
    nuevaPagina->modificada = true;
    nuevaPagina->key = pagina->key;
    nuevaPagina->ultimaVezUsada = getCurrentTime();
    free(pagina);
    memcpy(nuevaPagina->marco->base, nuevoValor, tamanioPagina - sizeof(char));
    memcpy(nuevaPagina->marco->base + tamanioPagina - sizeof(char), "\0", sizeof(char));

    dictionary_put(tablaDePaginas, key, nuevaPagina);
    return nuevaPagina;
}

t_pagina* insertarNuevaPagina(char* key, char* value, t_dictionary* tablaDePaginas, t_memoria* memoria, bool recibiTimestamp) {
    t_pagina* nuevaPagina = crearPagina(key, memoria);
    if (recibiTimestamp)
        nuevaPagina->modificada = false;
    //printf("Contenido del marco: %s \n", nuevaPagina->marco->base);
    insertarEnMemoriaAndActualizarTablaDePaginas(nuevaPagina, value, memoria->tamanioPagina, tablaDePaginas);
    return nuevaPagina;
}

t_pagina* crearPagina(char* key, t_memoria* memoria)  {
    t_pagina* nuevaPagina = (t_pagina*) malloc(sizeof(t_pagina));
    nuevaPagina->marco = getMarcoLibre(memoria);
    nuevaPagina->key = string_duplicate(key);
    nuevaPagina->modificada = true;
    nuevaPagina->ultimaVezUsada = getCurrentTime();
    return nuevaPagina;
}

void insertarEnMemoriaAndActualizarTablaDePaginas(t_pagina* nuevaPagina, char* value, int tamanioPagina, t_dictionary* tablaDePaginas)  {
    memcpy(nuevaPagina->marco->base, value, tamanioPagina - sizeof(char));
    memcpy(nuevaPagina->marco->base + tamanioPagina - sizeof(char), "\0", sizeof(char));
    dictionary_put(tablaDePaginas, nuevaPagina->key, nuevaPagina);
}

t_reemplazo_pagina* lruGlobal(t_dictionary* tablaDeSegmentos){

    t_pagina* paginaLRU = malloc(sizeof(t_pagina));
    t_reemplazo_pagina* tipoPaginaLRU = (t_reemplazo_pagina*)malloc(sizeof(t_reemplazo_pagina));
    bool encontrePaginaLRU=  false;
    paginaLRU->ultimaVezUsada = getCurrentTime();
    t_dictionary* tablaDePaginas;
    t_segmento* segmentoAuxiliar;

//    printf("%i", dictionary_has_key(tablaDeSegmentos, "pipo") );
//    fflush(stdout);
    void* buscarPaginaLRUEnTablaDePaginas(char* key,void*unSegmento){

        if (unSegmento!= NULL){
            segmentoAuxiliar = (t_segmento*)unSegmento;

            //if (unaTablaDePaginas != NULL){
                tablaDePaginas = (t_dictionary*)segmentoAuxiliar->tablaDePaginas;
                //tablaDePaginas = dictionary_get(tablaDeSegmentos, nombreTabla);



                int table_index;
                t_pagina* paginaActual;
                for (table_index = 0; table_index < tablaDePaginas->table_max_size; table_index++) {
                    t_hash_element *element = tablaDePaginas->elements[table_index];

                    while (element != NULL) {
                        paginaActual = (t_pagina*) element->data;
                        //si no fue modificada
                        if (paginaActual->modificada == false){
                            if ((paginaActual->ultimaVezUsada) < (paginaLRU->ultimaVezUsada)){

                                paginaLRU = paginaActual;
                                paginaLRU->key = element->key;
                                encontrePaginaLRU = true;
                                tipoPaginaLRU->paginaLRU = paginaLRU;
                                tipoPaginaLRU->nombreTabla = key;
                            }
                        }
                        element = element->next;
                    }
                }
            //}
        }
    }



    dictionary_iterator(tablaDeSegmentos, buscarPaginaLRUEnTablaDePaginas);

    if (encontrePaginaLRU){
        return tipoPaginaLRU;
    } else{
        free(paginaLRU);
        free(tipoPaginaLRU);
        return NULL;
    }
}

t_pagina* lru(t_dictionary* tablaDePaginas) {

    t_pagina* paginaLRU = malloc(sizeof(t_pagina));

    //Uso la hora del momento de la ejecucion para comparar
    paginaLRU->ultimaVezUsada = getCurrentTime();
    bool encontrePaginaLRU=  false;

    int table_index;
    t_pagina* paginaActual;
    for (table_index = 0; table_index < tablaDePaginas->table_max_size; table_index++) {
        t_hash_element *element = tablaDePaginas->elements[table_index];

        while (element != NULL) {
            paginaActual = (t_pagina*) element->data;
            //si no fue modificada
            if (paginaActual->modificada == false){
                if ((paginaActual->ultimaVezUsada) < (paginaLRU->ultimaVezUsada)){

                    paginaLRU = paginaActual;
                    paginaLRU->key = element->key;
                    encontrePaginaLRU = true;
                }
            }
            element = element->next;
        }
    }
    if (encontrePaginaLRU){
        return paginaLRU;
    } else{
        return NULL;
    }
}

t_pagina* insert(char* nombreTabla, char* key, char* value, t_memoria* memoria, char* timestamp, t_log* logger,  int conexionLissandra, t_sincro_journaling* semaforoJournaling){
    char* contenidoPagina = formatearPagina(key, value, timestamp, memoria);
    bool recibiTimestamp = timestamp != NULL;

//    log_info(logger, "Se insertará el siguiente valor en memoria: %s", value);
    t_pagina* pagina = NULL;
    // tengo la tabla en la memoria?
    pthread_mutex_lock(&memoria->control.tablaDeSegmentosEnUso);
    if(dictionary_has_key(memoria->tablaDeSegmentos, nombreTabla))   {
        t_segmento* segmento = (t_segmento*) dictionary_get(memoria->tablaDeSegmentos, nombreTabla);
        pthread_mutex_unlock(&memoria->control.tablaDeSegmentosEnUso);
        // tengo la key en la tabla de páginas?
        pthread_mutex_lock(&segmento->enUso);
        if(dictionary_has_key(segmento->tablaDePaginas, key))   {
            pagina = reemplazarPagina(key, contenidoPagina, memoria->tamanioPagina, segmento->tablaDePaginas);
            pthread_mutex_unlock(&segmento->enUso);
        }
        else {
            pthread_mutex_unlock(&segmento->enUso);
            pthread_mutex_lock(&memoria->control.tablaDeMarcosEnUso);
            if(hayMarcosLibres(memoria))   {
                pthread_mutex_lock(&segmento->enUso);
                pagina = insertarNuevaPagina(key, contenidoPagina, segmento->tablaDePaginas, memoria, recibiTimestamp);
                pthread_mutex_unlock(&segmento->enUso);
                pthread_mutex_unlock(&memoria->control.tablaDeMarcosEnUso);
            }
            else {
                //t_pagina* paginaLRU = lru(segmento->tablaDePaginas);
                //t_pagina* paginaLRU = lruGlobal(memoria->tablaDeSegmentos);
                t_reemplazo_pagina* tipoPaginaLRU = lruGlobal(memoria->tablaDeSegmentos);
                if (tipoPaginaLRU != NULL){
                    pagina = eliminarPaginaLruEInsertarNueva(tipoPaginaLRU, key, contenidoPagina,segmento->tablaDePaginas, memoria, recibiTimestamp);
                    pthread_mutex_unlock(&memoria->control.tablaDeMarcosEnUso);
                }
                else    {
                    pthread_mutex_unlock(&memoria->control.tablaDeMarcosEnUso);
                    log_info(logger, "La memoria se encuentra FULL");
                    sem_post(&semaforoJournaling->semaforoJournaling);
                    gestionarJournal(conexionLissandra, memoria, logger, semaforoJournaling);
                    sem_wait(&semaforoJournaling->semaforoJournaling);
                    return insert(nombreTabla,key,value,memoria, timestamp,logger,conexionLissandra, semaforoJournaling);
                }
            }
        }
    } else {
        pthread_mutex_unlock(&memoria->control.tablaDeSegmentosEnUso);
        pthread_mutex_lock(&memoria->control.tablaDeMarcosEnUso);
        if (hayMarcosLibres(memoria)) {
            t_segmento *nuevoSegmento = crearSegmento(nombreTabla, memoria);
            //        log_info(logger, "Se procede a insertar el nuevo valor.");
            pagina = insertarNuevaPagina(key, contenidoPagina, nuevoSegmento->tablaDePaginas, memoria, recibiTimestamp);
        } else {
            t_reemplazo_pagina *tipoPaginaLRU = lruGlobal(memoria->tablaDeSegmentos);
            if (tipoPaginaLRU != NULL) {
                t_segmento *nuevoSegmento = crearSegmento(nombreTabla, memoria);
                pagina = eliminarPaginaLruEInsertarNueva(tipoPaginaLRU, key, contenidoPagina,
                                                         nuevoSegmento->tablaDePaginas, memoria, recibiTimestamp);
            } else {
                pthread_mutex_unlock(&memoria->control.tablaDeMarcosEnUso);
                log_info(logger, "La memoria se encuentra FULL");
                sem_post(&semaforoJournaling->semaforoJournaling);
                gestionarJournal(conexionLissandra, memoria, logger, semaforoJournaling);
                sem_wait(&semaforoJournaling->semaforoJournaling);
                return insert(nombreTabla, key, value, memoria, timestamp, logger, conexionLissandra,
                              semaforoJournaling);
            }
        }
        pthread_mutex_unlock(&memoria->control.tablaDeMarcosEnUso);
    }
    free(contenidoPagina);
    return pagina;
}

t_segmento* crearSegmento(char* nombreTabla, t_memoria* memoria)    {
    t_segmento* segmento = (t_segmento*) malloc(sizeof(t_segmento));
    segmento->pathTabla = string_duplicate(nombreTabla);
    segmento->tablaDePaginas = dictionary_create();
    pthread_mutex_init(&segmento->enUso, NULL);
    pthread_mutex_lock(&memoria->control.tablaDeSegmentosEnUso);
    dictionary_put(memoria->tablaDeSegmentos, nombreTabla, segmento);
    pthread_mutex_unlock(&memoria->control.tablaDeSegmentosEnUso);
    return segmento;
}

void vaciarMemoria(t_memoria* memoria, t_log* logger, t_sincro_journaling* semaforoJournaling){
    t_dictionary* tablaDeSegmentos = memoria->tablaDeSegmentos;
    int table_index;
    for (table_index = 0; table_index < tablaDeSegmentos->table_max_size; table_index++) {
        t_hash_element *element = tablaDeSegmentos->elements[table_index];

        if (element != NULL) {
            log_info(logger, drop(element->key, memoria, semaforoJournaling));
            //element = element->next;

        }
    }
}

t_pagina* cmdSelect(char* nombreTabla, char* key, t_memoria* memoria)  {
    t_pagina* resultado = NULL;
    pthread_mutex_lock(&memoria->control.tablaDeSegmentosEnUso);
    bool existeLaTabla = dictionary_has_key(memoria->tablaDeSegmentos, nombreTabla);
    pthread_mutex_unlock(&memoria->control.tablaDeSegmentosEnUso);
    if(existeLaTabla)   {
        // obtengo el segmento asociado a la tabla en memoria
        pthread_mutex_lock(&memoria->control.tablaDeSegmentosEnUso);
        t_segmento* segmento = (t_segmento*) dictionary_get(memoria->tablaDeSegmentos, nombreTabla);
        pthread_mutex_unlock(&memoria->control.tablaDeSegmentosEnUso);
        pthread_mutex_lock(&segmento->enUso);
        if(dictionary_has_key(segmento->tablaDePaginas, key))   {
            resultado = (t_pagina*) dictionary_get(segmento->tablaDePaginas, key);
        }
        pthread_mutex_unlock(&segmento->enUso);
    }

    return resultado;
}

char* drop(char* nombreTabla, t_memoria* memoria, t_sincro_journaling* semaforoJournaling)    {
    pthread_mutex_lock(&semaforoJournaling->mutexNivel);
    sem_wait_n(&semaforoJournaling->semaforoJournaling, semaforoJournaling->cantidadRequestsEnParalelo);
    pthread_mutex_unlock(&semaforoJournaling->mutexNivel);

    pthread_mutex_lock(&memoria->control.tablaDeSegmentosEnUso);
    if(dictionary_has_key(memoria->tablaDeSegmentos, nombreTabla))  {
        t_segmento* segmento = dictionary_get(memoria->tablaDeSegmentos, nombreTabla);
        pthread_mutex_lock(&memoria->control.tablaDeMarcosEnUso);
        pthread_mutex_lock(&segmento->enUso);
        liberarPaginasSegmento(segmento->tablaDePaginas, memoria);
        pthread_mutex_unlock(&segmento->enUso);
        pthread_mutex_unlock(&memoria->control.tablaDeMarcosEnUso);
        char* respuesta = string_from_format("Se eliminó la tabla %s de la memoria.", nombreTabla);
        dictionary_remove_and_destroy(memoria->tablaDeSegmentos, nombreTabla, &eliminarSegmento);
        pthread_mutex_unlock(&memoria->control.tablaDeSegmentosEnUso);

        pthread_mutex_lock(&semaforoJournaling->mutexNivel);
        sem_post_n(&semaforoJournaling->semaforoJournaling, semaforoJournaling->cantidadRequestsEnParalelo);
        pthread_mutex_unlock(&semaforoJournaling->mutexNivel);

        return respuesta;
    } else  {
        pthread_mutex_unlock(&memoria->control.tablaDeSegmentosEnUso);

        pthread_mutex_lock(&semaforoJournaling->mutexNivel);
        sem_post_n(&semaforoJournaling->semaforoJournaling, semaforoJournaling->cantidadRequestsEnParalelo);
        pthread_mutex_unlock(&semaforoJournaling->mutexNivel);
        return string_from_format("No se encontró la tabla %s en memoria.", nombreTabla);
    }
}

void liberarPaginasSegmento(t_dictionary* tablaDePaginas, t_memoria* memoria)   {
    int cantidadPaginas = dictionary_size(tablaDePaginas);
    dictionary_destroy_and_destroy_elements(tablaDePaginas, &eliminarPagina);
    memoria->marcosOcupados -= cantidadPaginas;
}

void eliminarPagina(void* data)    {
    t_pagina* pagina = (t_pagina*) data;
    pagina->marco->ocupado = false;
}

void eliminarSegmento(void* data)   {
    t_segmento* segmento = (t_segmento*) data;
    free(segmento->pathTabla);
    free(data);
}

int cantidadTotalMarcosMemoria(t_memoria memoria)   {
    return memoria.tamanioMemoria / memoria.tamanioPagina;
}

int calcularTamanioDePagina(int tamanioValue){
    return sizeof(unsigned long long) + sizeof(u_int16_t) + sizeof(char) * tamanioValue;
}

//Esta funcion envia la petición del TAM_VALUE a lissandra y devuelve la respuesta del HS
int getTamanioValue(t_parametros_conexion_lissandra parametrosLissandra, t_log* logger){
    int conexionLissandra = crearSocketCliente(parametrosLissandra.ip, parametrosLissandra.puerto, logger);
    if(conexionLissandra > 0) {
        hacerHandshake(conexionLissandra, MEMORIA);
        t_paquete respuesta = recibirMensajeDeLissandra(conexionLissandra);
        if (respuesta.mensaje != NULL)  {
            int tamanioValue = atoi(respuesta.mensaje);
            log_info(logger, "Tamaño del value obtenido de Lissandra: %i", tamanioValue);
            return tamanioValue;
        }
        else {
            log_error(logger, "Se cerró la conexión con Lissandra. Finalizando memoria.");
            exit(-1);
        }
    }
    else    {
        log_error(logger, "No se pudo establecer la conexión con Lissandra. Cerrando el proceso.");
        exit(-1);
    }
    cerrarSocket(conexionLissandra, logger);
}

t_marco* getMarcoLibre(t_memoria* memoria)   {
    for(int i = 0; i < memoria->cantidadTotalMarcos; i++)   {
        t_marco* marcoLeido = &memoria->tablaDeMarcos[i];
        if(!marcoLeido->ocupado)    {
            marcoLeido->ocupado = true;
            memoria->marcosOcupados++;
            return marcoLeido;
        }
    }

    return NULL;
}

bool hayMarcosLibres(t_memoria* memoria)  {
    bool hayMarcosDisponibles = memoria->marcosOcupados < memoria->cantidadTotalMarcos;
    return hayMarcosDisponibles;
}

t_retardos_memoria* almacenarRetardosDeMemoria(t_configuracion configuracion){
    t_retardos_memoria* retardos = malloc(sizeof(t_retardos_memoria));
    retardos->retardoMemoria = configuracion.retardoMemoria;
    retardos->retardoGossiping = configuracion.retardoGossiping;
    retardos->retardoFileSystem = configuracion.retardoFileSystem;
    retardos->retardoJournaling = configuracion.retardoJournal;
    return retardos;
}

void* monitorearDirectorio(void* parametrosMonitor){
    parametros_hilo_monitor* parametros = (parametros_hilo_monitor*) parametrosMonitor;

    char* nombreDirectorio = parametros->directorioAMonitorear;
    char* nombreArchivoDeConfiguracion = parametros->nombreArchivoDeConfiguracion;
    t_log* logger = parametros->logger;
    t_retardos_memoria* retardos = parametros->retardos;
    pthread_mutex_t* semaforoRetardos = parametros->semaforoRetardos;
    char buffer[BUF_LEN];

    int file_descriptor = inotify_init();
    if (file_descriptor < 0) {
        perror("inotify_init");
    }

    // Creamos un monitor sobre un path indicando que eventos queremos escuchar
    int watch_descriptor = inotify_add_watch(file_descriptor, nombreDirectorio, IN_MODIFY);

    // El file descriptor creado por inotify, es el que recibe la información sobre los eventos ocurridos
    // para leer esta información el descriptor se lee como si fuera un archivo comun y corriente pero
    // la diferencia esta en que lo que leemos no es el contenido de un archivo sino la información
    // referente a los eventos ocurridos
    while (1){
        int length = read(file_descriptor, buffer, BUF_LEN);
        if (length < 0) {
            perror("read");
        }


        int offset = 0;

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
//                        printf("Se creo el directorio %s .\n", event->name);
                    } else {
//                        printf("Se creó el archivo%s.\n", event->name);
                    }
                } else if (event->mask & IN_DELETE) {
                    if (event->mask & IN_ISDIR) {
//                        printf("Se eliminó el directorio%s.\n", event->name);
                    } else {
//                        printf("Se eliminó el archivo%s.\n", event->name);
                    }
                } else if (event->mask & IN_MODIFY) {
                    if (event->mask & IN_ISDIR) {
//                        printf("Se modificó el directorio %s.\n", event->name);
                    } else {

//                        printf("Se modificó el archivo%s.\n", event->name);
                        pthread_mutex_lock(semaforoRetardos);
                        log_info(logger, string_from_format("Retardo Memoria anterior: %i. Retardo Gossiping Anterior: %i. Retardo Journaling Anterior: %i. Retardo Filesystem Anterior: %i", retardos->retardoMemoria, retardos->retardoGossiping, retardos->retardoJournaling, retardos->retardoFileSystem));
                        t_configuracion nuevaConfiguracion = cargarConfiguracion(nombreArchivoDeConfiguracion, logger);
                        retardos->retardoMemoria = nuevaConfiguracion.retardoMemoria;
                        retardos->retardoJournaling = nuevaConfiguracion.retardoJournal;
                        retardos->retardoFileSystem = nuevaConfiguracion.retardoFileSystem;
                        retardos->retardoGossiping = nuevaConfiguracion.retardoGossiping;
                        log_info(logger, string_from_format("Retardo Memoria nuevo: %i. Retardo Gossiping nuevo: %i. Retardo Journaling nuevo: %i. Retardo Filesystem nuevo: %i", retardos->retardoMemoria, retardos->retardoGossiping, retardos->retardoJournaling, retardos->retardoFileSystem));
                        pthread_mutex_unlock(semaforoRetardos);
                    }
                }
            }
            offset += sizeof (struct inotify_event) + event->len;
        }
    }

    inotify_rm_watch(file_descriptor, watch_descriptor);
    close(file_descriptor);
}

pthread_t* crearHiloMonitor(char* directorioAMonitorear, char* nombreArchivoConfiguracionConExtension, t_log* logger, t_retardos_memoria* retardos, pthread_mutex_t* semaforoRetardos){
    pthread_t* hiloMonitor= malloc(sizeof(pthread_t));
    parametros_hilo_monitor* parametros = (parametros_hilo_monitor*) malloc(sizeof(parametros_hilo_monitor));

    parametros->logger = logger;
    parametros->retardos= retardos;
    parametros->directorioAMonitorear= directorioAMonitorear;
    parametros->nombreArchivoDeConfiguracion = nombreArchivoConfiguracionConExtension;
    parametros->semaforoRetardos = semaforoRetardos;

    pthread_create(hiloMonitor, NULL, &monitorearDirectorio, parametros);
    return hiloMonitor;

}
int main(int argc, char* argv[]) {
    //char *nombrePruebaDebug = string_duplicate("prueba-lfs"); //Para debuggear
    //char *nombreConfigDebug = string_duplicate("memoria1"); //Para debuggear
    //char *rutaConfig = string_from_format("../pruebas/%s/memoria/%s.cfg", nombrePruebaDebug, nombreConfigDebug); //Para debuggear
    char *rutaConfig = string_from_format("/home/utnso/pruebas/%s/memoria/%s.cfg", argv[1], argv[2]); //Para ejecutar
    //char *rutaLogger = string_from_format("%s.log", nombrePruebaDebug); //Para debuggear
    char *rutaLogger = string_from_format("%s.log", argv[2]); //Para ejecutar

    t_log* logger = log_create(rutaLogger, "memoria", false, LOG_LEVEL_INFO);
	t_configuracion configuracion = cargarConfiguracion(rutaConfig, logger);
	t_parametros_conexion_lissandra conexionLissandra = {.ip = string_duplicate(configuracion.ipFileSystem), .puerto = conexionLissandra.puerto = configuracion.puertoFileSystem};


    pthread_mutex_t* semaforoRetardos = malloc(sizeof(pthread_mutex_t));
    pthread_mutex_init(semaforoRetardos, NULL);

    t_retardos_memoria* retardos = almacenarRetardosDeMemoria(configuracion);

    char* directorioAMonitorear = "/home/utnso/tp-2019-1c-Suck-et/prueba/prueba-lfs/memoria/";
    //monitorearDirectorio("/home/utnso/tp-2019-1c-Suck-et/memoria/", nombreArchivoConfiguracionConExtension, logger, retardos);

    t_control_conexion conexionKernel = {.fd = 0, .semaforo = (sem_t*) malloc(sizeof(sem_t))};
//    t_control_conexion conexionLissandra = {.semaforo = (sem_t*) malloc(sizeof(sem_t))};

    sem_init(conexionKernel.semaforo, 0, 0);
//    sem_init(conexionLissandra.semaforo, 0, 0);

    pthread_mutex_t* semaforoMemoriasConocidas = malloc(sizeof(pthread_mutex_t));
    pthread_mutex_init(semaforoMemoriasConocidas, NULL);

    t_sincro_journaling* semaforoJournaling = (t_sincro_journaling*) malloc(sizeof(t_sincro_journaling));
    semaforoJournaling->cantidadRequestsEnParalelo = 1;
    sem_init(&semaforoJournaling->semaforoJournaling, 0, semaforoJournaling->cantidadRequestsEnParalelo);
    pthread_mutex_init(&semaforoJournaling->mutexNivel, NULL);
    pthread_mutex_init(&semaforoJournaling->ejecutandoJournaling, NULL);

//	conectarseALissandra(&conexionLissandra, configuracion.ipFileSystem, configuracion.puertoFileSystem, logger);
	int tamanioValue = getTamanioValue(conexionLissandra, logger);
    t_memoria* memoriaPrincipal = inicializarMemoriaPrincipal(configuracion, tamanioValue, logger);
    memoriaPrincipal->memoryNumber = (int)configuracion.memoryNumber;
	GestorConexiones* misConexiones = inicializarConexion();
    levantarServidor(configuracion.puerto, misConexiones, logger);

    pthread_mutex_lock(semaforoMemoriasConocidas);
    agregarIpMemoria(configuracion.ipMemoria, string_itoa(configuracion.puerto), memoriaPrincipal->memoriasConocidas, logger);
    pthread_mutex_unlock(semaforoMemoriasConocidas);

    pthread_t * hiloMonitor = (pthread_t*)crearHiloMonitor(directorioAMonitorear, argv[2], logger, retardos, semaforoRetardos);
    pthread_t* hiloConexiones = (pthread_t*)crearHiloConexiones(misConexiones, memoriaPrincipal, &conexionKernel, conexionLissandra, logger, semaforoMemoriasConocidas, semaforoJournaling, retardos);
    pthread_t* hiloConsola = (pthread_t*) crearHiloConsola(memoriaPrincipal, logger, conexionLissandra, semaforoJournaling);
    //pthread_t* hiloJournal = (pthread_t*) crearHiloJournal(memoriaPrincipal, logger, conexionLissandra, retardos, semaforoJournaling, semaforoRetardos);
    pthread_t* hiloGossiping = (pthread_t*) crearHiloGossiping(misConexiones, memoriaPrincipal, logger, configuracion, semaforoMemoriasConocidas, semaforoJournaling, retardos, semaforoRetardos);

    pthread_join(*hiloConexiones, NULL);
    pthread_join(*hiloConsola, NULL);
    //pthread_join(*hiloJournal, NULL);
    pthread_join(*hiloGossiping, NULL);
    pthread_join(*hiloMonitor, NULL);

    pthread_detach(*hiloMonitor);
    pthread_detach(*hiloConexiones);
    pthread_detach(*hiloConsola);
    //pthread_detach(*hiloJournal);
    pthread_detach(*hiloGossiping);

    //free(nombrePruebaDebug); //TODO: Si se esta ejecutando comentar esta linea
    //free(nombreConfigDebug); //TODO: Si se esta ejecutando comentar esta linea
    free(rutaConfig);
    free(rutaLogger);

	return 0;
}