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
        configuracion.ipFileSystem = string_duplicate(config_get_string_value(archivoConfig, "IP_FS"));
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

t_memoria* inicializarMemoriaPrincipal(t_configuracion configuracion, int tamanioValue, t_log* logger)    {
    log_info(logger, "Inicializamos la memoria princial");
    t_memoria* memoriaPrincipal = (t_memoria*) malloc(sizeof(t_memoria));
    memoriaPrincipal->tamanioMemoria = configuracion.tamanioMemoria;
    log_info(logger, "Tamaño de memoria: %i", memoriaPrincipal->tamanioMemoria);
    memoriaPrincipal->direcciones = (char*) malloc(sizeof(char) * configuracion.tamanioMemoria);
    memoriaPrincipal->tablaDeSegmentos = dictionary_create();
    memoriaPrincipal->marcosOcupados = 0;
    memoriaPrincipal->cantidadMaximaCaracteresValue = getCantidadCaracteresByPeso(tamanioValue);
    memoriaPrincipal->tamanioPagina = calcularTamanioDePagina(tamanioValue);
    log_info(logger, "Tamaño de página: %i", memoriaPrincipal->tamanioPagina);
    memoriaPrincipal->cantidadTotalMarcos = cantidadTotalMarcosMemoria(*memoriaPrincipal);
    log_info(logger, "Cantidad total de marcos: %i", memoriaPrincipal->cantidadTotalMarcos);
    memoriaPrincipal->memoriasConocidas = list_create();
    memoriaPrincipal->nodosMemoria = list_create();

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

t_paquete gestionarInsert(char* nombreTabla, char* key, char* valueConComillas, t_memoria* memoria, t_log* logger, t_control_conexion* conexionLissandra, pthread_mutex_t* semaforoJournaling)    {
    char* value = string_substring(valueConComillas, 1, strlen(valueConComillas) - 2);
    free(valueConComillas);
    t_pagina* nuevaPagina = insert(nombreTabla, key, value, memoria, NULL, logger, conexionLissandra, semaforoJournaling);
    free(value);
    t_paquete respuesta = {.tipoMensaje = RESPUESTA, .mensaje = string_from_format("Valor insertado: %s", nuevaPagina->marco->base) };
    return respuesta;
}

t_paquete gestionarSelect(char *nombreTabla, char *key, t_control_conexion *conexionLissandra, t_memoria *memoria, t_log *logger, pthread_mutex_t* semaforoJournaling) {
    t_paquete respuesta;
    t_pagina* paginaEncontrada = cmdSelect(nombreTabla, key, memoria);
    if(paginaEncontrada != NULL)    {
        respuesta.tipoMensaje = RESPUESTA;
        respuesta.mensaje = paginaEncontrada->marco->base;
        return respuesta;
    }
    char* request = string_from_format("SELECT %s %s", nombreTabla, key);
    log_info(logger, "Valor no encontrado en memoria. Se enviará la siguiente request a Lissandra: %s", request);
    sem_wait(conexionLissandra->semaforo);
    enviarPaquete(conexionLissandra->fd, REQUEST, SELECT, request);
    free(request);
    respuesta = recibirMensajeDeLissandra(conexionLissandra);
    sem_post(conexionLissandra->semaforo);
    if(respuesta.tipoMensaje == RESPUESTA)   {
        char** componentesSelect = string_split(respuesta.mensaje, ";");
        insert(nombreTabla, key, componentesSelect[2], memoria, componentesSelect[0], logger, conexionLissandra, semaforoJournaling);
        free(respuesta.mensaje);
        respuesta.mensaje = string_from_format("%s%s%s", componentesSelect[0], key, componentesSelect[2]);
    }

    return respuesta;
}

t_paquete gestionarCreate(char* nombreTabla, char* tipoConsistencia, char* cantidadParticiones, char* tiempoCompactacion, t_control_conexion* conexionLissandra, t_log* logger)   {
    char* request = string_from_format("CREATE %s %s %s %s", nombreTabla, tipoConsistencia, cantidadParticiones, tiempoCompactacion);
    sem_wait(conexionLissandra->semaforo);
    enviarPaquete(conexionLissandra->fd, REQUEST, CREATE, request);
    free(request);
    t_paquete respuesta = recibirMensajeDeLissandra(conexionLissandra);
    sem_post(conexionLissandra->semaforo);
    return respuesta;
}

t_paquete gestionarDrop(char* nombreTabla, t_control_conexion* conexionLissandra, t_memoria* memoria, t_log* logger)   {
    char* resultado = drop(nombreTabla, memoria);
    log_info(logger, resultado);
    char* request = string_from_format("DROP %s", nombreTabla);
    sem_wait(conexionLissandra->semaforo);
    enviarPaquete(conexionLissandra->fd, REQUEST, DROP, request);
    free(request);
    t_paquete respuesta = recibirMensajeDeLissandra(conexionLissandra);
    sem_post(conexionLissandra->semaforo);
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

void enviarInsertLissandra(parametros_journal* parametrosJournal, char* key, char* value, char* timestamp){

    char* request =string_from_format("INSERT %s %s \"%s\" %s", parametrosJournal->nombreTabla, key, value, timestamp);
    string_trim(&request);
    t_log* logger = parametrosJournal->logger;


    log_info(logger, request);
    sem_wait(parametrosJournal->conexionLissandra->semaforo);
    enviarPaquete(parametrosJournal->conexionLissandra->fd, REQUEST, INSERT, request);
    free(request);

    t_paquete respuesta = recibirMensajeDeLissandra(parametrosJournal->conexionLissandra);
    sem_post(parametrosJournal->conexionLissandra->semaforo);
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
    char* timestamp = string_substring(unaPagina->marco->base, 0, 10);
    char* unValue = string_substring(unaPagina->marco->base, 11, strlen(unaPagina->marco->base));
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


t_paquete gestionarJournal(t_control_conexion *conexionConLissandra, t_memoria *memoria, t_log *logger, pthread_mutex_t* semaforoJournaling){

    pthread_mutex_lock(semaforoJournaling);
    parametros_journal* parametrosJournal = (parametros_journal*)malloc(sizeof(parametros_journal));
    parametrosJournal->logger = logger;
    parametrosJournal->conexionLissandra = conexionConLissandra;

    mi_dictionary_iterator(parametrosJournal, memoria->tablaDeSegmentos, &iterarSegmentos);

    vaciarMemoria(memoria, logger);

    pthread_mutex_unlock(semaforoJournaling);

    t_paquete resultado = {.mensaje = "Se gestionó el journal", .tipoMensaje = RESPUESTA };

    return resultado;
}

t_paquete gestionarDescribe(char *nombreTabla, t_control_conexion *conexionLissandra)  {
    char* request = string_from_format("DESCRIBE");
    if(nombreTabla != NULL) {
        request = string_from_format("DESCRIBE %s", nombreTabla);
    }
    sem_wait(conexionLissandra->semaforo);
    enviarPaquete(conexionLissandra->fd, REQUEST, DESCRIBE, request);
    free(request);
    t_paquete respuesta = recibirMensajeDeLissandra(conexionLissandra);
    sem_post(conexionLissandra->semaforo);
    return respuesta;
}

t_paquete gestionarRequest(t_comando comando, t_memoria* memoria, t_control_conexion* conexionLissandra, t_log* logger, pthread_mutex_t* semaforoJournaling) {
    switch(comando.tipoRequest) {
        case SELECT:
            return gestionarSelect(comando.parametros[0], comando.parametros[1], conexionLissandra, memoria, logger, semaforoJournaling);
        case INSERT:
            return gestionarInsert(comando.parametros[0], comando.parametros[1], comando.parametros[2], memoria, logger, conexionLissandra, semaforoJournaling);
        case DROP:
            return gestionarDrop(comando.parametros[0], conexionLissandra, memoria, logger);
        case CREATE:
            return gestionarCreate(comando.parametros[0], comando.parametros[1], comando.parametros[2], comando.parametros[3], conexionLissandra, logger);
        case DESCRIBE:
            return comando.cantidadParametros == 0? gestionarDescribe(NULL, conexionLissandra) : gestionarDescribe(comando.parametros[0], conexionLissandra);
        case JOURNAL:
            return gestionarJournal(conexionLissandra, memoria, logger, semaforoJournaling);
        default:;
            t_paquete respuesta = {.tipoMensaje = ERR, .mensaje = "Comando inválido"};
            return respuesta;
    }
}

void* ejecutarConsola(void* parametrosConsola){
    parametros_consola_memoria* parametros = (parametros_consola_memoria*) parametrosConsola;

    t_memoria* memoria = parametros->memoria;
    t_log* logger = parametros->logger;
    t_control_conexion* conexionLissandra = (t_control_conexion*) parametros->conexionLissandra;
    t_comando comando;
    pthread_mutex_t* semaforoJournaling = (pthread_mutex_t*)parametros->semaforoJournaling;


    do {
        char* leido = readline("\nMemoria@suck-ets:~$ ");
        char** comandoParseado = parser(leido);
        if(comandoParseado == NULL)
            continue;

        pthread_mutex_lock(semaforoJournaling);
        pthread_mutex_unlock(semaforoJournaling);
        comando = instanciarComando(comandoParseado);
        free(comandoParseado);
        if(validarComandosComunes(comando, logger))
            log_info(logger, gestionarRequest(comando, memoria, conexionLissandra, logger, semaforoJournaling).mensaje);

    } while(comando.tipoRequest != EXIT);
    log_info(logger, "Ya analizamos todo los solicitado");
}

pthread_t* crearHiloConsola(t_memoria* memoria, t_log* logger, t_control_conexion* conexionLissandra, pthread_mutex_t* semaforoJournaling){

    pthread_t* hiloConsola = malloc(sizeof(pthread_t));
    parametros_consola_memoria* parametros = (parametros_consola_memoria*) malloc(sizeof(parametros_consola_memoria));

    parametros->logger = logger;
    parametros->memoria = memoria;
    parametros->conexionLissandra = conexionLissandra;
    parametros->semaforoJournaling = semaforoJournaling;

    pthread_create(hiloConsola, NULL, &ejecutarConsola, parametros);
    return hiloConsola;
}

void journaling(parametros_hilo_journal* parametros){

    t_memoria* memoria = parametros->memoria;
    t_log* logger = parametros->logger;
    t_control_conexion* conexionLissandra = parametros->conexionLissandra;
    int retardoJournal = parametros->retardo;
    pthread_mutex_t* semaforoJournaling = parametros->semaforoJournaling;

    while (1){
        sleep(15);
        //sleep(retardoJournal);
        //pthread_mutex_lock(semaforoJournaling);
        gestionarJournal(conexionLissandra , memoria, logger, semaforoJournaling);
        log_info(logger, "Ha finalizado el Journal\n");
        fflush(stdout);
        //pthread_mutex_unlock(semaforoJournaling);
    }
}
pthread_t* crearHiloJournal(t_memoria* memoria, t_log* logger, t_control_conexion* conexionLissandra, int retardoJournal, pthread_mutex_t* semaforoJournaling){
    pthread_t* hiloJournal = malloc(sizeof(pthread_t));

    parametros_hilo_journal* parametros = (parametros_hilo_journal*) malloc(sizeof(parametros_hilo_journal));

    parametros->logger = logger;
    parametros->memoria = memoria;
    parametros->conexionLissandra = conexionLissandra;
    parametros->semaforoJournaling = semaforoJournaling;
    //retardo Journal
    parametros->retardo = retardoJournal;

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
            printf("Conozco a la memoria: %s\n", (char*) elemento);
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
    void enviarPedidoListaGossiping(void* elemento){
        if (elemento != NULL){
            nodoMemoria* unNodo = (nodoMemoria*)elemento;
            enviarPedidoGossiping(unNodo, memoria, semaforoMemoriasConocidas, logger, misConexiones);
        }
    }
    list_iterate(memoria->nodosMemoria, enviarPedidoListaGossiping);
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
        fdNodoMemoria = conectarseAServidor(direccionIp[0], atoi(direccionIp[1]), misConexiones, logger );
        if (fdNodoMemoria != NULL && fdNodoMemoria >0){
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

    while (ipSeeds[i] != NULL && puertoSeeds[i] != NULL){

        char* ipNuevaMemoria = string_new();
        string_append(&ipNuevaMemoria, ipSeeds[i]);
        string_append(&ipNuevaMemoria, ":");
        string_append(&ipNuevaMemoria, puertoSeeds[i]);

        pthread_mutex_lock(semaforoMemoriasConocidas);

        conectarYAgregarNuevaMemoria(ipNuevaMemoria, misConexiones, logger, memoria);
        pthread_mutex_unlock(semaforoMemoriasConocidas);
        i++;
    }

}
void gossiping(parametros_gossiping* parametros){

    t_memoria* memoria = (t_memoria*) parametros->memoria;
    t_log* logger = (t_log*)parametros->logger;
    t_configuracion configuracion = (t_configuracion) parametros->archivoDeConfiguracion;
    GestorConexiones* misConexiones = (GestorConexiones*) parametros->misConexiones;
    pthread_mutex_t* semaforoMemoriasConocidas = parametros->semaforoMemoriasConocidas;
    while (1){
        sleep(15);
        //sleep(configuracion.retardoGossiping);
        //Se conecta a memorias y las agrega como nodos de memoria
        gestionarGossiping(misConexiones, configuracion.ipSeeds, configuracion.puertoSeeds, logger, memoria, semaforoMemoriasConocidas);



        pthread_mutex_lock(parametros->semaforoJournaling);
        pthread_mutex_unlock(parametros->semaforoJournaling);
        //pthread_mutex_lock(semaforoMemoriasConocidas);

        intercambiarListaGossiping(memoria, semaforoMemoriasConocidas, logger, misConexiones);
        //mostrarMemoriasConocidasAlMomento(memoria->memoriasConocidas, semaforoMemoriasConocidas);
        //pthread_mutex_unlock(semaforoMemoriasConocidas);
        //Avisar a Kernel sobre las memorias que conozco


    }
}
pthread_t * crearHiloGossiping(GestorConexiones* misConexiones , t_memoria* memoria, t_log* logger, t_configuracion configuracion, pthread_mutex_t* semaforoMemoriasConocidas, pthread_mutex_t* semaforoJournaling){
    pthread_t* hiloGossiping = malloc(sizeof(pthread_t));
    parametros_gossiping* parametros = (parametros_gossiping*) malloc(sizeof(parametros_gossiping));

    parametros->logger = logger;
    parametros->memoria = memoria;
    parametros->misConexiones = misConexiones;
    parametros->archivoDeConfiguracion = configuracion;
    parametros->semaforoMemoriasConocidas = semaforoMemoriasConocidas;
    parametros->semaforoJournaling = semaforoJournaling;

    pthread_create(hiloGossiping, NULL, &gossiping, parametros);
    return hiloGossiping;

}
char* formatearPagina(char* key, char* value, char* timestamp)   {
    long tiempo;
    if(timestamp == NULL)   {
        time_t tiempoActual;
        tiempo = (long) time(&tiempoActual);
    } else
        tiempo = atol(timestamp);
    return string_from_format("%lu%s%s", tiempo, key, value);
}
t_pagina* eliminarPaginaLruEInsertarNueva(t_pagina* paginaLRU,char* keyNueva, char* nuevoValue,t_dictionary* tablaDePaginas, t_memoria* memoria, bool recibiTimestamp){
    t_pagina* paginaNueva;
    printf("Elimina pagina con key: %s\n", paginaLRU->key);


    //la pagina que obtiene está llegando sin un char* key
    //dictionary_remove_and_destroy(tablaDePaginas, paginaLRU->key, &eliminarPagina);
    t_pagina* unaPagina = dictionary_remove(tablaDePaginas, paginaLRU->key);
    unaPagina->marco->ocupado = false;
    free(unaPagina);
    memoria->marcosOcupados = memoria->marcosOcupados -1;
    printf("Inserto nueva pagina con key %s y contenido %s\n", keyNueva, nuevoValue);
    paginaNueva = insertarNuevaPagina(keyNueva, nuevoValue, tablaDePaginas, memoria, recibiTimestamp);
    return paginaNueva;

}
t_pagina* reemplazarPagina(char* key, char* nuevoValor, int tamanioPagina, t_dictionary* tablaDePaginas) {

    t_pagina* pagina = dictionary_get(tablaDePaginas, key);
    t_pagina* nuevaPagina = (t_pagina*) malloc(sizeof(t_pagina));
    nuevaPagina->marco = pagina->marco;
    nuevaPagina->modificada = true;
    nuevaPagina->key = pagina->key;
    time_t elTiempoActual;
    long elTiempo;
    elTiempo = (long) time(&elTiempoActual);
    nuevaPagina->ultimaVezUsada = elTiempo;
    //pagina->
    free(pagina);
    strncpy(nuevaPagina->marco->base, nuevoValor, tamanioPagina - 1);
    strcpy(nuevaPagina->marco->base + tamanioPagina - 1, "\0");

    dictionary_put(tablaDePaginas, key, nuevaPagina);
    return nuevaPagina;
}

t_pagina* insertarNuevaPagina(char* key, char* value, t_dictionary* tablaDePaginas, t_memoria* memoria, bool recibiTimestamp) {
    t_pagina* nuevaPagina = crearPagina(key, memoria);
    if (recibiTimestamp){
        fflush(stdout);
        nuevaPagina->modificada = false;
    }
    //printf("Contenido del marco: %s \n", nuevaPagina->marco->base);
    insertarEnMemoriaAndActualizarTablaDePaginas(nuevaPagina, value, memoria->tamanioPagina, tablaDePaginas);
    return nuevaPagina;
}

t_pagina* crearPagina(char* key, t_memoria* memoria)  {
    t_pagina* nuevaPagina = (t_pagina*) malloc(sizeof(t_pagina));
    long elTiempo;
    nuevaPagina->marco = getMarcoLibre(memoria);
    nuevaPagina->key = string_duplicate(key);
    nuevaPagina->modificada = true;
    time_t tiempoActualHoy;
    elTiempo = (long)time(&tiempoActualHoy);
    nuevaPagina->ultimaVezUsada = elTiempo;
    return nuevaPagina;
}

void insertarEnMemoriaAndActualizarTablaDePaginas(t_pagina* nuevaPagina, char* value, int tamanioPagina, t_dictionary* tablaDePaginas)  {
    strncpy(nuevaPagina->marco->base, value, tamanioPagina - 1);
    strcpy(nuevaPagina->marco->base + tamanioPagina - 1, "\0");
    dictionary_put(tablaDePaginas, nuevaPagina->key, nuevaPagina);
}

t_pagina* lru(t_dictionary* tablaDePaginas) {

    t_pagina* paginaLRU = malloc(sizeof(t_pagina));

    long elTiempo;
    time_t tiempoActual;
    elTiempo = (long) time(&tiempoActual);

    //Uso la hora del momento de la ejecucion para comparar
    paginaLRU->ultimaVezUsada = elTiempo;
    bool encontrePaginaLRU=  false;

    int table_index;
    t_pagina* paginaActual = malloc(sizeof(t_pagina));
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

t_pagina* insert(char* nombreTabla, char* key, char* value, t_memoria* memoria, char* timestamp, t_log* logger,  t_control_conexion* conexionLissandra, pthread_mutex_t* semaforoJournaling){
    char* contenidoPagina = formatearPagina(key, value, timestamp);
    bool recibiTimestamp = timestamp != NULL;

    log_info(logger, "Se insertará el siguiente valor en memoria: %s", contenidoPagina);
    t_pagina* pagina = NULL;
    // tengo la tabla en la memoria?
    if(dictionary_has_key(memoria->tablaDeSegmentos, nombreTabla))   {
        log_info(logger, "La tabla %s se encuentra en memoria", nombreTabla);
        // obtengo el segmento asociado a la tabla en memoria
        t_segmento* segmento = (t_segmento*) dictionary_get(memoria->tablaDeSegmentos, nombreTabla);
        // tengo la key en la tabla de páginas?
        if(dictionary_has_key(segmento->tablaDePaginas, key))   {
            log_info(logger, "La key %s ya existe en la tabla %s. Se procede a modificar su valor.", key, nombreTabla);
            pagina = reemplazarPagina(key, contenidoPagina, memoria->tamanioPagina, segmento->tablaDePaginas);
            fflush(stdout);
        }   else if(hayMarcosLibres(*memoria))   {
            log_info(logger, "La key %s no existe en la tabla %s. Se procede a insertarla.", key, nombreTabla);
            pagina = insertarNuevaPagina(key, contenidoPagina, segmento->tablaDePaginas, memoria, recibiTimestamp);
        }else {
            t_pagina* paginaLRU = lru(segmento->tablaDePaginas);
            if (paginaLRU != NULL){
                pagina = eliminarPaginaLruEInsertarNueva(paginaLRU, key, contenidoPagina,segmento->tablaDePaginas, memoria, recibiTimestamp);
            }else{
                log_info(logger, "La memoria se encuentra FULL");
                gestionarJournal(conexionLissandra, memoria, logger, semaforoJournaling);
                return insert(nombreTabla,key,value,memoria, timestamp,logger,conexionLissandra, semaforoJournaling);
            }
        }
    } else if(hayMarcosLibres(*memoria)) {
        log_info(logger, "La tabla %s no se encuentra en memoria. Se procede a crearla.", nombreTabla);
        t_segmento* nuevoSegmento = crearSegmento(nombreTabla, memoria);
        log_info(logger, "Se procede a insertar el nuevo valor.");
        pagina = insertarNuevaPagina(key, contenidoPagina, nuevoSegmento->tablaDePaginas, memoria, recibiTimestamp);
    } else{
        log_info(logger, "La memoria se encuentra FULL, todavia no se puede crear la nueva tabla");
        gestionarJournal(conexionLissandra,  memoria, logger, semaforoJournaling);
        return insert(nombreTabla, key, value, memoria, timestamp,logger,  conexionLissandra, semaforoJournaling);
    }

    free(contenidoPagina);
    return pagina;
}

void vaciarMemoria(t_memoria* memoria, t_log* logger){
    t_dictionary* tablaDeSegmentos = memoria->tablaDeSegmentos;
    int table_index;
    for (table_index = 0; table_index < tablaDeSegmentos->table_max_size; table_index++) {
        t_hash_element *element = tablaDeSegmentos->elements[table_index];

        if (element != NULL) {
            printf("Nombre de la tabla: %s\n", element->key);
            log_info(logger, drop(element->key, memoria));
            //element = element->next;

        }
    }

}

t_segmento* crearSegmento(char* nombreTabla, t_memoria* memoria)    {
    t_segmento* segmento = (t_segmento*) malloc(sizeof(t_segmento));
    segmento->pathTabla = string_duplicate(nombreTabla);
    segmento->tablaDePaginas = dictionary_create();
    dictionary_put(memoria->tablaDeSegmentos, nombreTabla, segmento);
    return segmento;
}

t_pagina* cmdSelect(char* nombreTabla, char* key, t_memoria* memoria)  {
    t_pagina* resultado = NULL;
    if(dictionary_has_key(memoria->tablaDeSegmentos, nombreTabla))   {
        // obtengo el segmento asociado a la tabla en memoria
        t_segmento* segmento = (t_segmento*) dictionary_get(memoria->tablaDeSegmentos, nombreTabla);
        if(dictionary_has_key(segmento->tablaDePaginas, key))   {
            resultado = (t_pagina*) dictionary_get(segmento->tablaDePaginas, key);
        }
    }

    return resultado;
}

char* drop(char* nombreTabla, t_memoria* memoria)    {
    char* respuesta = "No se encontró la tabla %s en memoria.";
    if(dictionary_has_key(memoria->tablaDeSegmentos, nombreTabla))  {
        t_segmento* segmento = dictionary_get(memoria->tablaDeSegmentos, nombreTabla);
        liberarPaginasSegmento(segmento->tablaDePaginas, memoria);
        dictionary_remove_and_destroy(memoria->tablaDeSegmentos, nombreTabla, &eliminarSegmento);
        respuesta = "Se eliminó la tabla %s de la memoria.";
    }
    return string_from_format(respuesta, nombreTabla);
}

void liberarPaginasSegmento(t_dictionary* tablaDePaginas, t_memoria* memoria)   {
    int cantidadPaginas = dictionary_size(tablaDePaginas);
    dictionary_destroy_and_destroy_elements(tablaDePaginas, &eliminarPagina);
    memoria->marcosOcupados -= cantidadPaginas;
}

void eliminarPagina(void* data)    {
    t_pagina* pagina = (t_pagina*) data;
    free(pagina->key);
    pagina->marco->ocupado = false;
    free(pagina);
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
    char* strMaxValueUint16 = string_itoa(UINT16_MAX);
    int cifrasMaxValueUint16 = strlen(strMaxValueUint16);
    free(strMaxValueUint16);
    time_t tiempoEjemplo;
    long tiempo = time(&tiempoEjemplo);
    char* strTiempo = string_from_format("%lu", tiempo);
    int cifrasTiempo = strlen(strTiempo);
    free(strTiempo);
    //tamaño de INT (timestamp) + tamaño de u_int16_t (key) + tamaño de value (respuesta HS con LS)
    return cifrasMaxValueUint16 + cifrasTiempo + getCantidadCaracteresByPeso(tamanioValue) + 1;
}

//Esta funcion envia la petición del TAM_VALUE a lissandra y devuelve la respuesta del HS
int getTamanioValue(t_control_conexion* conexionLissandra, t_log* logger){
    if(conexionLissandra->fd > 0) {
        hacerHandshake(conexionLissandra->fd, MEMORIA);
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

bool hayMarcosLibres(t_memoria memoria)  {
    return memoria.marcosOcupados < memoria.cantidadTotalMarcos;
}

int getCantidadCaracteresByPeso(int pesoString) {
    return (pesoString / sizeof(char)) - 1;
}

int main(void) {
    char* nombreArchivoConfiguracion = readline("Escriba el nombre del archivo de configuración que desee cargar (el mismo deberá estar en el mismo directorio que el ejecutable).\n");
    t_log* logger = log_create("memoria.log", "memoria", true, LOG_LEVEL_INFO);
    char* nombreArchivoConfiguracionConExtension = string_from_format("%s.cfg", nombreArchivoConfiguracion);
	t_configuracion configuracion = cargarConfiguracion(nombreArchivoConfiguracionConExtension, logger);
	free(nombreArchivoConfiguracionConExtension);

    t_control_conexion conexionKernel = {.fd = 0, .semaforo = (sem_t*) malloc(sizeof(sem_t))};
    t_control_conexion conexionLissandra = {.semaforo = (sem_t*) malloc(sizeof(sem_t))};

    sem_init(conexionKernel.semaforo, 0, 0);
    sem_init(conexionLissandra.semaforo, 0, 0);

    pthread_mutex_t* semaforoMemoriasConocidas = malloc(sizeof(pthread_mutex_t));
    pthread_mutex_init(semaforoMemoriasConocidas, NULL);

    pthread_mutex_t* semaforoJournaling = malloc(sizeof(pthread_mutex_t));
    pthread_mutex_init(semaforoJournaling, NULL);

    printf("\nSoy la memoria con puerto %i\n", configuracion.puerto);

	conectarseALissandra(&conexionLissandra, configuracion.ipFileSystem, configuracion.puertoFileSystem, logger);
	int tamanioValue = getTamanioValue(&conexionLissandra, logger);
    t_memoria* memoriaPrincipal = inicializarMemoriaPrincipal(configuracion, tamanioValue, logger);
	GestorConexiones* misConexiones = inicializarConexion();
    levantarServidor(configuracion.puerto, misConexiones, logger);

    //TODO Agregar "mi ip" al archivo de configuracion para que memorias tenga su propia ip en su lista de gossiping
    agregarIpMemoria(configuracion.ipFileSystem, string_itoa(configuracion.puerto), memoriaPrincipal->memoriasConocidas, logger);

    pthread_t* hiloConexiones = crearHiloConexiones(misConexiones, memoriaPrincipal, &conexionKernel, &conexionLissandra, logger, semaforoMemoriasConocidas, semaforoJournaling);
    pthread_t* hiloConsola = crearHiloConsola(memoriaPrincipal, logger, &conexionLissandra, semaforoJournaling);
    //pthread_t* hiloJournal = crearHiloJournal(memoriaPrincipal, logger, &conexionLissandra, configuracion.retardoJournal, semaforoJournaling);
    pthread_t* hiloGossiping = crearHiloGossiping(misConexiones, memoriaPrincipal, logger, configuracion, semaforoMemoriasConocidas, semaforoJournaling);

    pthread_join(*hiloConexiones, NULL);
    pthread_join(*hiloConsola, NULL);
    //pthread_join(*hiloJournal, NULL);
    pthread_join(*hiloGossiping, NULL);

	return 0;
}