//
// Created by utnso on 04/05/19.
//

#include "conexiones.h"

pthread_t* crearHiloConexiones(GestorConexiones* unaConexion, t_memoria* memoria, t_control_conexion* conexionKernel, t_control_conexion* conexionLissandra, t_log* logger, pthread_mutex_t* semaforoMemoriasConocidas, pthread_mutex_t* semaforoJournaling)    {

    /*int value;
    sem_getvalue(kernelConectado, &value);
    printf("El valor del semaforo es %d\n", value);*/

    pthread_t* hiloConexiones = malloc(sizeof(pthread_t));

    parametros_thread_memoria* parametros = (parametros_thread_memoria*) malloc(sizeof(parametros_thread_memoria));

    parametros->conexion = unaConexion;
    parametros->logger = logger;
    parametros->conexionKernel = conexionKernel;
    parametros->conexionLissandra = conexionLissandra;
    parametros->memoria = memoria;
    parametros->semaforoMemoriasConocidas = semaforoMemoriasConocidas;
    parametros->semaforoJournaling = semaforoJournaling;

    pthread_create(hiloConexiones, NULL, &atenderConexiones, parametros);

    return hiloConexiones;
}

void* atenderConexiones(void* parametrosThread)    {
    parametros_thread_memoria* parametros = (parametros_thread_memoria*) parametrosThread;
    GestorConexiones* unaConexion = parametros->conexion;
    t_log* logger = parametros->logger;
    t_memoria* memoria = parametros->memoria;

    fd_set emisores;

    while(1)    {
        if(hayNuevoMensaje(unaConexion, &emisores))    {
            // voy a recorrer todos los FD a los cuales estoy conectado para saber cuál de todos es el que tiene un nuevo mensaje
            for(int i=0; i < list_size(unaConexion->conexiones); i++)   {
                Header headerSerializado;
                int fdConectado = *((int*) list_get(unaConexion->conexiones, i));

                if(FD_ISSET(fdConectado, &emisores))    {
                    int bytesRecibidos = recv(fdConectado, &headerSerializado, sizeof(Header), MSG_DONTWAIT);

                    switch(bytesRecibidos)  {
                        // hubo un error al recibir los datos
                        case -1:
                            log_warning(logger, "Hubo un error al recibir el header proveniente del socket %i", fdConectado);
                            break;
                            // se desconectó
                        case 0:
                            // acá cada uno setea una maravillosa función que hace cada uno cuando se le desconecta alguien
                            // nombre_maravillosa_funcion();

                            if (esNodoMemoria(fdConectado, memoria->nodosMemoria)){

                                nodoMemoria* nodo;
                                bool mismoNodo(void* elemento){
                                    nodo = (nodoMemoria*) elemento;
                                    return fdConectado == nodo->fdNodoMemoria;
                                }
                                nodoMemoria* unNodoMemoria = (nodoMemoria*) list_find(memoria->nodosMemoria, mismoNodo);
                                eliminarMemoriaConocida(memoria, unNodoMemoria);
                                eliminarNodoMemoria(fdConectado, memoria->nodosMemoria);
                            }
                            else if(fdConectado == parametros->conexionKernel->fd)   {
                                parametros->conexionKernel->fd = 0;
                                sem_post(parametros->conexionKernel->semaforo);
                            }
                            desconectarCliente(fdConectado, unaConexion, logger);


                            //eliminarNodoMemoria(fdConectado, parametros->memoria);
                            break;
                            // recibí algún mensaje
                        default: ; // esto es lo más raro que vi pero tuve que hacerlo
                            Header header = deserializarHeader(&headerSerializado);
                            header.fdRemitente = fdConectado;
                            int pesoMensaje = header.tamanioMensaje * sizeof(char);
                            void* mensaje = (void*) malloc(pesoMensaje);
                            bytesRecibidos = recv(fdConectado, mensaje, pesoMensaje, MSG_DONTWAIT);
                            if(bytesRecibidos == -1 || bytesRecibidos < pesoMensaje)
                                log_warning(logger, "Hubo un error al recibir el mensaje proveniente del socket %i", fdConectado);
                            else if(bytesRecibidos == 0)	{
                                // acá cada uno setea una maravillosa función que hace cada uno cuando se le desconecta alguien
                                // nombre_maravillosa_funcion();
                                if (esNodoMemoria(fdConectado, memoria->nodosMemoria)){

                                    nodoMemoria* nodo;
                                    bool mismoNodo(void* elemento){
                                        nodo = (nodoMemoria*) elemento;
                                        return fdConectado == nodo->fdNodoMemoria;
                                    }
                                    nodoMemoria* unNodoMemoria = (nodoMemoria*) list_find(memoria->nodosMemoria, mismoNodo);
                                    eliminarMemoriaConocida(memoria, unNodoMemoria);
                                    eliminarNodoMemoria(fdConectado, memoria->nodosMemoria);
                                }
                                if(fdConectado == parametros->conexionKernel->fd)   {
                                    parametros->conexionKernel->fd = 0;
                                    sem_post(parametros->conexionKernel->semaforo);
                                }
                                desconectarCliente(fdConectado, unaConexion, logger);
                            }
                            else	{
                                switch(header.tipoMensaje)  {
                                    case HANDSHAKE: ;
                                        char* componenteStr = (char*) mensaje;
                                        Componente componente = (Componente) atoi(componenteStr);
                                        atenderHandshake(header, componente, parametros);
                                        break;
                                    case REQUEST: ;
                                        atenderMensajes(header, mensaje, parametros);
                                        break;
                                    case GOSSIPING: ;
                                        atenderPedidoMemoria(header, mensaje, parametros);
                                        break;
                                    case RESPUESTA_GOSSIPING_2: ;
                                        atenderPedidoMemoria(header, mensaje, parametros);
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
            if(FD_ISSET(unaConexion->servidor, &emisores))	{
                int* fdNuevoCliente = malloc(sizeof(int));
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

void atenderHandshake(Header header, Componente componente, parametros_thread_memoria* parametros) {
    if(componente == KERNEL) {
        if (parametros->conexionKernel->fd == 0) {
            parametros->conexionKernel->fd = header.fdRemitente;
            enviarPaquete(header.fdRemitente, CONEXION_ACEPTADA, INVALIDO, "Conexión aceptada.");
            sem_post(parametros->conexionKernel->semaforo);
        } else {
            enviarPaquete(header.fdRemitente, CONEXION_RECHAZADA, INVALIDO, "Conexión rechazada.");
            cerrarSocket(header.fdRemitente, parametros->logger);
        }
    }
    else if(componente == MEMORIA)  {
        //enviarPaquete(header.fdRemitente, CONEXION_ACEPTADA, NULL);
    }
}

char* concatenarMemoriasConocidas(t_list* memoriasConocidas){
    char* respuesta = string_new();

    //t_nodoMemoria* nodoMemoriaAuxiliar = malloc(sizeof(t_nodoMemoria));

    void _concatenarString(void* elemento){
        if (elemento != NULL){
            string_append(&respuesta, (char*) elemento);
            string_append(&respuesta, ";");
        }
    }
    if (!list_is_empty(memoriasConocidas)){
        list_iterate(memoriasConocidas, _concatenarString);
    }


    return respuesta;
}

void agregarMemoriasRecibidas(char* memoriasRecibidas, GestorConexiones* misConexiones, t_memoria* memoria, t_log* logger, pthread_mutex_t* semaforoMemoriasConocidas){
    char** memoriasNuevas = string_split(memoriasRecibidas, ";");
    int i = 0;
    while (memoriasNuevas[i] != NULL){
        //char** unaIpSpliteada = string_split(memoriasNuevas[i], ":");
        pthread_mutex_lock(semaforoMemoriasConocidas);
        conectarYAgregarNuevaMemoria(memoriasNuevas[i], misConexiones, logger, memoria);
        pthread_mutex_unlock(semaforoMemoriasConocidas);
        //agregarIpMemoria(unaIpSpliteada[0], unaIpSpliteada[1], memoriasConocidas, logger);
        i++;
    }


}
void enviarPedidoGossiping(nodoMemoria* unNodoMemoria, t_memoria* memoria, pthread_mutex_t* semaforoMemoriasConocidas, t_log* logger, GestorConexiones* misConexiones){

    int fdDestinatario = unNodoMemoria->fdNodoMemoria;
    enviarPaquete(fdDestinatario, GOSSIPING, REQUEST, "DAME_LISTA_GOSSIPING");
    //Capturo la respuesta del gossiping y luego envio mi lista de gossiping
    t_control_conexion unaMemoria = {.semaforo = (sem_t*) malloc(sizeof(sem_t))};
    unaMemoria.fd = fdDestinatario;
    t_paquete respuesta = recibirMensajeGossipingMemoria(&unaMemoria);

    if(respuesta.mensaje == NULL || unaMemoria.fd == 0){
        log_info(logger, "Parece que una memoria se desconectó. Procedo a eliminarla.");
        desconectarCliente(fdDestinatario, misConexiones, logger);
        eliminarMemoriaConocida(memoria, unNodoMemoria);
        eliminarNodoMemoria(unNodoMemoria->fdNodoMemoria, memoria->nodosMemoria);


    }else if (respuesta.tipoMensaje== RESPUESTA_GOSSIPING){

        if (strcmp(respuesta.mensaje, "LISTA_VACIA") != 0){
            log_info(logger, string_from_format(("Del header %i recibi %s como respuesta al gossiping\n", unaMemoria.fd, respuesta.mensaje)));
            //printf("Del header %i recibi %s como respuesta al gossiping\n", unaMemoria.fd, respuesta.mensaje);
            agregarMemoriasRecibidas(respuesta.mensaje,misConexiones, memoria, logger, semaforoMemoriasConocidas);
        } else{
            log_info(logger, "Recibi lista vacia como respuesta al gossiping");
            //printf("Recibi lista vacia\n");
        }

        char* memoriasConocidasConcatenadas = concatenarMemoriasConocidas(memoria->memoriasConocidas);

        if (memoriasConocidasConcatenadas != NULL && strlen(memoriasConocidasConcatenadas)> 0){
            //printf("Envio las memorias conocidas concatenadas: %s para %i\n", memoriasConocidasConcatenadas, unaMemoria.fd);
            log_info(logger, string_from_format("Envio las memorias que conozco %s", memoriasConocidasConcatenadas));
            enviarPaquete(unaMemoria.fd, RESPUESTA_GOSSIPING_2, INVALIDO, memoriasConocidasConcatenadas);
        } else{
            log_info(logger, "Mi lista vacia");
            enviarPaquete(unaMemoria.fd, RESPUESTA_GOSSIPING_2, INVALIDO, "LISTA_VACIA");
        }

        }


}
void atenderPedidoMemoria(Header header,char* mensaje, parametros_thread_memoria* parametros){

    pthread_mutex_t* semaforoMemoriasConocidas = (pthread_mutex_t*)parametros->semaforoMemoriasConocidas;
    //pthread_mutex_lock(semaforoMemoriasConocidas);
    t_memoria* memoria = (t_memoria*)parametros->memoria;
    t_list* memoriasConocidas = (t_list*)memoria->memoriasConocidas;
    t_log* logger = parametros->logger;
    GestorConexiones* misConexiones = parametros->conexion;


        if (strcmp(mensaje, "DAME_LISTA_GOSSIPING") == 0){
            printf("Recibi un pedido de mi lista de gossiping de fd: %i, envio la respuesta\n", header.fdRemitente);

            char* memoriasConocidasConcatenadas = concatenarMemoriasConocidas(memoriasConocidas);

            if (memoriasConocidasConcatenadas != NULL && strlen(memoriasConocidasConcatenadas)> 0){
                printf("Envio las memorias conocidas concatenadas: %s para %i\n", memoriasConocidasConcatenadas, header.fdRemitente);
                enviarPaquete(header.fdRemitente, RESPUESTA_GOSSIPING, RESPUESTA_GOSSIPING, memoriasConocidasConcatenadas);
            } else{
                printf("Mi lista estaba vacia primera respuesta\n");
                enviarPaquete(header.fdRemitente, RESPUESTA_GOSSIPING, RESPUESTA_GOSSIPING, "LISTA_VACIA");
            }

        }else if (header.tipoMensaje == RESPUESTA_GOSSIPING_2){
            if (strcmp(mensaje, "LISTA_VACIA") != 0){
                printf("Del header %i recibi %s como respuesta al gossiping\n", header.fdRemitente, mensaje);
                agregarMemoriasRecibidas(mensaje, misConexiones, logger, memoria, semaforoMemoriasConocidas);
            } else{
                printf("Recibi lista vacia como respuesta 2\n");
            }
    }
    //pthread_mutex_unlock(semaforoMemoriasConocidas);
}

void* atenderRequestKernel(void* parametrosRequest)    {
    parametros_thread_request* parametros = (parametros_thread_request*) parametrosRequest;


    pthread_mutex_lock(parametros->semaforoJournaling);


    t_paquete resultado = gestionarRequest(parametros->comando, parametros->memoria, parametros->conexionLissandra, parametros->logger, parametros->semaforoJournaling);

    if(parametros->conexionKernel->fd > 0)  {
        enviarPaquete(parametros->conexionKernel->fd, resultado.tipoMensaje, parametros->comando.tipoRequest, resultado.mensaje);
    }
    pthread_mutex_unlock(parametros->semaforoJournaling);
}

pthread_t* crearHiloRequest(t_comando comando, t_memoria* memoria, t_control_conexion* conexionKernel, t_control_conexion* conexionLissandra, t_log* logger, pthread_mutex_t* semaforoJournaling)   {
    pthread_t* hiloRequest = (pthread_t*) malloc(sizeof(pthread_t));

    parametros_thread_request* parametros = (parametros_thread_request*) malloc(sizeof(parametros_thread_request));

    parametros->comando = comando;
    parametros->logger = logger;
    parametros->conexionKernel = conexionKernel;
    parametros->memoria = memoria;
    parametros->conexionLissandra = conexionLissandra;
    parametros->semaforoJournaling = semaforoJournaling;

    pthread_create(hiloRequest, NULL, &atenderRequestKernel, parametros);

    return hiloRequest;
}

void atenderMensajes(Header header, void* mensaje, parametros_thread_memoria* parametros)    {

    //todo wait semaforoJournaling
    char** comandoParseado = parser(mensaje);
    t_comando comando = instanciarComando(comandoParseado);
    crearHiloRequest(comando, parametros->memoria, parametros->conexionKernel, parametros->conexionLissandra, parametros->logger, parametros->semaforoJournaling);

    //todo post semaforoJournaling
}

t_paquete recibirMensajeDeLissandra(t_control_conexion *conexion)    {
    Header header;
    //sem_wait(conexion->semaforo);
    t_paquete paquete;
    int bytesRecibidos = recv(conexion->fd, &header, sizeof(Header), MSG_WAITALL);
    if(bytesRecibidos == 0) {
        exit(-1);
        conexion->fd = 0;
    }
    else    {
        header = deserializarHeader(&header);
        paquete.tipoMensaje = header.tipoMensaje;
        char* respuesta = (char*) malloc(header.tamanioMensaje);
        bytesRecibidos = recv(conexion->fd, respuesta, header.tamanioMensaje, MSG_WAITALL);
        if(bytesRecibidos == 0) {
            exit(-1);
            conexion->fd = 0;
            paquete.mensaje = NULL;
        }
        else    {
            //sem_post(conexion->semaforo);
            paquete.mensaje = respuesta;
        }
    }
    return paquete;
}

t_paquete recibirMensajeGossipingMemoria(t_control_conexion *conexion)    {
    Header header;
    //sem_wait(conexion->semaforo);
    t_paquete paquete;
    int bytesRecibidos = recv(conexion->fd, &header, sizeof(Header), MSG_WAITALL);
    if(bytesRecibidos == 0) {
        //exit(-1);
        conexion->fd = 0;
    }
    else    {
        header = deserializarHeader(&header);
        paquete.tipoMensaje = header.tipoMensaje;
        char* respuesta = (char*) malloc(header.tamanioMensaje);
        bytesRecibidos = recv(conexion->fd, respuesta, header.tamanioMensaje, MSG_WAITALL);
        if(bytesRecibidos == 0) {
            //exit(-1);
            conexion->fd = 0;
            paquete.mensaje = NULL;
        }
        else    {
            //sem_post(conexion->semaforo);
            paquete.mensaje = respuesta;
        }
    }
    return paquete;
}

void conectarseALissandra(t_control_conexion* conexionLissandra, char* ipLissandra, int puertoLissandra, t_log* logger){
    log_info(logger, "Intentando conectarse a Lissandra...");
    conexionLissandra->fd = crearSocketCliente(ipLissandra, puertoLissandra, logger);
    if(conexionLissandra->fd < 0) {
        log_error(logger, "Hubo un error al intentar conectarse a Lissandra. Cerrando el proceso...");
        exit(-1);
    }
    else{
        log_info(logger, "Conexión con Lissandra establecida.");
        sem_post(conexionLissandra->semaforo);
    }
}