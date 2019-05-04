/*
 ============================================================================
 Name        : Memoria.c
 Author      : Suck et
 	 UTN FRBA - Sistemas Operativos
 	 	 TP-1C-2019-Lissandra
 ============================================================================
 */

#include "memoria.h"



t_configuracion cargarConfiguracion(char* pathArchivoConfiguracion, t_log* logger)	{
	t_configuracion configuracion;

	t_config* archivoConfig = abrirArchivoConfiguracion(pathArchivoConfiguracion, logger);

    bool existenTodasLasClavesObligatorias(t_config* archivoConfig, t_configuracion configuracion)	{
        char* clavesObligatorias[11] = {
                "PUERTO",
                "IP_FS",
                "PUERTO_FS",
                "IP_SEEDS",
                "PUERTO_SEEDS",
                "RETARDO_MEM",
                "RETARDO_FS",
                "TAM_MEM",
                "RETARDO_JOURNAL",
                "RETARDO_GOSSIPING",
                "MEMORY_NUMBER"
        };

        for(int i = 0; i < COUNT_OF(clavesObligatorias); i++)	{
            if(!config_has_property(archivoConfig, clavesObligatorias[i]))
                return false;
        }

        return true;
    }

	if(!existenTodasLasClavesObligatorias(archivoConfig, configuracion)){
		log_error(logger, "Alguna de las claves obligatorias no están setteadas en el archivo de configuración.");
        config_destroy(archivoConfig);
		exit(1); // settear algún código de error para cuando falte alguna key
	}
	else	{
		configuracion.puerto = config_get_int_value(archivoConfig, "PUERTO");
		configuracion.ipFileSystem = config_get_string_value(archivoConfig, "IP_FS");
		configuracion.puertoFileSystem = config_get_int_value(archivoConfig, "PUERTO_FS");
		configuracion.ipSeeds = config_get_array_value(archivoConfig, "IP_SEEDS");
		configuracion.puertoSeeds = (int*) config_get_array_value(archivoConfig, "PUERTO_SEEDS");
		configuracion.retardoMemoria = config_get_int_value(archivoConfig, "RETARDO_MEM");
		configuracion.retardoFileSystem = config_get_int_value(archivoConfig, "RETARDO_FS");
		configuracion.tamanioMemoria = config_get_int_value(archivoConfig, "TAM_MEM");
		configuracion.retardoJournal = config_get_int_value(archivoConfig, "RETARDO_JOURNAL");
		configuracion.retardoGossiping = config_get_int_value(archivoConfig, "RETARDO_GOSSIPING");
		configuracion.cantidadDeMemorias = config_get_int_value(archivoConfig, "MEMORY_NUMBER");

        config_destroy(archivoConfig);

		return configuracion;
	}
}

void atenderMensajes(Header header, char* mensaje)    {
    printf("Estoy recibiendo un mensaje del file descriptor %i: %s", header.fdRemitente, mensaje);
    fflush(stdout);

    char** arrayMensaje = parser(mensaje);

    //char** arrayMensaje = parser("SELECT amigues");


    if (strcmp(arrayMensaje[0], "SELECT") == 0){
        printf("Recibi un select");

        //Todo chequear que las queries traigan la cantidad correcta de parámetros
        //TODO crear un segmento y una página
        //TODO buscar dentro del segmento lo que se pidió ELSE

        enviarPaquete(FD_FS, mensaje);
        Header *header = (Header*)malloc(sizeof(Header));
        recv(FD_CLIENTE, header, sizeof(Header), NULL);
        char* respuesta = malloc(header->tamanioMensaje);
        recv(FD_CLIENTE, respuesta, header->tamanioMensaje, NULL);
        printf("%s", mensaje);

    }else if (strcmp(arrayMensaje[0], "INSERT") == 0){
        printf("Recibi un insert");
    }else if (strcmp(arrayMensaje[0], "CREATE") == 0){
        printf("Recibi un create");
    }else if (strcmp(arrayMensaje[0], "DESCRIBE") == 0){
        printf("Recibi un describe");
    }else if (strcmp(arrayMensaje[0], "DROP") == 0){
        printf("Recibi un drop");
    }else if (strcmp(arrayMensaje[0], "JOURNAL") == 0){
        printf("Recibi un journal");
    }else{
        printf("No entendi tu mensaje bro");
    }
    printf("\n");
    fflush(stdout);
}

//void startUp(){
//    FD_FS = crearSocketCliente(configuracion.ipFileSystem, configuracion.puertoFileSystem);
//    //TODO Pedirle el TAMAÑO_VALUE al FS
//    /*enviarPaquete(FD_FS, "DAME EL TAM_VALUE");
//    Header *header = (Header*)malloc(sizeof(Header));
//    recv(cliente, header, sizeof(Header), NULL);
//    char* mensaje = malloc(header->tamanioMensaje);
//    recv(cliente, mensaje, header->tamanioMensaje, NULL);
//    printf("%s", mensaje);*/
//
//    printf("Reservo bloque de memoria ");
//
//    memoria.direcciones = (void*) malloc(configuracion.tamanioMemoria);
//    memoria.tamanioMemoria = configuracion.tamanioMemoria;
//
//}

// funciones propias de cada módulo para las conexiones

pthread_t* crearHiloConexiones(GestorConexiones* unaConexion, t_log* logger)    {
	pthread_t* hiloConexiones = malloc(sizeof(pthread_t));

	parametros_thread* parametros = (parametros_thread*) malloc(sizeof(parametros_thread));

	parametros->conexion = unaConexion;
	parametros->logger = logger;

	pthread_create(hiloConexiones, NULL, &atenderConexiones, parametros);

	return hiloConexiones;
}

void* atenderConexiones(void* parametrosThread)    {
    parametros_thread* parametros = (parametros_thread*) parametrosThread;
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
                            char* mensaje = (char*) malloc(pesoMensaje);
                            int n = strlen(mensaje);
                            bytesRecibidos = recv(fdConectado, mensaje, pesoMensaje, MSG_DONTWAIT);
                            if(bytesRecibidos == -1 || bytesRecibidos < pesoMensaje)
                                log_warning(logger, "Hubo un error al recibir el mensaje proveniente del socket %i", fdConectado);
                            else if(bytesRecibidos == 0)	{
                                // acá cada uno setea una maravillosa función que hace cada uno cuando se le desconecta alguien
                                // nombre_maravillosa_funcion();
                                desconectarCliente(fdConectado, unaConexion, logger);
                            }
                            else	{
                                // acá cada uno setea una maravillosa función que hace cada uno cuando le llega un nuevo mensaje
                                // nombre_maravillosa_funcion();
                                int tamanioMensaje = strlen(mensaje);
                                atenderMensajes(header, mensaje);
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

int main(void) {
    t_log* logger = log_create("memoria.log", "memoria", true, LOG_LEVEL_INFO);

	t_configuracion configuracion = cargarConfiguracion("memoria.cfg", logger);

	GestorConexiones* misConexiones = inicializarConexion();

    levantarServidor(configuracion.puerto, misConexiones);

    pthread_t* hiloConexiones = crearHiloConexiones(misConexiones, logger);


//    FD_CLIENTE = crearSocketCliente("127.0.0.1", 5003);
//    enviarPaquete(FD_CLIENTE, "Hola, soy la memoria");
//    Header *header = (Header*)malloc(sizeof(Header));
//    recv(FD_CLIENTE, header, sizeof(Header), NULL);
//    char* mensaje = malloc(header->tamanioMensaje);
//    recv(FD_CLIENTE, mensaje, header->tamanioMensaje, NULL);
//    printf("%s", mensaje);
//    fflush(stdout);
//
//	char* linea;
//	while(1){
//	    /* Imprimo lo que ingreso por pantalla
//	     * queda comentado porque el crearHiloServido me bloquea el proceso*/
//	    linea = readline(">");
//        if (!linea) {
//            break;
//        }
//        printf("%s\n", linea);
//        free(linea);
//	};
    pthread_join(*hiloConexiones, NULL);

	return 0;
}
