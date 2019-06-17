//
// Created by utnso on 12/05/19.
//

#include "conexiones.h"

pthread_t *crearHiloConexiones(GestorConexiones *unaConexion, t_log *logger) {
    pthread_t *hiloConexiones = malloc(sizeof(pthread_t));

    parametros_thread *parametros = (parametros_thread *) malloc(sizeof(parametros_thread));

    parametros->conexion = unaConexion;
    parametros->logger = logger;

    pthread_create(hiloConexiones, NULL, &atenderConexiones, parametros);

    return hiloConexiones;
}

void *atenderConexiones(void *parametrosThread) {
    parametros_thread *parametros = (parametros_thread *) parametrosThread;
    GestorConexiones *unaConexion = parametros->conexion;
    t_log *logger = parametros->logger;

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
                                desconectarCliente(fdConectado, unaConexion, logger);
                            } else {
                                switch (header.tipoMensaje) {
                                    case RESPUESTA:
                                        //gestionarRespuesta();//atenderMensajes(header, mensaje, parametros);
                                        break;
                                    case CONEXION_ACEPTADA:
                                        //confirmarHandshake(header, parametros);
                                        break;
                                        //Componente componente = *((Componente *) mensaje);
                                        //atenderHandshake(header, componente, parametros);
                                    case CONEXION_RECHAZADA:
                                        break;
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

int conectarseAMemoriaPrincipal(char* ipMemoria, int puertoMemoria, GestorConexiones* misConexiones, t_log* logger) {
    int fdMemoria = conectarseAServidor(ipMemoria, puertoMemoria, misConexiones, logger);
    if (fdMemoria < 0) {
        log_error(logger, "Hubo un error al intentar conectarse a la Memoria Principal. Cerrando el proceso...");
        exit(-1);
    } else {
        log_info(logger, "Conexión con Memoria Principal establecida.");
        hacerHandshake(fdMemoria, KERNEL);
    }
    return fdMemoria;
}

/*void confirmarHandshake(Header header, parametros_thread parametros) {
    t_log *logger = parametros.logger;
    list_add(parametros.conexion, header.fdRemitente);
    log_info(logger, "La memoria conectada recientemente ya se encuentra disponible para ser utilizada.\n")
}*/

/*
void conectarseAMemoriaPrincipal(t_memoria_conocida *memoriaConocida, char *ipMemoria, int puertoMemoria, t_log *logger) {
    log_info(logger, "Intentando conectarse a Memoria Principal.");
    memoriaConocida->fdMemoria = crearSocketCliente(ipMemoria, puertoMemoria, logger);
    if (memoriaConocida->fdMemoria < 0) {
        log_error(logger, "Hubo un error al intentar conectarse con la Memoria Principal. Se va a cerrar el proceso.");
        exit(-1);
    } else {
        log_info(logger, "Conexión con Memoria Principal establecida");
        enviarPaquete(memoriaConocida.fdMemoria, HANDSHAKE, NULL);
        memoriaConocida->utilizacion = 1;
    }
}

void atenderHandshake(Header header, Componente componente, parametros_thread_memoria *parametros) {
    if (componente == MEMORIA) {
        switch (header.tipoMensaje) {
            case CONEXION_ACEPTADA:
                if (!memoriaEsConocida) {
                    agregarMemoriaATabla();
                } else {
                    log_warning(parametros->logger,
                                "Se ha tratado de conectar un proceso a traves del FD: %i y no tiene permiso de hacerlo.",
                                header.fdRemitente)
                }
                break;
            case CONEXION_RECHAZADA:
                log_error(parametros->logger, "No se ha podido hacer handshake con la memoria en FD: %i",
                          header.fdRemitente);
                break;
        }
    }
}

void atenderMensajes(Header header, char *mensaje) {
    if (header.tipoMensaje == RESPUESTA) {
        //hacerHandshake(header.fdRemitente, KERNEL);
    };
    printf("Estoy recibiendo un mensaje del file descriptor %i: %s", header.fdRemitente, mensaje);
    fflush(stdout);
}

*/