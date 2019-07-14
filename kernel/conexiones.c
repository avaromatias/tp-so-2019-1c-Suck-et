//
// Created by utnso on 12/05/19.
//

#include "conexiones.h"

pthread_t *crearHiloConexiones(GestorConexiones *unaConexion, t_log *logger, t_dictionary *tablaDeMemoriasConCriterios,
                               t_dictionary* metadataTabla, pthread_mutex_t *mutexJournal) {
    pthread_t *hiloConexiones = malloc(sizeof(pthread_t));

    parametros_thread_k *parametros = (parametros_thread_k *) malloc(sizeof(parametros_thread_k));

    parametros->tablaDeMemoriasConCriterios = tablaDeMemoriasConCriterios;
    parametros->metadataTabla = metadataTabla;
    parametros->conexion = unaConexion;
    parametros->logger = logger;
    parametros->mutexJournal = mutexJournal;

    pthread_create(hiloConexiones, NULL, &atenderConexiones, parametros);

    return hiloConexiones;
}

void *atenderConexiones(void *parametrosThread) {
    parametros_thread_k *parametros = (parametros_thread_k *) parametrosThread;
    GestorConexiones *unaConexion = parametros->conexion;
    t_log *logger = parametros->logger;
    t_dictionary *tablaDeMemoriasConCriterios = parametros->tablaDeMemoriasConCriterios;
    t_dictionary *metadata = parametros->metadataTabla;
    pthread_mutex_t *mutexJournal = parametros->mutexJournal;

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
                            desconexionMemoria(fdConectado, unaConexion, tablaDeMemoriasConCriterios, logger,
                                               mutexJournal);
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
                                desconexionMemoria(fdConectado, unaConexion, tablaDeMemoriasConCriterios, logger,
                                                   mutexJournal);
                            } else {
                                switch (header.tipoMensaje) {
                                    case RESPUESTA:
                                        gestionarRespuesta(header.fdRemitente, tablaDeMemoriasConCriterios, metadata,
                                                header.tipoRequest, mensaje, logger);
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
                                    case GOSSIPING:
                                        //char **direccionesNuevasMemorias = obtenerDatosDeConexion();
                                    case ERR:
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

void gestionarRespuesta(int fdMemoria, t_dictionary *memoriasConCriterios, t_dictionary *metadata, TipoRequest tipoRequest, char *mensaje, t_log *logger){
    switch (tipoRequest) {
        case SELECT:
            //incrementoCantidadSelects procesados para metricas;
            log_info(logger, "El SELECT enviado a la memoria %i fue procesado correctamente. Respuesta recibida: %s", fdMemoria, mensaje);
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
}

void enviarJournal(int fdMemoria) {
    enviarPaquete(fdMemoria, REQUEST, JOURNAL, "JOURNAL");
}

void borrarFdDeListaDeFdsConectados(int fdADesconectar, t_dictionary *tablaDeMemoriasConCriterios, char *criterio) {
    t_list *listaDeMemoriasConectadasACriterio = dictionary_get(tablaDeMemoriasConCriterios, criterio);
    dictionary_remove(tablaDeMemoriasConCriterios, criterio);
    list_remove(listaDeMemoriasConectadasACriterio, fdADesconectar);
    dictionary_put(tablaDeMemoriasConCriterios, criterio, listaDeMemoriasConectadasACriterio);
}

void eliminarFileDescriptorDeTablasDeMemorias(int fdDesconectado, t_dictionary *tablaDeMemoriasConCriterios,
                                              pthread_mutex_t *mutexJournal) {

    pthread_mutex_lock(mutexJournal);

    if (dictionary_has_key(tablaDeMemoriasConCriterios, "SC")) {
        char *criterio = "SC";
        borrarFdDeListaDeFdsConectados(fdDesconectado, tablaDeMemoriasConCriterios, criterio);
    }
    if (dictionary_has_key(tablaDeMemoriasConCriterios, "SHC")) {
        char *criterio = "SHC";
        borrarFdDeListaDeFdsConectados(fdDesconectado, tablaDeMemoriasConCriterios, criterio);
    }
    if (dictionary_has_key(tablaDeMemoriasConCriterios, "EC")) {
        char *criterio = "EC";
        borrarFdDeListaDeFdsConectados(fdDesconectado, tablaDeMemoriasConCriterios, criterio);
    }
}

void desconexionMemoria(int fdConectado, GestorConexiones *unaConexion, t_dictionary *tablaDeMemoriasConCriterios,
                        t_log *logger, pthread_mutex_t *mutexJournal) {
    eliminarFileDescriptorDeTablasDeMemorias(fdConectado, tablaDeMemoriasConCriterios, mutexJournal);
    desconectarCliente(fdConectado, unaConexion, logger);
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

void crearTablaEnMetadata(t_dictionary *metadataTablas, char* mensaje, t_log *logger) {
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