/*
 ============================================================================
 Name        : LFS.c
 Author      : Suck et
 	 UTN FRBA - Sistemas Operativos
 	 	 TP-1C-2019-Lissandra
 ============================================================================
 */

#include "lfs.h"

t_configuracion cargarConfiguracion(char *pathArchivoConfiguracion, t_log *logger) {

    t_config *archivoConfig = abrirArchivoConfiguracion(pathArchivoConfiguracion, logger);

    bool existenTodasLasClavesObligatorias(t_config *archivoConfig, t_configuracion configuracion) {
        char *clavesObligatorias[5] = {
                "PUERTO_ESCUCHA",
                "PUNTO_MONTAJE",
                "RETARDO",
                "TAMAÑO_VALUE",
                "TIEMPO_DUMP"
        };

        for (int i = 0; i < COUNT_OF(clavesObligatorias); i++) {
            if (!config_has_property(archivoConfig, clavesObligatorias[i]))
                return false;
        }

        return true;
    }

    if (!existenTodasLasClavesObligatorias(archivoConfig, configuracion)) {
        log_error(logger, "Alguna de las claves obligatorias no están setteadas en el archivo de configuración.");
        exit(1); // settear algún código de error para cuando falte alguna key
    } else {
        configuracion.puertoEscucha = config_get_int_value(archivoConfig, "PUERTO_ESCUCHA");
        char *puntoMontaje = config_get_string_value(archivoConfig, "PUNTO_MONTAJE");
        configuracion.puntoMontaje = (char *) malloc(sizeof(char) * strlen(puntoMontaje));
        strcpy(configuracion.puntoMontaje, puntoMontaje);
        configuracion.retardo = config_get_int_value(archivoConfig, "RETARDO");
        configuracion.tamanioValue = config_get_int_value(archivoConfig, "TAMAÑO_VALUE");
        configuracion.tiempoDump = config_get_int_value(archivoConfig, "TIEMPO_DUMP");

        return configuracion;
    }
}

void atenderMensajes(Header header, char *mensaje, parametros_thread_lfs *parametros) {
    if (header.tipoMensaje == HANDSHAKE) {
        enviarPaquete(header.fdRemitente, REQUEST, "Hola, soy Lissandra");
        printf("Estoy recibiendo un mensaje del File Descriptor %i: %s", header.fdRemitente, mensaje);
        fflush(stdout);
    } else if (header.tipoMensaje == REQUEST) {
        pthread_t *hiloRequest = crearHiloRequest(mensaje);
    }

}

pthread_t *crearHiloRequest(char *mensaje) {
    pthread_t *hiloRequest = malloc(sizeof(pthread_t));
    parametros_thread_request *parametros = (parametros_thread_request *) malloc(sizeof(parametros_thread_request));
    parametros->comando = mensaje;
    pthread_create(hiloRequest, NULL, &procesarComandoPorRequest, parametros);
    return hiloRequest;
}

char *obtenerPathArchivo(char *nombreTabla, char *nombreArchivo) {
    char *path = string_new();
    char *tablePath = obtenerPathTabla(nombreTabla, configuracion.puntoMontaje);
    string_append(&path, tablePath);
    string_append(&path, "/");
    string_append(&path, nombreArchivo);
    return path;
}

char *obtenerPathBloque(int numberoBloque) {
    char *unArchivoDeBloque = string_new();
    string_append(&unArchivoDeBloque, "..");
    string_append(&unArchivoDeBloque, configuracion.puntoMontaje);
    string_append(&unArchivoDeBloque, "Bloques/");
    string_append(&unArchivoDeBloque,string_from_format("%i", numberoBloque));
    string_append(&unArchivoDeBloque, ".bin");
    return unArchivoDeBloque;
}

void valorSinComillas(char *valor) {
    if(string_starts_with(valor, "\"") && string_ends_with(valor, "\"")) {
        int j = 0;
        for (int i = 0; i < strlen(valor); i++) {
            if (valor[i] == '\\') {
                valor[j++] = valor[i++];
                valor[j++] = valor[i];
                if (valor[i] == '\0')
                    break;
            } else if (valor[i] != '"')
                valor[j++] = valor[i];
        }
        valor[j] = '\0';
    }
}

char *armarLinea(char *key, char *valor, time_t timestamp) {
    char *linea = string_new();
    string_append(&linea, string_from_format("%ld",timestamp));
    string_append(&linea, ";");
    string_append(&linea, key);
    string_append(&linea, ";");
    valorSinComillas(valor);
    string_append(&linea, valor);
    string_append(&linea, "\n");
    return linea;
}

char **desarmarLinea(char *linea) {
    return string_split(linea, ";");
}

int validarConsistencia(char *tipoConsistencia) {
    if (strcmp(tipoConsistencia, "SC") == 0 || strcmp(tipoConsistencia, "SHC") == 0 ||
        strcmp(tipoConsistencia, "EC") == 0) {
        return 0;
    }
    return -1;
}

int validarValor(char *valor) {
    if (string_starts_with(valor, "\"") && string_ends_with(valor, "\"")) {
        return 0;
    }
    return -1;
}

void crearMetadata(char *nombreTabla, char *tipoConsistencia, char *particiones, char *tiempoCompactacion) {
    FILE *f = fopen(obtenerPathArchivo(nombreTabla, "Metadata"), "w");
    char *lineaConsistencia = string_new();
    string_append(&lineaConsistencia, "CONSISTENCY=");
    string_append(&lineaConsistencia, tipoConsistencia);
    string_append(&lineaConsistencia, "\n");
    char *lineaParticiones = string_new();
    string_append(&lineaParticiones, "PARTITIONS=");
    string_append(&lineaParticiones, particiones);
    string_append(&lineaParticiones, "\n");
    char *lineaCompactacion = string_new();
    string_append(&lineaCompactacion, "COMPACTION_TIME=");
    string_append(&lineaCompactacion, tiempoCompactacion);
    string_append(&lineaCompactacion, "\n");
    fwrite(lineaConsistencia, sizeof(char) * strlen(lineaConsistencia), 1, f);
    fwrite(lineaParticiones, sizeof(char) * strlen(lineaParticiones), 1, f);
    fwrite(lineaCompactacion, sizeof(char) * strlen(lineaCompactacion), 1, f);
    fclose(f);
}

void crearBinarios(char *nombreTabla, int particiones) {
    for(int i = 0; i < particiones; i++) {
        int bloque = obtenerBloqueLibreAsignado();
        if(bloque != -1) {
            char *nombreArchivo = string_new();
            string_append(&nombreArchivo, string_from_format("%i", i));
            string_append(&nombreArchivo, ".bin");
            FILE *file = fopen(obtenerPathArchivo(nombreTabla, nombreArchivo), "w");
            char *contenido = string_new();
            string_append(&contenido, "SIZE=");
            int tamanio = obtenerTamanioBloque(bloque);
            string_append(&contenido, string_from_format("%i", tamanio));
            string_append(&contenido, "\n");
            string_append(&contenido, "BLOCKS=[");
            string_append(&contenido, string_from_format("%i", bloque));
            string_append(&contenido, "]");
            string_append(&contenido, "\n");
            fwrite(contenido, sizeof(char) * strlen(contenido), 1, file);
            fclose(file);
        }
    }
}

int obtenerTamanioBloque(int bloque) {
    char *pathBloque = obtenerPathBloque(bloque);
    if(archivoVacio(pathBloque)) {
        return 0;
    } else {
        FILE *bloque = fopen(pathBloque, "r");
        char ch;
        int count = 0;
        while((ch = fgetc(bloque)) != EOF) {
            if(ch != '\n') {
                count++;
            }
        }
        fclose(bloque);
        return count;
    }
}

bool estaLibreElBloque(int bloque) { //TODO: Implementar el test bit
    return bitarray_test_bit(bitmap, bloque);
}

int obtenerBloqueLibreAsignado() {
    for(int i = 0; i < obtenerCantidadBloques(configuracion.puntoMontaje); i++) {
        if(estaLibreElBloque(i)) {
            return i;
        }
    }
    return -1;
}

void lfsCreate(char *nombreTabla, char *tipoConsistencia, char *particiones, char *tiempoCompactacion) {
    if (validarConsistencia(tipoConsistencia) != 0) {
        log_warning(logger, "El Tipo de Consistencia no es valido. Este puede ser SC, SHC o EC.");
    } else {
        char *tablePath = obtenerPathTabla(nombreTabla,configuracion.puntoMontaje);
        // Verificar que la tabla no exista en el file system.
        if (existeElArchivo(tablePath)) {
            char *path = obtenerPathMetadata(nombreTabla,configuracion.puntoMontaje);
            // En caso que exista, se guardará el resultado en un archivo .log y se retorna un error indicando dicho resultado.
            log_info(logger, "La tabla %s ya existe.\n", nombreTabla);
            if (!existeElArchivo(path)) {
                crearMetadata(nombreTabla, tipoConsistencia, particiones, tiempoCompactacion);
                printf("Se creo la Metadata de la tabla %s.\n", nombreTabla);
            }
        } else {
            // Crear el directorio para dicha tabla.
            mkdir(tablePath, 0777);
            // Crear el directorio para dicha tabla.
            // Grabar en dicho archivo los parámetros pasados por el request.
            crearMetadata(nombreTabla, tipoConsistencia, particiones, tiempoCompactacion);
            printf("Se creo la Metadata de la tabla %s.\n", nombreTabla);
            // Crear los archivos binarios asociados a cada partición de la tabla y
            // asignar a cada uno un bloque
            crearBinarios(nombreTabla,atoi(particiones));
        }
    }
}

void lfsInsert(char *nombreTabla, char *key, char *valor, time_t timestamp) {
    if (validarValor(valor) != 0) {
        log_warning(logger, "El valor debe estar enmascarado con \"\"");
    } else {
        // Verificar que la tabla exista en el file system. En caso que no exista, informa el error y continúa su ejecución.
        if (existeTabla(nombreTabla) == 0) {
            // Obtener la metadata asociada a dicha tabla.
            char *path = obtenerPathMetadata(nombreTabla,configuracion.puntoMontaje);
            if (existeElArchivo(path)) {
                printf("Existe metadata en %s\n", path);
                // TODO: Verificar si existe en memoria una lista de datos a dumpear. De no existir, alocar dicha memoria.
                char *linea = armarLinea(key, valor, timestamp);
                obtenerMetadata(nombreTabla);
                t_metadata *meta = dictionary_get(metadatas, nombreTabla);
                int particion = calcularParticion(key, meta);
                char *nombreArchivo = string_new();
                char *p = string_itoa(particion);
                string_append(&nombreArchivo, p);
                string_append(&nombreArchivo, ".bin");
                FILE *f = fopen(obtenerPathArchivo(nombreTabla, nombreArchivo), "a");
                printf("Linea %s\n", linea);
                // TODO: Insertar en la memoria temporal del punto anterior una nueva entrada que contenga los datos enviados en la request.
                fwrite(linea, sizeof(char) * strlen(linea), 1, f);
                fclose(f);
                free(path);
            } else {
                printf("No existe metadata en %s\n", path);
            }
        }

    }
}

void lfsSelect(char *nombreTabla, char *key) {
    //1. Verificar que la tabla exista en el File System
    if(existeTabla(nombreTabla) == 0) {

        //2. Obtener la metadata asociada a dicha tabla
        obtenerMetadata(nombreTabla);
        t_metadata *meta = dictionary_get(metadatas, nombreTabla);

        //3. Calcular cual es la partición que contiene dicho KEY
        int particion = calcularParticion(key, meta);

        //4. Escanear la partición objetivo, todos los archivos temporales y la memoria temporal de dicha tabla (si existe) buscando la key deseada
        char *nombreArchivo = string_new();
        char *p = string_itoa(particion);
        string_append(&nombreArchivo, p);
        string_append(&nombreArchivo, ".bin");
        char *archivePath = obtenerPathArchivo(nombreTabla, nombreArchivo);
        FILE *fd = fopen(archivePath, "r");

        char seek;
        char **palabras;
        char *linea;
        char str[2];
        str[1] = '\0';
        char *keyEncontrado;
        char *timestampEncontrado;
        int mayorTimestamp = 0;
        char *valorMayorTimestamp;

        //4.1. Escaneo particion objetivo
        while (!feof(fd)) {
            linea = string_new();
            keyEncontrado = string_new();
            timestampEncontrado = string_new();
            while ((seek = getc(fd)) != EOF && seek != '\n') {
                str[0] = seek;
                string_append(&linea, str);
            }
            palabras = desarmarLinea(linea);
            string_append(&timestampEncontrado, palabras[0]);
            string_append(&keyEncontrado, palabras[1]);
            if (strcmp(keyEncontrado, key) == 0 && (atoi(timestampEncontrado) > mayorTimestamp)) {
                mayorTimestamp = atoi(timestampEncontrado);
                valorMayorTimestamp = string_new();
                string_append(&valorMayorTimestamp, palabras[2]);
            }
        }

        fclose(fd);

        //4.2. Escaneo los archivos temporales


        //4.2. Escaneo la memoria temporal de la tabla


        //5. Encontradas las entradas para dicha Key, se retorna el valor con el Timestamp más grande
        printf("Value: %s\n", valorMayorTimestamp);
    }
}

void ejecutarConsola(void *parametrosConsola) {
    parametros_consola *parametros = (parametros_consola *) parametrosConsola;

    t_log *logger = parametros->logger;
    int (*gestionarComando)(char **) = parametros->gestionarComando;
    Componente nombreDelProceso = parametros->unComponente;

    char *comando;
    char *nombreDelGrupo = "@suck-ets:~$ ";
    char *prompt = string_new();
    string_append(&prompt, "Lissandra");
    string_append(&prompt, nombreDelGrupo);
    do {
        char *leido = readline(prompt);
        comando = malloc(sizeof(char) * strlen(leido) + 1);
        memcpy(comando, leido, strlen(leido));
        comando[strlen(leido)] = '\0';
        comando = procesarComando(comando);
    } while (strcmp(comando, "exit") != 0);
    free(comando);
    printf("Ya se analizo todo lo solicitado.\n");
}

void *procesarComandoPorRequest(void *params) {
    parametros_thread_request *parametros = (parametros_thread_request *) params;
    return procesarComando(parametros->comando);
}

char *procesarComando(char *comando) {
    char **comandoParseado = parser(comando);
    /*if (strcmp("exit", comando) != 0 && validarComandosComunes(comandoParseado) == 1) {*/ //TODO: Descomentar cuando salga fix del validarComandosComunes
    if (strcmp("exit", comando) != 0) {
        if (gestionarRequest(comandoParseado) == 0) {
            log_info(logger, "Request procesada correctamente.");
        } else {
            log_error(logger, "No se pudo procesar la request solicitada (%s).", comando);
        };
    }
    string_to_lower(comando);
    return comando;
}

int gestionarRequest(char **request) {
    char *tipoDeRequest = request[0];
    char *nombreTabla = request[1];
    char *param1 = request[2];
    char *param2 = request[3];
    char *param3 = request[4];
    string_to_upper(tipoDeRequest);

    if (strcmp(tipoDeRequest, "SELECT") == 0) {
        printf("Tipo de Request: %s\n", tipoDeRequest);
        printf("Tabla: %s\n", nombreTabla);
        printf("Key: %s\n", param1);
        lfsSelect(nombreTabla, param1);
        return 0;

    } else if (strcmp(tipoDeRequest, "INSERT") == 0) {
        printf("Tipo de Request: %s\n", tipoDeRequest);
        printf("Tabla: %s\n", nombreTabla);
        printf("Key: %s\n", param1);
        printf("Valor: %s\n", param2);
        time_t timestamp;
        // El parámetro Timestamp es opcional.
        // En caso que un request no lo provea (por ejemplo insertando un valor desde la consola),
        // se usará el valor actual del Epoch UNIX.
        if (param3 != NULL) {
            timestamp = (time_t) strtol(param3, NULL, 10);
        } else {
            timestamp = (time_t) time(NULL);
        }
        printf("Timestamp: %i\n", (int) timestamp);
        lfsInsert(nombreTabla, param1, param2, timestamp);
        return 0;

    } else if (strcmp(tipoDeRequest, "CREATE") == 0) {
        printf("Tipo de Request: %s\n", tipoDeRequest);
        printf("Tabla: %s\n", nombreTabla);
        printf("TIpo de consistencia: %s\n", param1);
        printf("Numero de particiones: %s\n", param2);
        printf("Tiempo de compactacion: %s\n", param3);
        lfsCreate(nombreTabla, param1, param2, param3);
        return 0;

    } else if (strcmp(tipoDeRequest, "DESCRIBE") == 0) {
        printf("Tipo de Request: %s\n", tipoDeRequest);
        if (nombreTabla == NULL) {
            // Hacer describe global
        } else {
            printf("Tabla: %s\n", nombreTabla);
            // Hacer describe de una tabla especifica
        }
        return 0;

    } else if (strcmp(tipoDeRequest, "DROP") == 0) {
        printf("Tipo de Request: %s\n", tipoDeRequest);
        printf("Tabla: %s\n", nombreTabla);
        return 0;

    } else if (strcmp(tipoDeRequest, "HELP") == 0) {
        printf("************ Comandos disponibles ************\n");
        printf("- SELECT [NOMBRE_TABLA] [KEY]\n");
        printf("- INSERT [NOMBRE_TABLA] [KEY] “[VALUE]” [Timestamp](Opcional)\n");
        printf("- CREATE [NOMBRE_TABLA] [TIPO_CONSISTENCIA] [NUMERO_PARTICIONES] [COMPACTION_TIME]\n");
        printf("- DESCRIBE [NOMBRE_TABLA](Opcional)\n");
        printf("- DROP [NOMBRE_TABLA]\n");
        printf("- EXIT\n");
        return 0;

    } else if (strcmp(tipoDeRequest, "EXIT") == 0) {
        return 0;

    } else {
        printf("Ingrese un comando valido.\n");
        return -2;
    }

}

int existeTabla(char *tabla) {
    char *tablePath = obtenerPathTabla(tabla,configuracion.puntoMontaje);
    if (!existeElArchivo(tablePath)) {
        log_error(logger, "No se encontro o no tiene permisos para acceder a la tabla %s.", tabla);
        return -1;
    }
    return 0;
}

void obtenerMetadata(char *tabla) {
    char *metadataPath = obtenerPathMetadata(tabla, configuracion.puntoMontaje);
    t_config *config = config_create(metadataPath);
    t_metadata *metadata = (t_metadata *) malloc(sizeof(t_metadata));

    if (config == NULL) {
        log_error(logger, "No se pudo obtener el archivo Metadata.");
        config_destroy(config);
        free(metadata);
        exit(1);
    }

    if (config_has_property(config, "PARTITIONS")) {
        metadata->partitions = config_get_int_value(config, "PARTITIONS");
    }

    if (config_has_property(config, "CONSISTENCY")) {
        metadata->consistency = config_get_int_value(config, "CONSISTENCY");
    }

    if (config_has_property(config, "COMPACTION_TIME")) {
        metadata->compaction_time = config_get_string_value(config, "COMPACTION_TIME");
    }

    dictionary_put(metadatas, tabla, metadata);
    config_destroy(config);
}

int calcularParticion(char *key, t_metadata *metadata) {
    int k = atoi(key);
    int b = metadata->partitions;
    return k % b;
}

pthread_t *crearHiloConexiones(GestorConexiones *unaConexion, int *fdMemoria, sem_t *kernelConectado, t_log *logger) {
    pthread_t *hiloConexiones = malloc(sizeof(pthread_t));

    parametros_thread_lfs *parametros = (parametros_thread_lfs *) malloc(sizeof(parametros_thread_lfs));

    parametros->conexion = unaConexion;
    parametros->logger = logger;
    parametros->fdMemoria = fdMemoria;
    parametros->memoriaConectada = kernelConectado;

    pthread_create(hiloConexiones, NULL, &atenderConexiones, parametros);

    return hiloConexiones;
}

char* crearDirEnPuntoDeMontaje(char *puntoMontaje, char *nombreDir) {
    char *nombreArchivo = string_new();
    string_append(&nombreArchivo, "..");
    string_append(&nombreArchivo, puntoMontaje);
    string_append(&nombreArchivo, nombreDir);

    if (!existeElArchivo(nombreArchivo)) {
        mkdir_recursive(nombreArchivo);
    }

    return nombreArchivo;
}

void crearDirTables(char* puntoMontaje) {
    char* nombreArchivo = crearDirEnPuntoDeMontaje(puntoMontaje,"/Tables");
}

int archivoVacio(char *path) {
    FILE *f = fopen(path, "r");
    int c = fgetc(f);
    fclose(f);
    return c == EOF;
}

int obtenerCantidadBloques(char *puntoMontaje) {
    char *nombreArchivo = string_new();
    string_append(&nombreArchivo, "..");
    string_append(&nombreArchivo, puntoMontaje);
    string_append(&nombreArchivo, "/Metadata/Metadata.bin");
    if (existeElArchivo(nombreArchivo) && !archivoVacio(nombreArchivo)) {
        t_config *archivoConfig = abrirArchivoConfiguracion(nombreArchivo, logger);
        int cantidadDeBloques = config_get_int_value(archivoConfig, "BLOCKS");
        return cantidadDeBloques;
    }
    return -1;
}

void crearDirBloques(char *puntoMontaje) {
    char* nombreArchivo = crearDirEnPuntoDeMontaje(puntoMontaje, "/Bloques");
    int bloques = obtenerCantidadBloques(puntoMontaje);
    if(bloques != -1) {

        int tamanioBitarray = bloques / 8;
        char data[tamanioBitarray];

        for(int i = 0; i < tamanioBitarray; i++){
            data[i] = 0;
        }

        bitmap = bitarray_create_with_mode(data, sizeof(data), LSB_FIRST);

        int file = open("../mnt/LISSANDRA_FS/Metadata/Bitmap.bin", O_RDWR | O_CREAT | O_TRUNC);
        mmap(NULL, sizeof(bitmap), PROT_WRITE, MAP_SHARED, file, 0);
        munmap(bitmap, sizeof(bitmap));

        close(file);

        for(int i = 0; i < bloques; i++) {
            char * unArchivoDeBloque = string_duplicate(nombreArchivo);
            string_append(&unArchivoDeBloque, "/");
            string_append(&unArchivoDeBloque, string_from_format("%i", i));
            string_append(&unArchivoDeBloque, ".bin");
            if(!existeElArchivo(unArchivoDeBloque)) {
                FILE *file = fopen(unArchivoDeBloque, "w");
                fclose(file);
            }
            //printf("%i", bitarray_test_bit(bitmap, i));
        }
    }
}

void crearDirMetadata(char *puntoMontaje) {
    char* nombreArchivo = crearDirEnPuntoDeMontaje(puntoMontaje, "/Metadata");
    char *pathArchivoMetadata = string_new();
    string_append(&pathArchivoMetadata, nombreArchivo);
    string_append(&pathArchivoMetadata, "/Metadata.bin");
    char *pathArchivoBitmap = string_new();
    string_append(&pathArchivoBitmap, nombreArchivo);
    string_append(&pathArchivoBitmap, "/Bitmap.bin");
    if (!existeElArchivo(pathArchivoMetadata)) {
        FILE *f = fopen(pathArchivoMetadata, "w");
        fclose(f);
    }
    if (!existeElArchivo(pathArchivoBitmap)) {
        FILE *f = fopen(pathArchivoBitmap, "w");
        fclose(f);
    }
}

int crearDirectoriosBase(char *puntoMontaje) {
    crearDirMetadata(puntoMontaje);
    crearDirTables(puntoMontaje);
    crearDirBloques(puntoMontaje);
}

void mkdir_recursive(char *path) {
    char *subpath, *fullpath;

    fullpath = strdup(path);
    subpath = dirname(fullpath);
    if (strlen(subpath) > 1)
        mkdir_recursive(subpath);
    mkdir(path, 0777);
    free(fullpath);
}

void inicializarLFS(char* puntoMontaje){
    crearDirectoriosBase(puntoMontaje);
    // TODO: Si no existen los directorios tables y bloques (suponiendo que la metadata ya existe), los crea.
    // TODO: Si no existen los bloques, los crea.

}

void *atenderConexiones(void *parametrosThread) {
    parametros_thread_lfs *parametros = (parametros_thread_lfs *) parametrosThread;
    GestorConexiones *unaConexion = parametros->conexion;
    t_log *logger = parametros->logger;
    int *fdMemoria = parametros->fdMemoria;
    sem_t *memoriaConectada = parametros->memoriaConectada;

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
                            void *mensaje = (void *) malloc(pesoMensaje);
                            bytesRecibidos = recv(fdConectado, mensaje, pesoMensaje, MSG_DONTWAIT);
                            if (bytesRecibidos == -1 || bytesRecibidos < pesoMensaje)
                                log_warning(logger, "Hubo un error al recibir el mensaje proveniente del socket %i",
                                            fdConectado);
                            else if (bytesRecibidos == 0) {
                                // acá cada uno setea una maravillosa función que hace cada uno cuando se le desconecta alguien
                                // nombre_maravillosa_funcion();
                                desconectarCliente(fdConectado, unaConexion, logger);
                            } else {
                                // acá cada uno setea una maravillosa función que hace cada uno cuando le llega un nuevo mensaje
                                // nombre_maravillosa_funcion();
                                atenderMensajes(header, mensaje, parametrosThread);
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

                // hacemos handshake
                hacerHandshake(*fdNuevoCliente, KERNEL);

                // acá cada uno setea una maravillosa función que hace cada uno cuando se le conecta un nuevo cliente
                // nombre_maravillosa_funcion();
            }
        }
    }
}


int main(void) {
    logger = log_create("../lfs.log", "lfs", true, LOG_LEVEL_INFO);

    log_info(logger, "Iniciando proceso Lissandra File System");

    configuracion = cargarConfiguracion("../lfs.cfg", logger);
    metadatas = dictionary_create();
    inicializarLFS(configuracion.puntoMontaje);

    log_info(logger, "Puerto Escucha: %i", configuracion.puertoEscucha);
    log_info(logger, "Punto Montaje: %s", configuracion.puntoMontaje);
    log_info(logger, "Retardo: %i", configuracion.retardo);
    log_info(logger, "Tamaño value: %i", configuracion.tamanioValue);
    log_info(logger, "Tiempo dump: %i", configuracion.tiempoDump);

    GestorConexiones *misConexiones = inicializarConexion();

    levantarServidor(configuracion.puertoEscucha, misConexiones, logger);


    int fdMemoria = 0;

    sem_t memoriaConectada;

    sem_init(&memoriaConectada, 0, 0);

    pthread_t *hiloConexiones = crearHiloConexiones(misConexiones, &fdMemoria, &memoriaConectada, logger);

    parametros_consola *parametros = (parametros_consola *) malloc(sizeof(parametros_consola));

    parametros->logger = logger;
    parametros->unComponente = LISSANDRA;
    parametros->gestionarComando = gestionarRequest;

    ejecutarConsola(parametros);
    // crearHiloServidor(configuracion.puertoEscucha, &atenderMensajes, NULL, NULL);
    //int cliente = crearSocketCliente("192.168.0.30", 8000);

    while (1) {
        sem_wait(&memoriaConectada);
        if (fdMemoria > 0) {
            Header header;
            int bytesRecibidos = recv(fdMemoria, &header, sizeof(Header), MSG_WAITALL);
            if (bytesRecibidos == 0)
                fdMemoria = 0;
            else {
                header = deserializarHeader(&header);
                char *request = (char *) malloc(header.tamanioMensaje);
                bytesRecibidos = recv(fdMemoria, &request, header.tamanioMensaje, MSG_WAITALL);
                printf("Request recibida: %s\n", request);
                fflush(stdout);
            }
        }
    }

    return 0;
}