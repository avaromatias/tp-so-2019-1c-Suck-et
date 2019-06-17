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

char* gestionarInsert(char* nombreTabla, char* key, char* valueConComillas, t_memoria* memoria, t_log* logger, t_control_conexion* conexionLissandra)    {
    char* value = string_substring(valueConComillas, 1, strlen(valueConComillas) - 2);
    free(valueConComillas);
    t_pagina* nuevaPagina = insert(nombreTabla, key, value, memoria, NULL, logger, conexionLissandra);
    free(value);
    if (nuevaPagina == NULL){
        return "Memoria FULL";
    } else{
        return string_from_format("Valor insertado: %s", nuevaPagina->marco->base);
    }

}

char* gestionarSelect(char* nombreTabla, char* key, t_control_conexion* conexionLissandra, t_memoria* memoria, t_log* logger) {
    t_pagina* paginaEncontrada = cmdSelect(nombreTabla, key, memoria);
    if(paginaEncontrada != NULL)
        return paginaEncontrada->marco->base;
    char* request = string_from_format("SELECT %s %s", nombreTabla, key);
    log_info(logger, "Valor no encontrado en memoria. Se enviará la siguiente request a Lissandra: %s", request);
    enviarPaquete(conexionLissandra->fd, REQUEST, request);
    free(request);
    t_paquete respuesta = recibirMensaje(conexionLissandra);
    if(respuesta.tipoMensaje == RESPUESTA)   {
        char** componentesSelect = string_split(respuesta.mensaje, ";");
        insert(nombreTabla, key, componentesSelect[2], memoria, componentesSelect[0], logger, conexionLissandra);
        free(respuesta.mensaje);
        return string_from_format("%s%s%s", componentesSelect[0], key, componentesSelect[2]);
    } else
        return respuesta.mensaje;
}

char* gestionarCreate(char* nombreTabla, char* tipoConsistencia, char* cantidadParticiones, char* tiempoCompactacion, t_control_conexion* conexionLissandra, t_log* logger)   {
    char* request = string_from_format("CREATE %s %s %s %s", nombreTabla, tipoConsistencia, cantidadParticiones, tiempoCompactacion);
    enviarPaquete(conexionLissandra->fd, REQUEST, request);
    free(request);
    t_paquete respuesta = recibirMensaje(conexionLissandra);
    return respuesta.mensaje;
}

char* gestionarDrop(char* nombreTabla, int fdLissandra, t_memoria* memoria, t_log* logger)   {
    char* resultado = drop(nombreTabla, memoria);
    log_info(logger, resultado);
    char* request = string_from_format("DROP %s", nombreTabla);
    enviarPaquete(fdLissandra, REQUEST, request);
    return resultado;
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
    printf("Request a enviar a lissandra: %s \n", request);
    t_log* logger = parametrosJournal->logger;


    log_info(logger, "Se procede a enviar a lissandra la request: ", request);
    enviarPaquete(parametrosJournal->conexionLissandra->fd, REQUEST, request);
    free(request);

    t_paquete respuesta = recibirMensaje(parametrosJournal->conexionLissandra);
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


void* gestionarJournal(t_control_conexion* conexionConLissandra, t_memoria* memoria, t_log* logger){

    parametros_journal* parametrosJournal = (parametros_journal*)malloc(sizeof(parametros_journal));
    parametrosJournal->logger = logger;
    parametrosJournal->conexionLissandra = conexionConLissandra;
    mi_dictionary_iterator(parametrosJournal, memoria->tablaDeSegmentos, &iterarSegmentos);

    vaciarMemoria(memoria, logger);
}

char* gestionarRequest(t_comando comando, t_memoria* memoria, t_control_conexion* conexionLissandra, t_log* logger) {
    switch(comando.tipoRequest) {
        case SELECT:
            return gestionarSelect(comando.parametros[0], comando.parametros[1], conexionLissandra, memoria, logger);
        case INSERT:
            return gestionarInsert(comando.parametros[0], comando.parametros[1], comando.parametros[2], memoria, logger, conexionLissandra);
        case DROP:
            return gestionarDrop(comando.parametros[0], conexionLissandra->fd, memoria, logger);
        case CREATE:
            return gestionarCreate(comando.parametros[0], comando.parametros[1], comando.parametros[2], comando.parametros[3], conexionLissandra, logger);
        case JOURNAL:
            gestionarJournal(conexionLissandra, memoria, logger);
            return "se gestionó el journal";
        default:
            return "Comando inválido.";
    }
}

void* ejecutarConsola(void* parametrosConsola){
    parametros_consola_memoria* parametros = (parametros_consola_memoria*) parametrosConsola;

    t_memoria* memoria = parametros->memoria;
    t_log* logger = parametros->logger;
    t_control_conexion* conexionLissandra = parametros->conexionLissandra;
    t_comando comando;


    do {
        char* leido = readline("\nMemoria@suck-ets:~$ ");
//        logearValorDeSemaforo(lissandraConectada, logger, "consola");
        // TODO modularizar hasta el printf
        char** comandoParseado = parser(leido);
        if(comandoParseado == NULL)
            continue;
        comando = instanciarComando(comandoParseado);
        free(comandoParseado);
        if(validarComandosComunes(comando, logger))
            printf("%s", gestionarRequest(comando, memoria, conexionLissandra, logger));
    } while(comando.tipoRequest != EXIT);
    printf("Ya analizamos todo lo solicitado.\n");
}

pthread_t* crearHiloConsola(t_memoria* memoria, t_log* logger, t_control_conexion* conexionLissandra){
    //sem_wait(lissandraConectada);

    pthread_t* hiloConsola = malloc(sizeof(pthread_t));
    parametros_consola_memoria* parametros = (parametros_consola_memoria*) malloc(sizeof(parametros_consola_memoria));

    parametros->logger = logger;
    parametros->memoria = memoria;
    parametros->conexionLissandra = conexionLissandra;

    pthread_create(hiloConsola, NULL, &ejecutarConsola, parametros);
    return hiloConsola;
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
    dictionary_remove_and_destroy(tablaDePaginas, paginaLRU->key, &free);
    memoria->marcosOcupados = memoria->marcosOcupados -1;
    printf("Inserto nueva pagina con key %s y contenido %s\n", keyNueva, nuevoValue);
    paginaNueva = insertarNuevaPagina(keyNueva, nuevoValue, tablaDePaginas, memoria, recibiTimestamp);
    return paginaNueva;

}
t_pagina* reemplazarPagina(char* key, char* nuevoValor, int tamanioPagina, t_dictionary* tablaDePaginas) {

    t_pagina* pagina = dictionary_get(tablaDePaginas, key);
    t_pagina* nuevaPagina = (t_pagina*) malloc(sizeof(t_pagina));
    nuevaPagina->marco = pagina->marco;
    //nuevaPagina->modificada = true;
    time_t elTiempoActual;
    long elTiempo;
    elTiempo = (long) time(&elTiempoActual);
    nuevaPagina->ultimaVezUsada = elTiempo;
    free(pagina);
    strncpy(nuevaPagina->marco->base, nuevoValor, tamanioPagina - 1);
    strcpy(nuevaPagina->marco->base + tamanioPagina - 1, "\0");

    dictionary_put(tablaDePaginas, key, nuevaPagina);
    return nuevaPagina;
}

t_pagina* insertarNuevaPagina(char* key, char* value, t_dictionary* tablaDePaginas, t_memoria* memoria, bool recibiTimestamp) {
    t_pagina* nuevaPagina = crearPagina(key, memoria);
    if (recibiTimestamp){
        printf("Es un select a lissandra, entonces no es una pagina modificada");
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
                    printf("Element key: %s \n", paginaLRU->key);
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

/*bool puedoReemplazarPagina(t_dictionary* tablaDePaginas){
    t_pagina* paginaActual = malloc(sizeof(t_pagina));
    int table_index;
    for (table_index = 0; table_index < tablaDePaginas->table_max_size; table_index++) {
        t_hash_element *element = tablaDePaginas->elements[table_index];

        while (element != NULL) {
            paginaActual = (t_pagina*)element->data;
            //si no fue modificada entonces es una candidata a reemplazar
            if((bool) (paginaActual->modificada) == false){
                return true;
            }
            element = element->next;

        }
    }

    return false;
}*/


t_pagina* insert(char* nombreTabla, char* key, char* value, t_memoria* memoria, char* timestamp, t_log* logger,  t_control_conexion* conexionLissandra){
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
        }   else if(hayMarcosLibres(*memoria))   {
            log_info(logger, "La key %s no existe en la tabla %s. Se procede a insertarla.", key, nombreTabla);
            pagina = insertarNuevaPagina(key, contenidoPagina, segmento->tablaDePaginas, memoria, recibiTimestamp);
        }else {
            printf("Si la pagina no estaba en la tabla y no hay marcos libres intento reemplazarla. \n");

            //Obtengo la pagina a reemplazar si hay
            t_pagina* paginaLRU = lru(segmento->tablaDePaginas);
            if (paginaLRU != NULL){
                pagina = eliminarPaginaLruEInsertarNueva(paginaLRU, key, contenidoPagina,segmento->tablaDePaginas, memoria, recibiTimestamp);
                printf("La pagina nueva tiene la key %s ", pagina->key);
            }else{
                printf("MEMORIA FULL \n");
                gestionarJournal(conexionLissandra, memoria, logger);

                return insert(nombreTabla,key,value,memoria, timestamp,logger,conexionLissandra);
            }
        }
    } else if(hayMarcosLibres(*memoria)) {
        log_info(logger, "La tabla %s no se encuentra en memoria. Se procede a crearla.", nombreTabla);
        t_segmento* nuevoSegmento = crearSegmento(nombreTabla, memoria);
        log_info(logger, "Se procede a insertar el nuevo valor.");
        pagina = insertarNuevaPagina(key, contenidoPagina, nuevoSegmento->tablaDePaginas, memoria, recibiTimestamp);
    } else{
        printf("MEMORIA FULL, No hay espacio para una tabla nueva.\n");
        gestionarJournal(conexionLissandra,  memoria, logger);
        return insert(nombreTabla, key, value, memoria, timestamp,logger,  conexionLissandra);
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
        t_paquete respuesta = recibirMensaje(conexionLissandra);
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


/*void logearValorDeSemaforo(sem_t* unSemaforo, t_log* logger, char* unString){
    int value;
   if (sem_getvalue(unSemaforo, &value) == 0){
       if (strcmp("kernel", unString) == 0){
            log_info(logger, "Se está ejecutando una request por CONSOLA, espero para la ejecucion");
       }
       if (strcmp("consola", unString) == 0){
           log_info(logger, "Se está ejecutando una request de KERNEL, espero para la ejecucion");
       }
   }else{
       if (strcmp("kernel", unString) == 0){
           log_info(logger, "Puedo ejecutar request de KERNEL");
       }
       if (strcmp("consola", unString) == 0){
           log_info(logger, "Puedo ejecutar request por CONSOLA");
       }
   }



}*/

int main(void) {
    t_log* logger = log_create("memoria.log", "memoria", true, LOG_LEVEL_INFO);
	t_configuracion configuracion = cargarConfiguracion("memoria.cfg", logger);

    t_control_conexion conexionKernel = {.fd = 0, .semaforo = (sem_t*) malloc(sizeof(sem_t))};
    t_control_conexion conexionLissandra = {.semaforo = (sem_t*) malloc(sizeof(sem_t))};

    sem_init(conexionKernel.semaforo, 0, 0);
    sem_init(conexionLissandra.semaforo, 0, 0);

	conectarseALissandra(&conexionLissandra, configuracion.ipFileSystem, configuracion.puertoFileSystem, logger);
	int tamanioValue = getTamanioValue(&conexionLissandra, logger);
    t_memoria* memoriaPrincipal = inicializarMemoriaPrincipal(configuracion, tamanioValue, logger);
	GestorConexiones* misConexiones = inicializarConexion();
    levantarServidor(configuracion.puerto, misConexiones, logger);


    pthread_t* hiloConexiones = crearHiloConexiones(misConexiones, &conexionKernel, logger);
    pthread_t* hiloConsola = crearHiloConsola(memoriaPrincipal, logger, &conexionLissandra);
    t_comando comando;

    while(1)    {
		sem_wait(conexionKernel.semaforo);
		if(conexionKernel.fd > 0)	{
			t_paquete request = recibirMensaje(&conexionKernel);
//			logearValorDeSemaforo(&lissandraConectada, logger, "kernel");
			if(request.mensaje == NULL)
				continue;
			else    {
                char** comandoParseado = parser(request.mensaje);
                free(request.mensaje);
                if (comandoParseado == NULL)
                    continue;
                comando = instanciarComando(comandoParseado);
                free(comandoParseado);
                char* respuesta = gestionarRequest(comando, memoriaPrincipal, &conexionLissandra, logger);
                if(respuesta == NULL)   {
                    log_error(logger, "Se desconectó Lissandra. Finalizando el proceso.");
                    exit(-1);
                }
                if(conexionKernel.fd > 0)
                    enviarPaquete(conexionKernel.fd, REQUEST, respuesta);
                sem_post(conexionKernel.semaforo);
			}
		}
    }
    pthread_join(*hiloConexiones, NULL);
    pthread_join(*hiloConsola, NULL);

	return 0;
}