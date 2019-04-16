/*
 ============================================================================
 Name        : Sockets.h
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

    //Hasta que no salga del listen, nunca va a retornar el socket del servidor ya que el listen es bloqueante

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