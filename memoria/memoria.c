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
		char* ipFileSystem = config_get_string_value(archivoConfig, "IP_FS");
		configuracion.ipFileSystem = (char*) malloc(pesoString("255.255.255.255"));
        strcpy(configuracion.ipFileSystem, ipFileSystem);
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

t_memoria* inicializarMemoriaPrincipal(t_configuracion configuracion, int tamanioPagina, t_log* logger)    {
    log_info(logger, "Inicia el proceso de inicializar la memoria princial");
    t_memoria* memoriaPrincipal = (t_memoria*) malloc(sizeof(t_memoria));
    memoriaPrincipal->tamanioMemoria = configuracion.tamanioMemoria;
    memoriaPrincipal->direcciones = (char*) malloc(sizeof(char) * configuracion.tamanioMemoria);
    memoriaPrincipal->tablaDeSegmentos = dictionary_create();
    memoriaPrincipal->marcosOcupados = 0;
    memoriaPrincipal->tamanioPagina = tamanioPagina;
    memoriaPrincipal->cantidadTotalMarcos = cantidadTotalMarcosMemoria(*memoriaPrincipal);

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

int gestionarRequest(char **request, t_memoria* memoria, int fdLissandra) {
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

        t_pagina* unaPagina = cmdSelect(nombreTabla, param1, memoria);

        if (unaPagina!= NULL){
            char* value = unaPagina->base;
            printf("Valor hallado: %s", value);
        }else{
            printf("Valor no hallado, se lo pido a lissandra");
            /*char* nuevaRequest = string_new();
            string_append(&nuevaRequest, request[0]);
            string_append(&nuevaRequest, request[1]);
            string_append(&nuevaRequest, request[2]);
            enviarPaquete(fdLissandra, REQUEST, nuevaRequest);

            //TODO nos tienen que responder timestamp;key;value
            char* respuestaMensaje =recibirMensaje(fdLissandra);
            char** respuesta = stringToArray(respuestaMensaje);
            //La respuesta sera del tipo [timestamp, key, value]
            insert(request[1], respuesta[1], respuesta[2]);

            */



        }
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
        insert(nombreTabla, param1, param2, memoria);
        return 0;

    } else if (strcmp(tipoDeRequest, "CREATE") == 0) {
        printf("Tipo de Request: %s\n", tipoDeRequest);
        printf("Tabla: %s\n", nombreTabla);
        printf("TIpo de consistencia: %s\n", param1);
        printf("Numero de particiones: %s\n", param2);
        printf("Tiempo de compactacion: %s\n", param3);
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
        return 0;

    }else if(strcmp(tipoDeRequest, "JOURNAL") == 0){
        printf("Tipo de Request: %s\n", tipoDeRequest);
        return 0;
    } else if (strcmp(tipoDeRequest, "HELP") == 0) {
        printf("************ Comandos disponibles ************\n");
        printf("- SELECT [NOMBRE_TABLA] [KEY]\n");
        printf("- INSERT [NOMBRE_TABLA] [KEY] “[VALUE]” [Timestamp](Opcional)\n");
        printf("- CREATE [NOMBRE_TABLA] [TIPO_CONSISTENCIA] [NUMERO_PARTICIONES] [COMPACTION_TIME]\n");
        printf("- DESCRIBE [NOMBRE_TABLA](Opcional)\n");
        printf("- DROP [NOMBRE_TABLA]\n");
        printf("- JOURNAL\n");
        printf("- EXIT\n");
        return 0;

    } else {
        printf("Ingrese un comando valido.");
        return -2;
    }

}

char** stringToArray(char* unString){
    return string_split(unString, ';');
}

void ejecutarConsola(void* parametrosConsola){

    parametros_consola_memoria* parametros = (parametros_consola_memoria*) parametrosConsola;

    t_memoria* memoria = parametros->memoria;
    t_log* logger = parametros->logger;
    int fdLissandra = parametros->fdLissandra;

    char* comando;
    char* nombreDelGrupo = "@suck-ets:~$ ";
    char* prompt = string_new();
    string_append(&prompt, "Memoria");
    string_append(&prompt, nombreDelGrupo);
    do {
        char* leido = readline(prompt);
        comando = malloc(sizeof(char) * (strlen(leido) + 1));
        memcpy(comando, leido, strlen(leido));
        comando[strlen(leido)] = '\0';
        char** comandoParseado = parser(comando);
        if(validarComandosComunes(comandoParseado)== 1){
            if(gestionarRequest(comandoParseado, memoria, fdLissandra) == 0){
                log_info(logger, "Request procesada correctamente.");
            } else {
                log_error(logger, "No se pudo procesar la request solicitada.");
            };
        }
        string_to_lower(comando);
    } while(strcmp(comando, "exit") != 0);
    free(comando);
    printf("Ya analizamos todo lo solicitado.\n");
}

pthread_t* crearHiloConsola(t_memoria* memoria, t_log* logger){
    pthread_t* hiloConsola = malloc(sizeof(pthread_t));
    parametros_consola_memoria* parametros = (parametros_consola_memoria*) malloc(sizeof(parametros_consola_memoria));

    parametros->logger = logger;
    parametros->memoria = memoria;

    pthread_create(hiloConsola, NULL, &ejecutarConsola, parametros);
    return hiloConsola;
}

char* formatearPagina(char* key, char* value)   {
    time_t tiempo;
    return string_from_format("%i%s%s", (int) time(&tiempo), key, value);
}

char* reemplazarPagina(char* key, char* nuevoValor, t_dictionary* tablaDePaginas) {
    t_pagina* pagina = dictionary_get(tablaDePaginas, key);
    t_pagina* nuevaPagina = (t_pagina*) malloc(sizeof(t_pagina));
    nuevaPagina->base = pagina->base;
    free(pagina);
    strcpy(nuevaPagina->base, nuevoValor);
    // reemplazo la página encontrada en la tabla de páginas
    dictionary_put(tablaDePaginas, key, nuevoValor);
    return nuevaPagina->base;
}

char* insertarNuevaPagina(char* key, char* value, t_dictionary* tablaDePaginas, t_memoria* memoria) {
    t_pagina* nuevaPagina = crearPagina(key, memoria);
    insertarEnMemoriaAndActualizarTablaDePaginas(nuevaPagina, value, tablaDePaginas);
    return nuevaPagina->base;
}

t_pagina* crearPagina(char* key, t_memoria* memoria)  {
    t_pagina* nuevaPagina = (t_pagina*) malloc(sizeof(t_pagina));
    nuevaPagina->base = getMarcoLibre(memoria);
    nuevaPagina->key = string_duplicate(key);
    nuevaPagina->modificada = true;
    return nuevaPagina;
}

void insertarEnMemoriaAndActualizarTablaDePaginas(t_pagina* nuevaPagina, char* value, t_dictionary* tablaDePaginas)  {
    strcpy(nuevaPagina->base, value);
    dictionary_put(tablaDePaginas, nuevaPagina->key, nuevaPagina);
}

char* insert(char* nombreTabla, char* key, char* value, t_memoria* memoria)   {
    char* contenidoPagina = formatearPagina(key, value);
    // tengo la tabla en la memoria?
    if(dictionary_has_key(memoria->tablaDeSegmentos, nombreTabla))   {
        // obtengo el segmento asociado a la tabla en memoria
        t_segmento* segmento = (t_segmento*) dictionary_get(memoria->tablaDeSegmentos, nombreTabla);
        // tengo la key en la tabla de páginas?
        if(dictionary_has_key(segmento->tablaDePaginas, key))   {
            reemplazarPagina(key, contenidoPagina, segmento->tablaDePaginas);
        }   else if(hayMarcosLibres(*memoria))   {
            return insertarNuevaPagina(key, contenidoPagina, segmento->tablaDePaginas, memoria);
        }
    } else  {
        t_segmento* nuevoSegmento = crearSegmento(nombreTabla, memoria);
        return insertarNuevaPagina(key, contenidoPagina, nuevoSegmento->tablaDePaginas, memoria);
    }
    free(contenidoPagina);
}

// creo un segmento con una página

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
        if(dictionary_has_key(segmento->tablaDePaginas, key))
            resultado = (t_pagina*) dictionary_get(segmento->tablaDePaginas, key);
    }
    return resultado;
}


int main(void) {
    t_log* logger = log_create("memoria.log", "memoria", true, LOG_LEVEL_INFO);
	t_configuracion configuracion = cargarConfiguracion("memoria.cfg", logger);

    sem_t kernelConectado;
    sem_t lissandraConectada;
    sem_init(&kernelConectado, 0, 0);
    sem_init(&lissandraConectada, 0, 0);

    int fdKernel = 0;
	int fdLissandra = conectarseALissandra(configuracion.ipFileSystem, configuracion.puertoFileSystem, &lissandraConectada, logger);
	int tamanioPagina = calcularTamanioDePagina(getTamanioValue(fdLissandra, logger));
    t_memoria* memoriaPrincipal = inicializarMemoriaPrincipal(configuracion, tamanioPagina, logger);
	GestorConexiones* misConexiones = inicializarConexion();
    levantarServidor(configuracion.puerto, misConexiones, logger);

    insert("tableA", "5", "QUÉ ONDA GUACHOOOOO", memoriaPrincipal);
    insert("tableB", "2", "qué onda guachín", memoriaPrincipal);
    insert("tableA", "3", "lalalalala", memoriaPrincipal);

    char* direccion = cmdSelect("tableA", "5", memoriaPrincipal)->base;
    char* otraPalabra = cmdSelect("tableA", "3", memoriaPrincipal)->base;
    char* word = cmdSelect("tableB", "2", memoriaPrincipal)->base;

    printf("%s\n", direccion);
    printf("%s\n", otraPalabra);
    printf("%s\n", word);
    insert("tableA", "3", "ñacañacañaca", memoriaPrincipal);
    printf("%s\n", otraPalabra);
    fflush(stdout);

    pthread_t* hiloConexiones = crearHiloConexiones(misConexiones, &fdKernel, &kernelConectado, logger);
    pthread_t* hiloConsola = crearHiloConsola(memoriaPrincipal, logger);

    while(1){
		sem_wait(&kernelConectado);
		sem_wait(&lissandraConectada);
		if(fdKernel > 0)	{
			char* request = recibirMensaje(&fdKernel);
			if(request == NULL) {
			    // tengo que habilitar el semáforo de Lissandra por si ésta sigue conectada, si no lo sigue se lo deshabilitará en otra vuelta del ciclo
			    sem_post(&lissandraConectada);
				continue;
			}
			else    {
				sem_post(&kernelConectado);
				if(fdLissandra > 0)
				    enviarPaquete(fdLissandra, REQUEST, request);
                char* respuestaLissandra = recibirMensaje(&fdLissandra);
                if(respuestaLissandra == NULL)	{
                	continue;
                }
                else	{
                	if(fdKernel > 0)
                		enviarPaquete(fdKernel, REQUEST, respuestaLissandra);
                }
			}
		}
    }
    pthread_join(*hiloConexiones, NULL);
//    pthread_join(*hiloConsola, NULL);

	return 0;
}

int cantidadTotalMarcosMemoria(t_memoria memoria)   {
    return memoria.tamanioMemoria / memoria.tamanioPagina;
}

int calcularTamanioDePagina(int tamanioValue){
    //tamaño de INT (timestamp) + tamaño de u_int16_t (key) + tamaño de value (respuesta HS con LS)
    return sizeof(time_t) + sizeof(u_int16_t) + tamanioValue;
}

//Esta funcion envia la petición del TAM_VALUE a lissandra y devuelve la respuesta del HS
int getTamanioValue(int fdLissandra, t_log* logger){
    return 4;
    hacerHandshake(fdLissandra, MEMORIA);
    int tamanioValue = (int) recibirMensaje(&fdLissandra);
    if(tamanioValue != NULL)
        log_info(logger, "Tamaño del value obtenido de Lissandra: %i", tamanioValue);
    else
        log_error(logger, "Hubo un error al intentar obtener el tamaño del value de Lissandra.");
    return tamanioValue;
}

char* getMarcoLibre(t_memoria* memoria)   {
    t_marco* marcoLeido = memoria->tablaDeMarcos;
    for(int i = 0; i < memoria->cantidadTotalMarcos; i++)   {
        if(!marcoLeido->ocupado)    {
            marcoLeido->ocupado = true;
            memoria->marcosOcupados++;
            return marcoLeido->base;
        } else
            marcoLeido += memoria->tamanioPagina;
    }

    return NULL;
}

bool hayMarcosLibres(t_memoria memoria)  {
    return memoria.marcosOcupados < memoria.cantidadTotalMarcos;
}