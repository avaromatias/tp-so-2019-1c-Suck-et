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

int crearSocket() {
    int fileDescriptor = socket(AF_INET, SOCK_STREAM, 0);//usa protocolo TCP/IP
    if (fileDescriptor == ERROR) {
        perror("No se pudo crear el file descriptor.\n");
    }

    //Hay que mejorarlo para el caso en el que escucha muchas conexiones

    return fileDescriptor;
}

//Creamos ahora un Socket Servidor

int crearSocketServidor(int puerto)	{
    struct sockaddr_in miDireccionServidor;
    int socketDeEscucha = crearSocket();

    miDireccionServidor.sin_family = AF_INET;			//Protocolo de conexion
    miDireccionServidor.sin_addr.s_addr = INADDR_ANY;	//INADDR_ANY = 0 y significa que usa la IP actual de la maquina
    miDireccionServidor.sin_port = htons(puerto);		//Puerto en el que escucha
    memset(&(miDireccionServidor.sin_zero), '\0', 8);	//Pone 0 al campo de la estructura "miDireccionServidor"

    //Veamos si el puerto está en uso
    int puertoEnUso = 1;
    int puertoYaAsociado = setsockopt(socketDeEscucha, SOL_SOCKET, SO_REUSEADDR, (char*) &puertoEnUso, sizeof(puertoEnUso));

    if (puertoYaAsociado == ERROR) {
        perror("El puerto asignado ya está siendo utilizado.\n");
    }
    //Turno del bind
    int activado = 1;
    //Para evitar que falle el bind, al querer usar un mismo puerto
    setsockopt(socketDeEscucha,SOL_SOCKET,SO_REUSEADDR,&activado,sizeof(activado));

    int valorBind = bind(socketDeEscucha,(void*) &miDireccionServidor, sizeof(miDireccionServidor));

    if ( valorBind !=0) {
        perror("El bind no funcionó, el socket no se pudo asociar al puerto");
        return 1;
    }

    return socketDeEscucha;
}

void escucharSocketsEn(int fd_socket){

    int valorListen;
    valorListen = listen(fd_socket, conexionesMaximasPermitidas);/*Le podríamos poner al listen
				SOMAXCONN como segundo parámetro, y significaría el máximo tamaño de la cola*/
    if(valorListen == ERROR) {
        puts("El servidor no pudo recibir escuchar conexiones de clientes.\n");
    } else	{
        printf("¡Hola, estoy escuchando!");
        fflush(stdout);
        sleep(1);
    }
}

int crearSocketEscucha (int puerto) {

    int socketDeEscucha = crearSocketServidor(puerto);

    //Escuchar conexiones
    escucharSocketsEn(socketDeEscucha);

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

    cliente = crearSocket(); //Creamos socket
    int valorConnect = connect(cliente, (struct sockaddr *) &direccionServidor, sizeof(direccionServidor));

    if(valorConnect == ERROR)  {
        log_error(logger, "No se pudo establecer conexión entre el socket y el servidor.");
        return ERROR;
    }
    else {
        log_info(logger, "¡Socket conectado a Servidor!");
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
    list_add(conexion->conexiones, fdNuevoServidor);
    conexion->descriptorMaximo = getFdMaximo(conexion);

    return *fdNuevoServidor;
}

void levantarServidor(int puerto, GestorConexiones* conexion) {
    int fdServidor = crearSocketEscucha(puerto);
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
    bool esElClienteDesconectado(void* cliente)	{
        return *((int*) cliente) == fdCliente;
    }
    list_remove_and_destroy_by_condition(unaConexion->conexiones, esElClienteDesconectado, free);
    unaConexion->descriptorMaximo = getFdMaximo(unaConexion);
    log_info(logger, "El socket %i ha cerrado la conexión (posiblemente haya sido terminado).", fdCliente);
}

void enviarPaquete(int fdDestinatario, char* mensaje)  {
    int pesoMensaje = (strlen(mensaje) + 1) * sizeof(char);
    Header header = armarHeader(fdDestinatario, pesoMensaje);
    void* headerSerializado = serializarHeader(header);
    int pesoPaquete = sizeof(Header) + pesoMensaje;
    void* paquete = empaquetar(headerSerializado, mensaje);
    send(fdDestinatario, paquete, pesoPaquete, MSG_DONTWAIT);
    free(headerSerializado);
    free(paquete);
}

void* empaquetar(void* headerSerializado, char* mensaje)   {
    int pesoMensaje = (strlen(mensaje) + 1) * sizeof(char);
    void* paquete = (void*) malloc(sizeof(Header) + pesoMensaje);
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

    memcpy(puntero, &(header.tamanioMensaje), tamanioSize);
    puntero += tamanioSize;
    memcpy(puntero, &(header.fdRemitente), pesoTipoRemitente);

    return headerSerializado;
}

Header deserializarHeader(void* headerSerializado)	{
    Header header;
    void* puntero = headerSerializado;
    int tamanioSize = sizeof(typeof(header.tamanioMensaje));
    int pesoTipoRemitente = sizeof(typeof(header.fdRemitente));

    memcpy(&(header.tamanioMensaje), puntero, tamanioSize);
    puntero += tamanioSize;

    memcpy(&(header.fdRemitente), puntero, pesoTipoRemitente);

    return header;
}

Header armarHeader(int fdDestinatario, int tamanioDelMensaje)    {
    Header header = {.tamanioMensaje = tamanioDelMensaje, .fdRemitente = fdDestinatario};
    return header;
}

GestorConexiones* inicializarConexion() {
    GestorConexiones* nuevaConexion = (GestorConexiones*) malloc(sizeof(GestorConexiones));

    nuevaConexion->descriptorMaximo = 0;
    nuevaConexion->servidor = 0;
    nuevaConexion->conexiones = list_create();

    return nuevaConexion;
}