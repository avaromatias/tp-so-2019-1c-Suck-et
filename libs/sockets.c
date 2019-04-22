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
        puts("¡Hola, estoy escuchando!");
    }
}

int crearSocketEscucha (int puerto) {

    int socketDeEscucha = crearSocketServidor(puerto);

    //Escuchar conexiones
    escucharSocketsEn(socketDeEscucha);

    return socketDeEscucha;
}

int aceptarCliente(int fd_socket){

    struct sockaddr_in unCliente;
    unsigned int addres_size = sizeof(unCliente);

    int fd_Cliente = accept(fd_socket, (struct sockaddr*) &unCliente, &addres_size);
    if(fd_Cliente == ERROR)  {
        puts("El servidor no pudo aceptar la conexión entrante.\n");
    } else	{
        puts("¡Estamos conectados!");
    }

    return fd_Cliente;

}

//Creamos un Socket Cliente -ya que tenemos antes el Servidor-
int crearSocketCliente(char *ipServidor, int puerto) {

    int cliente;
    struct sockaddr_in direccionServidor;

    direccionServidor.sin_family = AF_INET;				// Ordenación de bytes de la máquina
    direccionServidor.sin_addr.s_addr = inet_addr(ipServidor);
    direccionServidor.sin_port = htons(puerto);			// short, Ordenación de bytes de la red
    memset(&(direccionServidor.sin_zero), '\0', 8); 	// Pone cero al resto de la estructura

    cliente = crearSocket(); //Creamos socket
    int valorConnect = connect(cliente, (struct sockaddr *) &direccionServidor, sizeof(direccionServidor));

    if(valorConnect == ERROR)  {
        perror("No se pudo establecer conexión entre el socket y el servidor.\n");
        return ERROR;
    }
    else {
        puts("¡Socket conectado a Servidor!");
        return cliente;
    }
}

void cerrarSocket(int fd_socket) {

    int cerrar = close(fd_socket);
    if (cerrar == ERROR) {
        printf("No se pudo cerrar el file descriptor del socket: %d\n",fd_socket);
    }
}

void crearHiloServidor(int puerto, void (*gestionarMensajes)(Header, char*), void (*gestionarDesconexiones)(int), void (*gestionarConexiones)(int))    {
    int socketServidor = crearSocketEscucha(puerto);

    t_conexion unaConexion;
    unaConexion.servidor = socketServidor;
    unaConexion.descriptorMaximo = socketServidor;
    unaConexion.clientes = list_create();

    pthread_t* hiloConexiones = malloc(sizeof(pthread_t));

    parametros_thread parametros = {.conexion = unaConexion, .funcionRecepcionMensajes = gestionarMensajes, .funcionDesconexionClientes = gestionarDesconexiones, .funcionConexionClientes = gestionarConexiones};

    pthread_create(hiloConexiones, NULL, &atenderConexiones, &parametros);

    pthread_join(*hiloConexiones, NULL);
}

void cargarListaClientes(t_conexion* unaConexion, fd_set* solicitantes, fd_set* respondidos)	{
    FD_ZERO(solicitantes);
    FD_ZERO(respondidos);

    FD_SET(unaConexion->servidor, solicitantes);
    t_link_element * unCliente = (unaConexion->clientes)->head;

    while(unCliente != NULL)	{
        int fdCliente = *((int*) unCliente->data);
        FD_SET(fdCliente, solicitantes);
        unCliente = unCliente->next;
    }
}

// esta función se puede modularizar más... definiendo funciones para cuando recibimos mensajes, cuando se conecta alguien, cuando se desconecta alguien...
void * atenderConexiones(void* parametrosThread)	{
    parametros_thread* parametros = (parametros_thread*) parametrosThread;
    t_conexion unaConexion = parametros->conexion;

    fd_set solicitantes;
    fd_set respondidos;

    while(1){
        // inicializo las colas de clientes
        cargarListaClientes(&unaConexion, &solicitantes, &respondidos);

        // cuando haga el select, el thread / programa se va a quedar colgado, y cuando salga, será con alguna solicitud (o error)
        int cambios = select(unaConexion.descriptorMaximo + 1, &solicitantes, &respondidos, NULL, NULL);

        // si hay nuevos mensajes, conexiones, o lo que sea, lo trato
        if(cambios > 0)	{
            // recorro todos los clientes
            t_link_element * unCliente = unaConexion.clientes->head;
            while(unCliente != NULL)	{
                void* headerSerializado = malloc(sizeof(Header));
                int fdCliente = *((int*) unCliente->data);

                // me fijo si hay algún cliente enviándome un mensaje
                if(FD_ISSET(fdCliente, &solicitantes))	{
                    // recibo el header
                    int bytesRecibidos = recv(fdCliente, headerSerializado, sizeof(Header), MSG_DONTWAIT);
                    switch(bytesRecibidos)  {
                        // hubo un error al recibir los datos
                        case -1:
                            printf("Hubo un error al recibir el header proveniente del socket %i\n", fdCliente);
                            fflush(stdout);// quizás sea mejor retornar un código de error definido por nosotros, y que cada proceso se encargue de manejarlo como más le parezca
                            break;
                            // se desconectó
                        case 0:
                            if(parametros->funcionDesconexionClientes != NULL)
                                parametros->funcionDesconexionClientes(fdCliente);
                            desconectarCliente(fdCliente, unaConexion);
                            break;
                            // recibí algún mensaje
                        default: ; // esto es lo más raro que vi pero tuve que hacerlo
                            Header header = deserializarHeader(headerSerializado);
                            header.fdRemitente = fdCliente;
                            void* mensaje = (void*) malloc(header.tamanioMensaje);
                            bytesRecibidos = recv(fdCliente, mensaje, header.tamanioMensaje, MSG_DONTWAIT);
                            if(bytesRecibidos == -1 || bytesRecibidos < header.tamanioMensaje)	{
                                printf("Hubo un error al recibir el mensaje proveniente del socket %i\n", fdCliente);
                                fflush(stdout);// quizás sea mejor retornar un código de error definido por nosotros, y que cada proceso se encargue de manejarlo como más le parezca
                            }
                            else if(bytesRecibidos == 0)	{
                                if(parametros->funcionDesconexionClientes != NULL)
                                    parametros->funcionDesconexionClientes(fdCliente);
                                desconectarCliente(fdCliente, unaConexion);
                            }
                            else	{
                                if(parametros->funcionRecepcionMensajes != NULL)
                                    parametros->funcionRecepcionMensajes(header, mensaje);
                            }
                            break;
                    }
                }

                unCliente = unCliente->next;

                free(headerSerializado);
            }

            // si hay un nuevo cliente esperando conectarse con el servidor, lo acepto
            if(FD_ISSET(unaConexion.servidor, &solicitantes))	{
                int* fdNuevoCliente = malloc(sizeof(int));
                *fdNuevoCliente = aceptarCliente(unaConexion.servidor);
                list_add(unaConexion.clientes, fdNuevoCliente);

                //me fijo si hay que actualizar el file descriptor máximo con el del nuevo cliente
                if(unaConexion.descriptorMaximo < *fdNuevoCliente)
                    unaConexion.descriptorMaximo = *fdNuevoCliente;

                if(parametros->funcionConexionClientes != NULL)
                    parametros->funcionConexionClientes(*fdNuevoCliente);
            }
        }
    }

}

void desconectarCliente(int fdCliente, t_conexion unaConexion)	{
    printf("El socket %i ha cerrado la conexión (posiblemente haya sido terminado).\n", fdCliente);
    fflush(stdout);// quizás sea mejor retornar un código de error definido por nosotros, y que cada proceso se encargue de manejarlo como más le parezca
    bool esElClienteDesconectado(int* cliente)	{
        return *cliente == fdCliente;
    }
    list_remove_and_destroy_by_condition(unaConexion.clientes, (void*) esElClienteDesconectado, free);
}

void enviarPaquete(int fdDestinatario, char* mensaje)  {
    int tamanioDelMensaje = strlen(mensaje) * sizeof(char);
    Header header = armarHeader(tamanioDelMensaje);
    void* headerSerializado = serializarHeader(header);
    void* paquete = empaquetar(headerSerializado, mensaje);
    send(fdDestinatario, paquete, sizeof(Header) + strlen(mensaje), MSG_DONTWAIT);
}

void* empaquetar(void* headerSerializado, char* mensaje)   {
    void* paquete = (void*) malloc(sizeof(Header) + strlen(mensaje));
    void* puntero = paquete;
    memcpy(puntero, headerSerializado, sizeof(Header));
    puntero += sizeof(Header);
    memcpy(puntero, mensaje, strlen(mensaje));
    return paquete;
}

void* serializarHeader(Header header)    {
    void* headerSerializado = (void*) malloc(sizeof(Header));
    void* puntero = headerSerializado;
    int tamanioSize = sizeof(typeof(header.tamanioMensaje));

    memcpy(puntero, &(header.tamanioMensaje), tamanioSize);
    puntero += tamanioSize;

    return headerSerializado;
}

Header deserializarHeader(void* headerSerializado)	{
    Header header;
    void* puntero = headerSerializado;
    int tamanioSize = sizeof(typeof(header.tamanioMensaje));

    memcpy(&(header.tamanioMensaje), puntero, tamanioSize);
    puntero += tamanioSize;

    return header;
}

Header armarHeader(int tamanioDelMensaje)    {
    Header header = {.tamanioMensaje = tamanioDelMensaje};
    return header;
}

//Message* empaquetarTexto(char* texto, unsigned int length){
//    if (texto == NULL || length < 1)
//        return NULL;
//    Message *msg = malloc(sizeof(Message));
//    msg->header = malloc(sizeof(Header));
//    msg->header->size = length + 1;
//    msg->contenido = malloc(length + 1);
//    memcpy(msg->contenido, texto, length);
//    ((char*) msg->contenido)[length] = '\0';
//    return msg;
//}