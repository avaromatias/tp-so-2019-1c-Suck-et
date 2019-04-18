#include "conexiones.h"

void crearHiloServidor(int puerto, void (*gestionarMensajes)(Header, void*), void (*gestionarDesconexiones)(int), void (*gestionarConexiones)(int))    {
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
    int tamanioSize = sizeof(typeof(header.size));

	memcpy(puntero, &(header.tamanioMensaje), tamanioSize);
	puntero += tamanioSize;

	return headerSerializado;
}

Header deserializarHeader(void* headerSerializado)	{
	Header header;
	void* puntero = headerSerializado;
	int tamanioSize = sizeof(typeof(header.size));

	memcpy(&(header.size), puntero, tamanioSize);
	puntero += tamanioSize;

	return header;
}

Header armarHeader(int tamanioDelMensaje)    {
    Header header = {.tamanioMensaje = tamanioDelMensaje};
    return header;
}

Message* empaquetarTexto(char* texto, unsigned int length){
	if (texto == NULL || length < 1)
		return NULL;
	Message *msg = malloc(sizeof(Message));
	msg->header = malloc(sizeof(Header));
	msg->header->size = length + 1;
	msg->contenido = malloc(length + 1);
	memcpy(msg->contenido, texto, length);
	((char*) msg->contenido)[length] = '\0';
	return msg;
}

char* contenido_toString(Message* mensaje){
	char* retorno = malloc(mensaje->header->size + 1);

	memcpy(retorno, mensaje->contenido, mensaje->header->size);
	retorno[mensaje->header->size] = '\0';

	return retorno;
}

char** descomponer_mensaje(Message* mensaje){
	printf("Voy a descomponer\n");
	char* contenido = contenido_toString(mensaje);
	printf("Input: %s\n", contenido);
	int cantidad_de_palabras = 0;
	for(int i = 0; i < strlen(contenido);i++){

		if(contenido[i]==';'){
			cantidad_de_palabras++;
		}

	}

	cantidad_de_palabras++;

	char** retorno = malloc(sizeof(char*) * cantidad_de_palabras);

	int inicio_palabra = 0;
	int letra_actual = 0;

	for(int i = 0; i < cantidad_de_palabras; i++){

		int cantidad_letras = 0;


		while(contenido[letra_actual] != ';' && strlen(contenido) > letra_actual){
			cantidad_letras++;
			letra_actual++;
		}
			letra_actual++;
		char* palabra = malloc(cantidad_letras + 1);

		memcpy(palabra, contenido + inicio_palabra, cantidad_letras);

		retorno[i] = palabra;

		printf("Palabra actual: %s\n", palabra);

		inicio_palabra += cantidad_letras + 1;

		//cantidad_de_palabras--;
	}

	for(int i = 0; i < cantidad_de_palabras; i++){
		printf("Palabra %d: %s\n", i, retorno[i]);
	}

	return retorno;
}