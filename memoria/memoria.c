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

char* gestionarInsert(char* nombreTabla, char* key, char* value, t_memoria* memoria)    {
    t_pagina* nuevaPagina = insert(nombreTabla, key, value, memoria);
    return string_from_format("Valor insertado: %s", nuevaPagina->marco->base);
}

char* gestionarSelect(char* nombreTabla, char* key, int fdLissandra, t_memoria* memoria) {
    t_pagina* paginaEncontrada = cmdSelect(nombreTabla, key, memoria);
    if(paginaEncontrada != NULL)
        return paginaEncontrada->marco->base;
    char* request = string_from_format("select %s %s", nombreTabla, key);
    enviarPaquete(fdLissandra, REQUEST, request);
    free(request);
    return recibirMensaje(&fdLissandra);
}

char* gestionarRequest(t_comando comando, t_memoria* memoria, int fdLissandra) {

    switch(comando.tipoRequest) {
        case SELECT:
            return gestionarSelect(comando.parametros[0], comando.parametros[1], fdLissandra, memoria);
        case INSERT:
            return gestionarInsert(comando.parametros[0], comando.parametros[1], comando.parametros[2], memoria);
//        case DROP:
//            return gestionarDrop(comando.parametros[0], memoria);
//        case CREATE:
//            return gestionarCreate(comando.parametros[0], comando.parametros[1], comando.parametros[2], comando.parametros[3], memoria);
        default:
            return "Comando inválido.";
    }
}

void* ejecutarConsola(void* parametrosConsola){
    parametros_consola_memoria* parametros = (parametros_consola_memoria*) parametrosConsola;

    t_memoria* memoria = parametros->memoria;
    t_log* logger = parametros->logger;
    int fdLissandra = parametros->fdLissandra;
    sem_t* lissandraConectada = (sem_t* ) parametros->lissandraConectada;
    t_comando comando;


    do {


        char* leido = readline("Memoria@suck-ets:~$ ");
        sem_wait(lissandraConectada);
        char** comandoParseado = parser(leido);

        if (comandoParseado == NULL){
            printf("Alguno de los parámetros ingresados es incorrecto. Por favor verifique su entrada.\n");
            continue;
        }

        string_trim(comandoParseado);
        comando = instanciarComando(comandoParseado);
        free(comandoParseado);
        printf(validarComandosComunes(comando)? "%s" : "Alguno de los parámetros ingresados es incorrecto. Por favor verifique su entrada.\n", gestionarRequest(comando, memoria, fdLissandra));
        sem_post(lissandraConectada);
    } while(comando.tipoRequest != EXIT);
    printf("Ya analizamos todo lo solicitado.\n");
}

pthread_t* crearHiloConsola(t_memoria* memoria, t_log* logger, int fdLissandra, sem_t* lissandraConectada ){
    //sem_wait(lissandraConectada);

    pthread_t* hiloConsola = malloc(sizeof(pthread_t));
    parametros_consola_memoria* parametros = (parametros_consola_memoria*) malloc(sizeof(parametros_consola_memoria));

    parametros->logger = logger;
    parametros->memoria = memoria;
    parametros->fdLissandra = fdLissandra;
    parametros->lissandraConectada = lissandraConectada;

    pthread_create(hiloConsola, NULL, &ejecutarConsola, parametros);
    return hiloConsola;
}

char* formatearPagina(char* key, char* value)   {
    time_t tiempo;
    return string_from_format("%i%s%s", (int) time(&tiempo), key, value);
}

t_pagina* reemplazarPagina(char* key, char* nuevoValor, t_dictionary* tablaDePaginas) {
    t_pagina* pagina = dictionary_get(tablaDePaginas, key);
    t_pagina* nuevaPagina = (t_pagina*) malloc(sizeof(t_pagina));
    nuevaPagina->marco = pagina->marco;
    free(pagina);
    strcpy(nuevaPagina->marco->base, nuevoValor);
    // reemplazo la página encontrada en la tabla de páginas
    dictionary_put(tablaDePaginas, key, nuevaPagina);
    return nuevaPagina;
}

t_pagina* insertarNuevaPagina(char* key, char* value, t_dictionary* tablaDePaginas, t_memoria* memoria) {
    t_pagina* nuevaPagina = crearPagina(key, memoria);
    insertarEnMemoriaAndActualizarTablaDePaginas(nuevaPagina, value, tablaDePaginas);
    return nuevaPagina;
}

t_pagina* crearPagina(char* key, t_memoria* memoria)  {
    t_pagina* nuevaPagina = (t_pagina*) malloc(sizeof(t_pagina));
    nuevaPagina->marco = getMarcoLibre(memoria);
    nuevaPagina->key = string_duplicate(key);
    nuevaPagina->modificada = true;
    return nuevaPagina;
}

void insertarEnMemoriaAndActualizarTablaDePaginas(t_pagina* nuevaPagina, char* value, t_dictionary* tablaDePaginas)  {
    strcpy(nuevaPagina->marco->base, value);
    dictionary_put(tablaDePaginas, nuevaPagina->key, nuevaPagina);
}

t_pagina* insert(char* nombreTabla, char* key, char* value, t_memoria* memoria)   {
    char* contenidoPagina = formatearPagina(key, value);
    t_pagina* pagina = NULL;
    // tengo la tabla en la memoria?
    if(dictionary_has_key(memoria->tablaDeSegmentos, nombreTabla))   {
        // obtengo el segmento asociado a la tabla en memoria
        t_segmento* segmento = (t_segmento*) dictionary_get(memoria->tablaDeSegmentos, nombreTabla);
        // tengo la key en la tabla de páginas?
        if(dictionary_has_key(segmento->tablaDePaginas, key))   {
            pagina = reemplazarPagina(key, contenidoPagina, segmento->tablaDePaginas);
        }   else if(hayMarcosLibres(*memoria))   {
            pagina = insertarNuevaPagina(key, contenidoPagina, segmento->tablaDePaginas, memoria);
        }
//        else {
//            hacerJournaling();
//            insert(nombreTabla, key, value, memoria);
//        }
    } else if(hayMarcosLibres(*memoria)) {
        t_segmento* nuevoSegmento = crearSegmento(nombreTabla, memoria);
        pagina = insertarNuevaPagina(key, contenidoPagina, nuevoSegmento->tablaDePaginas, memoria);
    }
//    else {
//        hacerJournaling();
//        insert(nombreTabla, key, value, memoria);
//    }

    free(contenidoPagina);
    return pagina;
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
        if(dictionary_has_key(segmento->tablaDePaginas, key))   {
            resultado = (t_pagina*) dictionary_get(segmento->tablaDePaginas, key);
        }
    }

    return resultado;
}

void drop(char* nombreTabla, t_memoria* memoria)    {
    if(dictionary_has_key(memoria->tablaDeSegmentos, nombreTabla))  {
        t_segmento* segmento = dictionary_get(memoria->tablaDeSegmentos, nombreTabla);
        liberarPaginasSegmento(segmento->tablaDePaginas, memoria);
    }
}

void liberarPaginasSegmento(t_dictionary* tablaDePaginas, t_memoria* memoria)   {
    memoria->marcosOcupados -= dictionary_size(tablaDePaginas);
    dictionary_destroy_and_destroy_elements(tablaDePaginas, &eliminarPagina);
    free(tablaDePaginas);
}

void eliminarPagina(void* data)    {
    t_pagina* pagina = (t_pagina*) data;
    free(pagina->key);
    pagina->marco->ocupado = false;
    free(pagina);
}

int cantidadTotalMarcosMemoria(t_memoria memoria)   {
    return memoria.tamanioMemoria / memoria.tamanioPagina;
}

int calcularTamanioDePagina(int tamanioValue){
    //tamaño de INT (timestamp) + tamaño de u_int16_t (key) + tamaño de value (respuesta HS con LS)
    return sizeof(time_t) + sizeof(uint16_t) + tamanioValue;
}

//Esta funcion envia la petición del TAM_VALUE a lissandra y devuelve la respuesta del HS
int getTamanioValue(int fdLissandra, t_log* logger){
    return 4;
//    hacerHandshake(fdLissandra, MEMORIA);
//    int tamanioValue = (int) recibirMensaje(&fdLissandra);
//    if(tamanioValue != NULL)
//        log_info(logger, "Tamaño del value obtenido de Lissandra: %i", tamanioValue);
//    else
//        log_error(logger, "Hubo un error al intentar obtener el tamaño del value de Lissandra.");
//    return tamanioValue;
}

t_marco* getMarcoLibre(t_memoria* memoria)   {
    t_marco* marcoLeido = memoria->tablaDeMarcos;
    for(int i = 0; i < memoria->cantidadTotalMarcos; i++)   {
        if(!marcoLeido->ocupado)    {
            marcoLeido->ocupado = true;
            memoria->marcosOcupados++;
            return marcoLeido;
        } else
            marcoLeido += memoria->tamanioPagina;
    }

    return NULL;
}

bool hayMarcosLibres(t_memoria memoria)  {
    return memoria.marcosOcupados < memoria.cantidadTotalMarcos;
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

    t_pagina* unaPagina = insert("tableA", "5", "corki", memoriaPrincipal);
    t_pagina* paginaDos = insert("tableB", "2", "shaco", memoriaPrincipal);
    t_pagina* paginaTres = insert("tableA", "3", "lissandra", memoriaPrincipal);

    char* direccion = cmdSelect("tableA", "5", memoriaPrincipal)->marco->base;
    char* otraPalabra = cmdSelect("tableA", "3", memoriaPrincipal)->marco->base;
    char* word = cmdSelect("tableB", "2", memoriaPrincipal)->marco->base;

    printf("%s\n", direccion);
    printf("%s\n", otraPalabra);
    printf("%s\n", word);
    insert("tableA", "3", "Teemo", memoriaPrincipal);
    printf("%s\n", otraPalabra);
    fflush(stdout);

//    drop("tableA", memoriaPrincipal);
//    drop("tableB", memoriaPrincipal);

    //Simulo que está conectado kernel
    //sem_post(&kernelConectado);

    pthread_t* hiloConexiones = crearHiloConexiones(misConexiones, &fdKernel, &kernelConectado, logger);
    pthread_t* hiloConsola = crearHiloConsola(memoriaPrincipal, logger, fdLissandra, &lissandraConectada);

    while(1)    {
		sem_wait(&kernelConectado);

		if(fdKernel > 0)	{
			char* request = recibirMensaje(&fdKernel);

            sem_wait(&lissandraConectada);
			if(request == NULL) {
			    // tengo que habilitar el semáforo de Lissandra por si ésta sigue conectada, si no lo sigue se lo deshabilitará en otra vuelta del ciclo
			    sem_post(&lissandraConectada);
			    //request == NULL es que se desconecto kernel?
				continue;
			}
			else    {
				sem_post(&kernelConectado);
				if(fdLissandra > 0)
				    enviarPaquete(fdLissandra, REQUEST, request);
                char* respuestaLissandra = recibirMensaje(&fdLissandra);
                if(respuestaLissandra == NULL)	{
                    sem_post(&lissandraConectada);
                	continue;
                }
                else	{
                	if(fdKernel > 0)
                		enviarPaquete(fdKernel, REQUEST, respuestaLissandra);
                        sem_post(&lissandraConectada);
                }
			}
		}
    }
    pthread_join(*hiloConexiones, NULL);
    pthread_join(*hiloConsola, NULL);

	return 0;
}