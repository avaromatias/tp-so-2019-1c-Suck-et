//
// Created by utnso on 12/05/19.
//

#include "conexiones.h"

pthread_t *crearHiloConexiones(GestorConexiones *unaConexion, t_log *logger, t_dictionary *tablaDeMemoriasConCriterios,
                               t_dictionary *metadataTabla, pthread_mutex_t *mutexJournal, t_dictionary *visorDeHilos, t_list* memoriasConocidas) {
    pthread_t *hiloConexiones = malloc(sizeof(pthread_t));

    parametros_thread_k *parametros = (parametros_thread_k *) malloc(sizeof(parametros_thread_k));

    parametros->tablaDeMemoriasConCriterios = tablaDeMemoriasConCriterios;
    parametros->metadataTabla = metadataTabla;
    parametros->conexion = unaConexion;
    parametros->logger = logger;
    parametros->mutexJournal = mutexJournal;
    parametros->supervisorDeHilos = visorDeHilos;
    parametros->memoriasConocidas = memoriasConocidas;

    pthread_create(hiloConexiones, NULL, &atenderConexiones, parametros);

    return hiloConexiones;
}

void *atenderConexiones(void *parametrosThread) {
    parametros_thread_k *parametros = (parametros_thread_k *) parametrosThread;
    GestorConexiones *unaConexion = parametros->conexion;
    t_log *logger = parametros->logger;
    t_dictionary *tablaDeMemoriasConCriterios = parametros->tablaDeMemoriasConCriterios;
    t_dictionary *metadata = parametros->metadataTabla;
    t_dictionary *supervisorDeHilos = parametros->supervisorDeHilos;
    pthread_mutex_t *mutexJournal = parametros->mutexJournal;
    t_list* memoriasConocidas = (t_list*) parametros->memoriasConocidas;

    char **direccionesNuevasMemorias;
    t_list *listaDeNodosMemorias = list_create();

    fd_set emisores;

    while (1) {
        if (hayNuevoMensaje(unaConexion, &emisores)) {
            // voy a recorrer todos los FD a los cuales estoy conectado para saber cuál de todos es el que tiene un nuevo mensaje
            for (int i = 0; i < list_size(unaConexion->conexiones); i++) {
                Header headerSerializado;
                int fdConectado = *((int *) list_get(unaConexion->conexiones, i));

                if (FD_ISSET(fdConectado, &emisores)) {
                    int bytesRecibidos = recv(fdConectado, &headerSerializado, sizeof(Header), MSG_DONTWAIT);

                    switch (bytesRecibidos) {
                        // hubo un error al recibir los datos
                        case -1:
                            log_warning(logger, "Hubo un error al recibir el header proveniente del socket %i",
                                        fdConectado);
                            break;
                            // se desconectó
                        case 0:
                            // acá cada uno setea una maravillosa función que hace cada uno cuando se le desconecta alguien
                            // nombre_maravillosa_funcion();
                            eliminarFileDescriptorDeTablasDeMemorias(fdConectado, tablaDeMemoriasConCriterios,
                                                                     mutexJournal);
                            desconectarCliente(fdConectado, unaConexion, logger);
                            break;
                            // recibí algún mensaje
                        default:; // esto es lo más raro que vi pero tuve que hacerlo
                            Header header = deserializarHeader(&headerSerializado);
                            header.fdRemitente = fdConectado;
                            int pesoMensaje = header.tamanioMensaje * sizeof(char);
                            char *mensaje = (char *) malloc(pesoMensaje);
                            bytesRecibidos = recv(fdConectado, mensaje, pesoMensaje, MSG_DONTWAIT);
                            if (bytesRecibidos == -1 || bytesRecibidos < pesoMensaje)
                                log_warning(logger, "Hubo un error al recibir el mensaje proveniente del socket %i",
                                            fdConectado);
                            else if (bytesRecibidos == 0) {
                                // acá cada uno setea una maravillosa función que hace cada uno cuando se le desconecta alguien
                                // nombre_maravillosa_funcion();
                                eliminarFileDescriptorDeTablasDeMemorias(fdConectado, tablaDeMemoriasConCriterios,
                                                                         mutexJournal);
                                desconectarCliente(fdConectado, unaConexion, logger);
                            } else {
                                switch (header.tipoMensaje) {
                                    case RESPUESTA:
                                        gestionarRespuesta(header.fdRemitente, header.pid, header.tipoRequest,
                                                           supervisorDeHilos,
                                                           tablaDeMemoriasConCriterios, metadata, mensaje, logger);
                                        break;
                                    case CONEXION_ACEPTADA:
                                        //atenderHandshake(header, componente, parametros);
                                        log_info(logger,
                                                 "La memoria conectada recientemente ya se encuentra disponible para ser utilizada.\n");
                                        break;
                                        //Componente componente = *((Componente *) mensaje);
                                        //atenderHandshake(header, componente, parametros);
                                    case CONEXION_RECHAZADA:
                                        break;
                                    case RESPUESTA_GOSSIPING:
                                        //direccionesNuevasMemorias = obtenerDatosDeConexion(mensaje);
                                        //identificarMemorias(listaDeNodosMemorias, direccionesNuevasMemorias, logger);
                                        agregarMemoriasRecibidas(mensaje, memoriasConocidas, logger);
                                        break;

                                    case ERR:;
                                        char *PID = string_itoa(header.pid);
                                        pthread_mutex_t *semaforo = (pthread_mutex_t *) dictionary_get(
                                                supervisorDeHilos, PID);
                                        pthread_mutex_unlock(semaforo);
                                        free(PID);
                                        break;
                                }
                                // acá cada uno setea una maravillosa función que hace cada uno cuando le llega un nuevo mensaje
                                // nombre_maravillosa_funcion();
                            }
                            break;
                    }
                }
            }
            // me fijo si hay algún nuevo conectado
            if (FD_ISSET(unaConexion->servidor, &emisores)) {
                int *fdNuevoCliente = malloc(sizeof(int));
                *fdNuevoCliente = aceptarCliente(unaConexion->servidor, logger);
                list_add(unaConexion->conexiones, fdNuevoCliente);

                //me fijo si hay que actualizar el file descriptor máximo con el del nuevo cliente
                unaConexion->descriptorMaximo = getFdMaximo(unaConexion);

                // acá cada uno setea una maravillosa función que hace cada uno cuando se le conecta un nuevo cliente
                // nombre_maravillosa_funcion();
            }
        }
    }
}

/*void identificarMemorias(t_list *listaDeNodosMemorias, char **direccionesNuevasMemorias, t_log *logger) {

    int cantidadMemoriasReconocidas = tamanioDeArrayDeStrings(direccionesNuevasMemorias);
    char **ipYPuerto;

    for (int i = 0; i < cantidadMemoriasReconocidas; i++) {
        t_nodoMemoria *nodoDatosDeMemoria = (t_nodoMemoria *) malloc(sizeof(t_nodoMemoria));
        ipYPuerto = string_split(direccionesNuevasMemorias[i], ":");

        if (esString(ipYPuerto[0]) && esEntero(ipYPuerto[1])) {
            nodoDatosDeMemoria->ipNodoMemoria = ipYPuerto[0];
            nodoDatosDeMemoria->puertoNodoMemoria = ipYPuerto[1];
            if (tenemosMemoriaEnListaDeMemorias(listaDeNodosMemorias, nodoDatosDeMemoria)) {
                log_warning(logger,
                            "La memoria ya se está en nuestra lista de memorias. No procederemos a conectarnos.\n");
            } else {
                list_add(listaDeNodosMemorias, nodoDatosDeMemoria);
            }
        }
    }
}

void conectarseANodosPorGossiping(t_list *listaDeNodosMemorias, ) {

}

bool tenemosMemoriaEnListaDeMemorias(t_list *listaDeNodosMemorias, t_nodoMemoria *nodoDatosDeMemoria) {

    bool memoriaEncontrada(void *elemento) {
        t_nodoMemoria *nodoAVerificar = elemento;
        char *ip = nodoAVerificar->ipNodoMemoria;

        if (strcmp(nodoDatosDeMemoria->ipNodoMemoria, ip)) {
            return true;
        } else {
            return false;
        }
    }
    if (list_find(listaDeNodosMemorias, memoriaEncontrada(nodoDatosDeMemoria)) != NULL) {
        return true;
    } else return false;

}*/

int conectarseAMemoriaPrincipal(char *ipMemoria, int puertoMemoria, GestorConexiones *misConexiones, t_log *logger) {
    int fdMemoria = conectarseAServidor(ipMemoria, puertoMemoria, misConexiones, logger);
    if (fdMemoria < 0) {
        log_error(logger, "Hubo un error al intentar conectarse a la Memoria Principal. Cerrando el proceso...");
        exit(-1);
    } else {
        hacerHandshake(fdMemoria, KERNEL);
        log_info(logger, "Conexión con Memoria Principal establecida.");
    }
    return fdMemoria;
}

void actualizarMetadata(t_dictionary *metadataTablas, char *mensaje, t_log *logger) {
    char **tablaARenovar;
    char **consistenciaTabla;
    char **tablasDelDescribe;
    char *dataLissandra;
    char *nombreTabla;
    char *consistencia;
    dataLissandra = mensaje;

    if ((dataLissandra != NULL)) {
        int cantTablasActualizadas = 0;
        tablasDelDescribe = string_split(dataLissandra, "-----------------");
        int cantidadDeTablasActualizar = tamanioDeArrayDeStrings(tablasDelDescribe);

        for (int i = 0; i < cantidadDeTablasActualizar; i++) {
            char **infoTabla = string_split(tablasDelDescribe[i], "\n");

            if (string_contains(infoTabla[0], "TABLE=")) {
                tablaARenovar = string_split(infoTabla[0], "=");
                nombreTabla = tablaARenovar[1]; //El 0 es el TABLE
            } else {
                log_warning(logger, "No existe tabla %s a ejecutar", nombreTabla);
                break;
            }
            if (string_contains(infoTabla[1], "CONSISTENCY=")) {
                consistenciaTabla = string_split(infoTabla[1], "=");
                consistencia = consistenciaTabla[1]; //El 0 es el CONSISTENCY
            } else {
                log_warning(logger, "La tabla %s no posee consistencia.", infoTabla[0]);
                break;
            }
            dictionary_put(metadataTablas, nombreTabla, consistencia);
            cantTablasActualizadas++;
        }
        log_info(logger, "Cantidad de Tablas actualizadas %i", cantTablasActualizadas);
    } else {
        log_error(logger, "La información recibida por Lissandra es NULA.\n");
        exit(-1);
    }
}

void crearTablaEnMetadata(t_dictionary *metadataTablas, char *mensaje, t_log *logger) {
    char *dataCreateEnLissandra;
    char **infoCreate;
    char **separacionTablaConsistencia;
    char *nombreTabla;
    char *consistencia;
    char *tablaConsistencia;

    dataCreateEnLissandra = mensaje; //“CREATE OK|tabla;consistencia”
    if ((dataCreateEnLissandra != NULL)) {
        infoCreate = string_split(dataCreateEnLissandra, "|");
        if (string_contains(infoCreate[0], "CREATE OK")) {
            tablaConsistencia = infoCreate[1];
        } else {
            log_warning(logger, "El CREATE para la tabla/consistencia no se pudo crear.");
            exit(-1);
        }
        if (string_contains(tablaConsistencia, ";")) {
            separacionTablaConsistencia = string_split(tablaConsistencia, ";");
            nombreTabla = separacionTablaConsistencia[0];
            consistencia = separacionTablaConsistencia[1];
        }
        dictionary_put(metadataTablas, nombreTabla, consistencia);
        log_info(logger, "La metadata se actualizó correctamente");
    }
}

void gestionarRespuesta(int fdMemoria, int pid, TipoRequest tipoRequest, t_dictionary *supervisorDeHilos,
                        t_dictionary *memoriasConCriterios, t_dictionary *metadata, char *mensaje, t_log *logger) {

    char *PIDCasteado = string_itoa(pid);
    pthread_mutex_t *semaforoADesbloquear;
    if (dictionary_has_key(supervisorDeHilos, PIDCasteado)) {
        semaforoADesbloquear = (pthread_mutex_t *) dictionary_get(supervisorDeHilos, PIDCasteado);
    } else {
        semaforoADesbloquear = paramPlanificacionGeneral->parametrosPCP->mutexSemaforoHilo;
        pthread_mutex_lock(semaforoADesbloquear);
        dictionary_put(supervisorDeHilos, PIDCasteado, semaforoADesbloquear);
    }

    switch (tipoRequest) {
        case SELECT:
            //incrementoCantidadSelects procesados para metricas;
            log_info(logger, "El SELECT enviado a la memoria %i fue procesado correctamente. Respuesta recibida: %s",
                     fdMemoria, mensaje);
            printf("hola");
            break;
        case INSERT:
            //incrementoCantidadInserts procesados para metricas;
            log_info(logger, "El INSERT enviado a la memoria %i fue procesado correctamente.", fdMemoria);
            break;
        case CREATE:
            //incrementoCantidadCreates procesados para metricas;
            crearTablaEnMetadata(metadata, mensaje, logger);
            log_info(logger, "El CREATE enviado a la memoria %i fue procesado correctamente.", fdMemoria);
            break;
        case DROP:
            log_info(logger, "El DROP enviado a la memoria %i fue procesado correctamente.", fdMemoria);
            break;
        case DESCRIBE:
            actualizarMetadata(metadata, mensaje, logger);
            log_info(logger, "El DESCRIBE enviado a la memoria %i fue procesado correctamente.", fdMemoria);
            break;
    }
    pthread_mutex_unlock(semaforoADesbloquear);
    free(PIDCasteado);
}

/*void borrarFdDeListaDeFdsConectados(int fdMemoria, t_dictionary *tablaDeMemoriasConCriterios, char *criterio) {

    char *fdADesconectar = string_itoa(fdMemoria);

    bool memoriaEncontrada(void *elemento) {
        char *elementoAComparar = string_itoa((int) elemento);

        if (elemento != NULL) {
            return (strcmp(fdADesconectar, elementoAComparar) == 0);
        } else {
            return false;
        }
    }
    t_list *listaDeMemoriasConectadasACriterio = dictionary_get(tablaDeMemoriasConCriterios, criterio);
    dictionary_remove(tablaDeMemoriasConCriterios, criterio);
    int indiceASacar = (int) list_find(listaDeMemoriasConectadasACriterio, memoriaEncontrada(fdADesconectar));
    if (indiceASacar != 0) {
        list_remove(listaDeMemoriasConectadasACriterio, indiceASacar);
        dictionary_put(tablaDeMemoriasConCriterios, criterio, listaDeMemoriasConectadasACriterio);
    }
}*/

void eliminarFileDescriptorDeTablasDeMemorias(int fdDesconectado, t_dictionary *tablaDeMemoriasConCriterios,
                                              pthread_mutex_t *mutexJournal) {

    pthread_mutex_lock(mutexJournal);

    if (dictionary_has_key(tablaDeMemoriasConCriterios, "SC")) {
        char *criterio = "SC";
        //borrarFdDeListaDeFdsConectados(fdDesconectado, tablaDeMemoriasConCriterios, criterio);
    }
    if (dictionary_has_key(tablaDeMemoriasConCriterios, "SHC")) {
        char *criterio = "SHC";
        //borrarFdDeListaDeFdsConectados(fdDesconectado, tablaDeMemoriasConCriterios, criterio);
    }
    if (dictionary_has_key(tablaDeMemoriasConCriterios, "EC")) {
        char *criterio = "EC";
        //borrarFdDeListaDeFdsConectados(fdDesconectado, tablaDeMemoriasConCriterios, criterio);
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

//GOSSIPING

void agregarMemoriasRecibidas(char* memoriasRecibidas, t_list* memoriasConocidas, t_log* logger){

    int i = 0;
    char** listaDeMemorias = string_split(memoriasRecibidas, ";");
    while(listaDeMemorias[i] != NULL){

        char** nuevaMemoria = string_split(listaDeMemorias[i], ":");

        agregarIpMemoria(nuevaMemoria[0], atoi(nuevaMemoria[1]), memoriasConocidas, logger);

        i++;

    }
}

void agregarIpMemoria(char* ipNuevaMemoria, int puertoNuevaMemoria, t_list* memoriasConocidas, t_log* logger){

    t_nodoMemoria* unNodoMemoria = malloc(sizeof(t_nodoMemoria));


    bool sonMismaMemoria(void* elemento){

        if (elemento != NULL){

            unNodoMemoria = (t_nodoMemoria*)elemento;

            return strcmp( (char*) unNodoMemoria->ipNodoMemoria, ipNuevaMemoria) == 0 && (puertoNuevaMemoria == (int) unNodoMemoria->puertoNodoMemoria);

        }else{
            return false;
        }
    }

    if (!list_any_satisfy(memoriasConocidas, sonMismaMemoria)){



        t_nodoMemoria* nuevoNodoMemoria = (t_nodoMemoria*)malloc(sizeof(t_nodoMemoria));
        nuevoNodoMemoria->ipNodoMemoria = ipNuevaMemoria;
        nuevoNodoMemoria->puertoNodoMemoria = puertoNuevaMemoria;
        nuevoNodoMemoria->fdNodoMemoria = 0;
        list_add(memoriasConocidas, nuevoNodoMemoria);
        log_info(logger, "Nueva memoria agregada a lista de memorias conocidas");
    }else{
        log_info(logger, "Memoria ya conocida");
    }
}


