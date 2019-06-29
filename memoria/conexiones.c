//
// Created by utnso on 04/05/19.
//

#include "conexiones.h"

pthread_t* crearHiloConexiones(GestorConexiones* unaConexion, t_memoria* memoria, t_control_conexion* conexionKernel, t_control_conexion* conexionLissandra, t_log* logger, pthread_mutex_t semaforoMemoriasConocidas)    {

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

    pthread_create(hiloConexiones, NULL, &atenderConexiones, parametros);

    return hiloConexiones;
}

void* atenderConexiones(void* parametrosThread)    {
    parametros_thread_memoria* parametros = (parametros_thread_memoria*) parametrosThread;
    GestorConexiones* unaConexion = parametros->conexion;
    t_log* logger = parametros->logger;

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
                            desconectarCliente(fdConectado, unaConexion, logger);
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
                                desconectarCliente(fdConectado, unaConexion, logger);
                            }
                            else	{
                                switch(header.tipoMensaje)  {
                                    case HANDSHAKE: ;
                                        Componente componente = *((Componente*) mensaje);
                                        atenderHandshake(header, componente, parametros);
                                        break;
                                    case REQUEST: ;
                                        atenderMensajes(header, mensaje, parametros);
                                        break;
                                    case GOSSIPING: ;
                                        atenderPedidoMemoria(header, mensaje, parametros);
                                        break;
                                    case RESPUESTA_GOSSIPING: ;
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
            enviarPaquete(header.fdRemitente, CONEXION_ACEPTADA, INVALIDO, NULL);
            sem_post(parametros->conexionKernel->semaforo);
        } else {
            enviarPaquete(header.fdRemitente, CONEXION_RECHAZADA, INVALIDO, NULL);
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

void agregarMemoriasRecibidas(char* memoriasRecibidas, t_list* memoriasConocidas, t_log* logger){
    char** memorias = string_split(memoriasRecibidas, ";");
    int i = 0;
    while (memorias[i] != NULL){
        char** unaIpSpliteada = string_split(memorias[i], ":");

        agregarIpMemoria(unaIpSpliteada[0], unaIpSpliteada[1], memoriasConocidas, logger);
        i++;
    }


}

void enviarRespuestaGossiping(t_list* memoriasConocidas, int fdRemitente){
    char* memoriasConocidasConcatenadas = concatenarMemoriasConocidas(memoriasConocidas);

    if (memoriasConocidasConcatenadas != NULL && strlen(memoriasConocidasConcatenadas)> 0){
        printf("Todas las memorias conocidas concatenadas: %s\n", memoriasConocidasConcatenadas);
        enviarPaquete(fdRemitente, RESPUESTA_GOSSIPING, RESPUESTA, memoriasConocidasConcatenadas);
    } else{
        printf("Mi lista estaba vacia\n");
        enviarPaquete(fdRemitente, RESPUESTA_GOSSIPING, RESPUESTA, "LISTA_VACIA");
    }
}
void atenderPedidoMemoria(Header header,char* mensaje, parametros_thread_memoria* parametros){
    t_memoria* memoria = (t_memoria*)parametros->memoria;
    pthread_mutex_t semaforoMemoriasConocidas = (pthread_mutex_t)parametros->semaforoMemoriasConocidas;
    t_list* memoriasConocidas = (t_list*)memoria->memoriasConocidas;
    t_log* logger = parametros->logger;

    pthread_mutex_lock(&semaforoMemoriasConocidas);
    if (header.tipoMensaje == GOSSIPING){
        if (strcmp(mensaje, "DAME_LISTA_GOSSIPING") == 0){
            printf("Recibi un pedido de mi lista de gossiping, yo tambien la pido\n");
            enviarRespuestaGossiping(memoriasConocidas, header.fdRemitente);
        }
    }else{
        //Es la respuesta al pedido
        if (header.tipoMensaje == RESPUESTA_GOSSIPING){
            if (strcmp(mensaje, "LISTA_VACIA") != 0){
                printf("Recibi %s como respuesta al gossiping\n", mensaje);
                agregarMemoriasRecibidas(mensaje, memoriasConocidas, logger);
            } else{
                printf("Recibi lista vacia\n");
            }
        }


    }
    pthread_mutex_unlock(&semaforoMemoriasConocidas);

    //free(memoriasConocidas);
    //free(memoria);

}

void* atenderRequestKernel(void* parametrosRequest)    {
    parametros_thread_request* parametros = (parametros_thread_request*) parametrosRequest;

    t_paquete resultado = gestionarRequest(parametros->comando, parametros->memoria, parametros->conexionLissandra, parametros->logger);

    if(parametros->conexionKernel->fd > 0)  {
        enviarPaquete(parametros->conexionKernel->fd, resultado.tipoMensaje, parametros->comando.tipoRequest, resultado.mensaje);
    }
}

pthread_t* crearHiloRequest(t_comando comando, t_memoria* memoria, t_control_conexion* conexionKernel, t_control_conexion* conexionLissandra, t_log* logger)   {
    pthread_t* hiloRequest = (pthread_t*) malloc(sizeof(pthread_t));

    parametros_thread_request* parametros = (parametros_thread_request*) malloc(sizeof(parametros_thread_request));

    parametros->comando = comando;
    parametros->logger = logger;
    parametros->conexionKernel = conexionKernel;
    parametros->memoria = memoria;
    parametros->conexionLissandra = conexionLissandra;

    pthread_create(hiloRequest, NULL, &atenderRequestKernel, parametros);

    return hiloRequest;
}

void atenderMensajes(Header header, void* mensaje, parametros_thread_memoria* parametros)    {
    char** comandoParseado = parser(mensaje);
    t_comando comando = instanciarComando(comandoParseado);
    crearHiloRequest(comando, parametros->memoria, parametros->conexionKernel, parametros->conexionLissandra, parametros->logger);

//    char** arrayMensaje = parser(mensaje);
//
//    //char** arrayMensaje = parser("SELECT amigues");
//
//
//    if (strcmp(arrayMensaje[0], "SELECT") == 0){
//        printf("Recibi un select");
//
//        //Todo chequear que las queries traigan la cantidad correcta de parámetros
//        //TODO crear un segmento y una página
//        //TODO buscar dentro del segmento lo que se pidió ELSE
//
//        enviarPaquete(FD_FS, mensaje);
//        Header *header = (Header*)malloc(sizeof(Header));
//        recv(FD_CLIENTE, header, sizeof(Header), NULL);
//        char* respuesta = malloc(header->tamanioMensaje);
//        recv(FD_CLIENTE, respuesta, header->tamanioMensaje, NULL);
//        printf("%s", mensaje);
//
//    }else if (strcmp(arrayMensaje[0], "INSERT") == 0){
//        printf("Recibi un insert");
//    }else if (strcmp(arrayMensaje[0], "CREATE") == 0){
//        printf("Recibi un create");
//    }else if (strcmp(arrayMensaje[0], "DESCRIBE") == 0){
//        printf("Recibi un describe");
//    }else if (strcmp(arrayMensaje[0], "DROP") == 0){
//        printf("Recibi un drop");
//    }else if (strcmp(arrayMensaje[0], "JOURNAL") == 0){
//        printf("Recibi un journal");
//    }else{
//        printf("No entendi tu mensaje bro");
//    }
//    printf("\n");
//    fflush(stdout);
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

bool conectarseANodoMemoria(char* unaIp, char* unPuerto, t_log* logger){
    log_info(logger, "Intentando conectarse a una memoria seed...");
    int fdNodoMemoria = crearSocketCliente(unaIp, atoi(unPuerto), logger);
    if(fdNodoMemoria< 0) {
        char* mensajeError = string_from_format("Hubo un error al intentar conectarse a la memoria con ip %s y puerto %s . Cerrando el proceso...", unaIp, unPuerto);
        log_error(logger, mensajeError);
        return NULL;
    }
    else{
        char* mensajeInfo = string_from_format("Conexion establecida con memoria con ip %s y puerto %s", unaIp, unPuerto);
        log_info(logger, mensajeInfo);
        return fdNodoMemoria;
    }
}