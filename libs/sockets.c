/*
 ============================================================================
 Name        : Sockets.c
 Author      : Suck et
 	 UTN FRBA - Sistemas Operativos
 	 	 TP-1C-2019-Lissandra
 ============================================================================
 */

#include "sockets.h"

//Creamos un socket!

int crearSocket(t_log* logger) {
    int fileDescriptor = socket(AF_INET, SOCK_STREAM, 0);//usa protocolo TCP/IP
    if (fileDescriptor == ERROR) {
        log_error(logger, "No se pudo crear el file descriptor.");
    }

    return fileDescriptor;
}

//Creamos ahora un Socket Servidor

int crearSocketServidor(int puerto, t_log* logger)	{
    struct sockaddr_in miDireccionServidor;
    int socketDeEscucha = crearSocket(logger);

    miDireccionServidor.sin_family = AF_INET;			//Protocolo de conexion
    miDireccionServidor.sin_addr.s_addr = INADDR_ANY;	//INADDR_ANY = 0 y significa que usa la IP actual de la maquina
    miDireccionServidor.sin_port = htons(puerto);		//Puerto en el que escucha
    memset(&(miDireccionServidor.sin_zero), '\0', 8);	//Pone 0 al campo de la estructura "miDireccionServidor"

    //Veamos si el puerto está en uso
    int puertoEnUso = 1;
    int puertoYaAsociado = setsockopt(socketDeEscucha, SOL_SOCKET, SO_REUSEADDR, (char*) &puertoEnUso, sizeof(puertoEnUso));

    if (puertoYaAsociado == ERROR) {
        log_error(logger, "El puerto asignado ya está siendo utilizado.");
    }
    //Turno del bind
    int activado = 1;
    //Para evitar que falle el bind, al querer usar un mismo puerto
    setsockopt(socketDeEscucha,SOL_SOCKET,SO_REUSEADDR,&activado,sizeof(activado));

    int valorBind = bind(socketDeEscucha,(void*) &miDireccionServidor, sizeof(miDireccionServidor));

    if ( valorBind !=0) {
        log_error(logger, "El bind no funcionó, el socket no se pudo asociar al puerto");
        return 1;
    }

    log_info(logger, "Servidor levantado en el puerto %i", puerto);

    return socketDeEscucha;
}

void escucharSocketsEn(int fd_socket, t_log* logger){

    int valorListen;
    valorListen = listen(fd_socket, conexionesMaximasPermitidas);/*Le podríamos poner al listen
				SOMAXCONN como segundo parámetro, y significaría el máximo tamaño de la cola*/
    if(valorListen == ERROR) {
        log_error(logger, "El servidor no pudo recibir escuchar conexiones de clientes.");
    } else	{
        log_info(logger, "El servidor está escuchando conexiones a través del socket %i.", fd_socket);
    }
}

int crearSocketEscucha (int puerto, t_log* logger) {

    int socketDeEscucha = crearSocketServidor(puerto, logger);

    //Escuchar conexiones
    escucharSocketsEn(socketDeEscucha, logger);

    return socketDeEscucha;
}

int aceptarCliente(int fd_socket, t_log* logger){

    struct sockaddr_in unCliente;
    unsigned int addres_size = sizeof(unCliente);

    int fd_Cliente = accept(fd_socket, (struct sockaddr*) &unCliente, &addres_size);
    if(fd_Cliente == ERROR)  {
        log_error(logger, "El servidor no pudo aceptar la conexión entrante.");
    } else	{
        log_info(logger, "Se abrió una conexión con el file descriptor %i.", fd_Cliente);
    }

    return fd_Cliente;

}

//Creamos un Socket Cliente -ya que tenemos antes el Servidor-
int crearSocketCliente(char *ipServidor, int puerto, t_log* logger) {

    int cliente;
    struct sockaddr_in direccionServidor;

    direccionServidor.sin_family = AF_INET;				// Ordenación de bytes de la máquina
    direccionServidor.sin_addr.s_addr = inet_addr(ipServidor);
    direccionServidor.sin_port = htons(puerto);			// short, Ordenación de bytes de la red
    memset(&(direccionServidor.sin_zero), '\0', 8); 	// Pone cero al resto de la estructura

    cliente = crearSocket(logger); //Creamos socket
    int valorConnect = connect(cliente, (struct sockaddr *) &direccionServidor, sizeof(direccionServidor));

    if(valorConnect == ERROR)  {
        log_error(logger, "No se pudo establecer conexión entre el socket y el servidor.");
        return ERROR;
    }
    else {
//        log_info(logger, "Se estableció correctamente la conexión con el servidor a través del socket %i.", cliente);
        return cliente;
    }
}

void cerrarSocket(int fd_socket, t_log* logger) {

    int cerrar = close(fd_socket);
    if (cerrar == ERROR)
        log_error(logger, "No se pudo cerrar el file descriptor del socket: %d", fd_socket);
}

void cargarListaClientes(GestorConexiones* unaConexion, fd_set* emisores)	{
    FD_ZERO(emisores);
    FD_SET(unaConexion->servidor, emisores);

    for(int i=0; i < list_size(unaConexion->conexiones); i++)   {
        int fdConexion = *((int*) list_get(unaConexion->conexiones, i));
        FD_SET(fdConexion, emisores);
    }
}

bool hayNuevoMensaje(GestorConexiones* unaConexion, fd_set* emisores)    {
    // inicializo las colas de clientes
    cargarListaClientes(unaConexion, emisores);
    return select(unaConexion->descriptorMaximo + 1, emisores, NULL, NULL, NULL) > 0;
}

int conectarseAServidor(char* ip, int puerto, GestorConexiones* conexion, t_log* logger)  {
    int* fdNuevoServidor = (int*) malloc(sizeof(int));
    *fdNuevoServidor = crearSocketCliente(ip, puerto, logger);
    pthread_mutex_lock(&conexion->mutexConexiones);
    if(*fdNuevoServidor > 0)
        list_add(conexion->conexiones, fdNuevoServidor);
    conexion->descriptorMaximo = getFdMaximo(conexion);
    pthread_mutex_unlock(&conexion->mutexConexiones);

    return *fdNuevoServidor;
}

void levantarServidor(int puerto, GestorConexiones* conexion, t_log* logger) {
    int fdServidor = crearSocketEscucha(puerto, logger);
    conexion->servidor = fdServidor;
    conexion->descriptorMaximo = getFdMaximo(conexion);
}

int getFdMaximo(GestorConexiones* conexion) {
    int maximo = conexion->servidor;
    int tamanioLista = list_size(conexion->conexiones);

    for(int i=0; i < tamanioLista; i++) {
        int* fdConexion = (int*) list_get(conexion->conexiones, i);
        if(*fdConexion > maximo)
            maximo = *fdConexion;
    }

    return maximo;
}

void desconectarCliente(int fdCliente, GestorConexiones* unaConexion, t_log* logger)	{
    cerrarSocket(fdCliente, logger);
    eliminarFdDeListaDeConexiones(fdCliente, unaConexion);
    log_info(logger, "El socket %i ha cerrado la conexión (posiblemente haya sido terminado).", fdCliente);
}

void eliminarFdDeListaDeConexiones(int fdCliente, GestorConexiones* unaConexion)  {
    bool esElClienteDesconectado(void* cliente)	{
        return *((int*) cliente) == fdCliente;
    }
    pthread_mutex_lock(&unaConexion->mutexConexiones);
    list_remove_and_destroy_by_condition(unaConexion->conexiones, esElClienteDesconectado, free);
    unaConexion->descriptorMaximo = getFdMaximo(unaConexion);
    pthread_mutex_unlock(&unaConexion->mutexConexiones);
}

void enviarPaquete(int fdDestinatario, TipoMensaje tipoMensaje, TipoRequest tipoRequest, char* mensaje, int pid)  {
    int pesoMensaje = pesoString(mensaje);
    Header header = armarHeader(fdDestinatario, pesoMensaje, tipoMensaje, tipoRequest, pid);
    void* headerSerializado = serializarHeader(header);
    int pesoPaquete = sizeof(Header) + pesoMensaje;
    void* paquete = empaquetar(headerSerializado, mensaje);
    send(fdDestinatario, paquete, pesoPaquete, MSG_WAITALL);
    free(headerSerializado);
    free(paquete);
}

void hacerHandshake(int fdDestinatario, Componente componente)  {
    char* componenteStr = string_itoa(componente);
    enviarPaquete(fdDestinatario, HANDSHAKE, INVALIDO, componenteStr, -1);
    free(componenteStr);
}

void* empaquetar(void* headerSerializado, char* mensaje)   {
    int pesoMensaje = pesoString(mensaje);
    void* paquete = malloc(sizeof(Header) + pesoMensaje);
    void* puntero = paquete;
    memcpy(puntero, headerSerializado, sizeof(Header));
    puntero += sizeof(Header);
    memcpy(puntero, mensaje, pesoMensaje);
    return paquete;
}

void* serializarHeader(Header header)    {
    void* headerSerializado = malloc(sizeof(Header));
    void* puntero = headerSerializado;
    int tamanioSize = sizeof(typeof(header.tamanioMensaje));
    int pesoTipoRemitente = sizeof(typeof(header.fdRemitente));
    int pesoTipoMensaje = sizeof(typeof(header.tipoMensaje));
    int pesoTipoRequest = sizeof(typeof(header.tipoRequest));

    memcpy(puntero, &(header.tamanioMensaje), tamanioSize);
    puntero += tamanioSize;
    memcpy(puntero, &(header.fdRemitente), pesoTipoRemitente);
    puntero += pesoTipoRemitente;
    memcpy(puntero, &(header.tipoMensaje), pesoTipoMensaje);
    puntero += pesoTipoMensaje;
    memcpy(puntero, &(header.tipoRequest), pesoTipoRequest);
    puntero += pesoTipoRequest;
    memcpy(puntero, &(header.pid), sizeof(int));

    return headerSerializado;
}

Header deserializarHeader(void* headerSerializado)	{
    Header header;
    void* puntero = headerSerializado;
    int tamanioSize = sizeof(typeof(header.tamanioMensaje));
    int pesoTipoRemitente = sizeof(typeof(header.fdRemitente));
    int pesoTipoMensaje = sizeof(typeof(header.tipoMensaje));
    int pesoTipoRequest = sizeof(typeof(header.tipoRequest));

    memcpy(&(header.tamanioMensaje), puntero, tamanioSize);
    puntero += tamanioSize;

    memcpy(&(header.fdRemitente), puntero, pesoTipoRemitente);
    puntero += pesoTipoRemitente;

    memcpy(&(header.tipoMensaje), puntero, pesoTipoMensaje);
    puntero += pesoTipoMensaje;

    memcpy(&(header.tipoRequest), puntero, pesoTipoRequest);
    puntero += pesoTipoRequest;

    memcpy(&(header.pid), puntero, sizeof(int));

    return header;
}

Header armarHeader(int fdDestinatario, int tamanioDelMensaje, TipoMensaje tipoMensaje, TipoRequest tipoRequest, int pid)    {
    Header header = {.tamanioMensaje = tamanioDelMensaje, .fdRemitente = fdDestinatario, .tipoMensaje = tipoMensaje, .tipoRequest = tipoRequest, .pid = pid};
    return header;
}

GestorConexiones* inicializarConexion() {
    GestorConexiones* nuevaConexion = (GestorConexiones*) malloc(sizeof(GestorConexiones));

    nuevaConexion->descriptorMaximo = -1;
    nuevaConexion->servidor = -1;
    nuevaConexion->conexiones = list_create();
    pthread_mutex_init(&nuevaConexion->mutexConexiones, NULL);

    return nuevaConexion;
}