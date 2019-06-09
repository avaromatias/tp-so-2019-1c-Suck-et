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
        valorSinComillas(puntoMontaje);
        if (!string_ends_with(puntoMontaje, "/")) {
            string_append(&puntoMontaje, "/");
        }
        configuracion.puntoMontaje = concat(1, puntoMontaje);
        configuracion.retardo = config_get_int_value(archivoConfig, "RETARDO");
        configuracion.tamanioValue = config_get_int_value(archivoConfig, "TAMAÑO_VALUE");
        configuracion.tiempoDump = config_get_int_value(archivoConfig, "TIEMPO_DUMP");

        return configuracion;
    }
}

void atenderMensajes(Header header, char *mensaje, parametros_thread_lfs *parametros) {
    char **comandoParseado = parser(mensaje);
    t_response *retorno;
    t_comando comando = instanciarComando(comandoParseado);
    retorno = gestionarRequest(comando);
    enviarPaquete(header.fdRemitente, retorno->tipoRespuesta, retorno->valor);
    free(retorno);
    free(comandoParseado);

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
    string_append(&unArchivoDeBloque, configuracion.puntoMontaje);
    if (!string_ends_with(configuracion.puntoMontaje, "/")) {
        string_append(&unArchivoDeBloque, "/");
    }
    string_append(&unArchivoDeBloque, "Bloques/");
    string_append(&unArchivoDeBloque, string_from_format("%i", numberoBloque));
    string_append(&unArchivoDeBloque, ".bin");
    return unArchivoDeBloque;
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
    char *linea = string_new();
    linea = concat(9, "CONSISTENCY=", tipoConsistencia, "\n", "PARTITIONS=", particiones, "\n", "COMPACTION_TIME=",
                   tiempoCompactacion, "\n");
    fwrite(linea, sizeof(char) * (strlen(linea) + 1), 1, f);
    free(linea);
    fclose(f);
}

void crearBinarios(char *nombreTabla, int particiones) {
    for (int i = 0; i < particiones; i++) {
        int bloque = obtenerBloqueDisponible(nombreTabla, i);
        if (bloque != -1) {
            t_bloqueAsignado *bloqueA = (t_bloqueAsignado *) malloc(sizeof(t_bloqueAsignado));
            bloqueA->tabla = concat(1, nombreTabla);
            bloqueA->particion = i;
            dictionary_put(bloquesAsignados, (char *) string_from_format("%i", bloque), bloqueA);
            char *nombreArchivo = string_new();
            string_append(&nombreArchivo, string_from_format("%i", i));
            string_append(&nombreArchivo, ".bin");
            FILE *file = fopen(obtenerPathArchivo(nombreTabla, nombreArchivo), "w");
            int tamanio = obtenerTamanioBloque(bloque);
            char *contenido = generarContenidoParaParticion(string_from_format("%i", tamanio),
                                                            concat(3, "[", string_from_format("%i", bloque), "]"));
            fwrite(contenido, sizeof(char) * strlen(contenido), 1, file);
            free(contenido);
            fclose(file);
        }
    }
}

int obtenerTamanioBloque(int bloque) {
    char *pathBloque = obtenerPathBloque(bloque);
    if (archivoVacio(pathBloque)) {
        return 0;
    } else {
        FILE *bloque = fopen(pathBloque, "r");
        char ch;
        int count = 0;
        while ((ch = fgetc(bloque)) != EOF) {
            count++;
        }
        fclose(bloque);
        return count;
    }
}

int estaLibreElBloque(int bloque) {
    if (bitarray_test_bit(bitmap, bloque) == 0) {
        return 1;
    }
    return 0;
}

int estaDisponibleElBloqueParaTabla(int i, char *nombreTabla, int particion) {
    int bloqueDisponible = 0;
    int bloqueLibre = estaLibreElBloque(i) == 1;
    if (bloquesAsignados->elements_amount > 0) {
        t_bloqueAsignado *bloque = dictionary_get(bloquesAsignados, (char *) string_from_format("%i", i));
        if (strcmp(bloque->tabla, "") == 0 ||
            (strcmp(bloque->tabla, nombreTabla) == 0 && bloque->particion == particion)) {
            bloqueDisponible = 1;
        }
    }

    return bloqueDisponible && bloqueLibre;
}

int obtenerBloqueDisponible(char *nombreTabla, int particion) {
    for (int i = 0; i < obtenerCantidadBloques(configuracion.puntoMontaje); i++) {
        if (estaDisponibleElBloqueParaTabla(i, nombreTabla, particion)) {
            return i;
        }
    }
    return -1;
}

t_response *lfsCreate(char *nombreTabla, char *tipoConsistencia, char *particiones, char *tiempoCompactacion) {
    t_response *retorno = (t_response *) malloc(sizeof(t_response));
    if (validarConsistencia(tipoConsistencia) != 0) {
        retorno->tipoRespuesta = ERR;
        retorno->valor = concat(1, "El Tipo de Consistencia no es valido. Este puede ser SC, SHC o EC.");
    } else {
        char *tablePath = obtenerPathTabla(nombreTabla, configuracion.puntoMontaje);
        // Verificar que la tabla no exista en el file system.
        if (existeElArchivo(tablePath)) {
            char *path = obtenerPathMetadata(nombreTabla, configuracion.puntoMontaje);
            // En caso que exista, se guardará el resultado en un archivo .log y se retorna un error indicando dicho resultado.
            if (!existeElArchivo(path)) {
                crearMetadata(nombreTabla, tipoConsistencia, particiones, tiempoCompactacion);
                retorno->tipoRespuesta = ERR;
                retorno->valor = concat(3, "La tabla ", nombreTabla, " ya existe. Se creo su Metadata.\n");
            } else {
                retorno->tipoRespuesta = ERR;
                retorno->valor = concat(3, "La tabla ", nombreTabla, " ya existe.\n");
            }
        } else {
            // Crear el directorio para dicha tabla.
            mkdir(tablePath, 0777);
            // Crear el directorio para dicha tabla.
            // Grabar en dicho archivo los parámetros pasados por el request.
            crearMetadata(nombreTabla, tipoConsistencia, particiones, tiempoCompactacion);
            // Crear los archivos binarios asociados a cada partición de la tabla y
            // asignar a cada uno un bloque
            crearBinarios(nombreTabla, atoi(particiones));
            retorno->tipoRespuesta = RESPUESTA;
            retorno->valor = concat(3, "La tabla ", nombreTabla, " fue creada con exito.\n");
        }
        return retorno;
    }
}

t_response *lfsInsert(char *nombreTabla, char *key, char *valor, time_t timestamp) {
    t_response *retorno = (t_response *) malloc(sizeof(t_response));
    if (validarValor(valor) != 0) {
        retorno->tipoRespuesta = ERR;
        retorno->valor = concat(1, "El valor debe estar enmascarado con \"\"\n");
        return retorno;
    } else {
        // Verificar que la tabla exista en el file system. En caso que no exista, informa el error y continúa su ejecución.
        if (existeTabla(nombreTabla) == 0) {
            // Obtener la metadata asociada a dicha tabla.
            char *path = obtenerPathMetadata(nombreTabla, configuracion.puntoMontaje);
            if (existeElArchivo(path)) {
                // printf("Existe metadata en %s\n", path);
                // TODO: Verificar si existe en memoria una lista de datos a dumpear. De no existir, alocar dicha memoria.
                char *linea = armarLinea(key, valor, timestamp);
                obtenerMetadata(nombreTabla);
                t_metadata *meta = dictionary_get(metadatas, nombreTabla);
                int particion = calcularParticion(key, meta);
                char *nombreArchivo = string_new();
                char *p = string_itoa(particion);
                nombreArchivo = concat(4, obtenerPathTabla(nombreTabla, configuracion.puntoMontaje), "/", p, ".bin");
                int bloque = obtenerBloqueDisponible(nombreTabla, particion);
                t_bloqueAsignado *bloqueA = (t_bloqueAsignado *) malloc(sizeof(t_bloqueAsignado));
                bloqueA->tabla = concat(1, nombreTabla);
                bloqueA->particion = particion;
                dictionary_put(bloquesAsignados, (char *) string_from_format("%i", bloque), bloqueA);
                FILE *f = fopen(obtenerPathBloque(bloque), "a");
                printf("Linea %s", linea);
                // TODO: Insertar en la memoria temporal del punto anterior una nueva entrada que contenga los datos enviados en la request.
                fwrite(linea, sizeof(char) * strlen(linea), 1, f);
                fclose(f);
                if (obtenerTamanioBloque(bloque) >= obtenerTamanioBloques(configuracion.puntoMontaje))
                    bitarray_set_bit(bitmap, bloque);
                free(path);
                t_config *archivoConfig = abrirArchivoConfiguracion(nombreArchivo, logger);
                char **blocks = string_get_string_as_array(config_get_string_value(archivoConfig, "BLOCKS"));
                if (!arrayIncluye(blocks, string_from_format("%i", bloque))) {
                    int tam = tamanioDeArrayDeStrings(blocks);
                    blocks[tam] = string_from_format("%i", bloque);
                    blocks[tam + 1] = NULL;
                }
                char *bloques = convertirArrayAString(blocks);
                int size = config_get_int_value(archivoConfig, "SIZE") + strlen(linea);
                FILE *fParticion = fopen(nombreArchivo, "r+");
                char *contenido = generarContenidoParaParticion(string_from_format("%i", size), bloques);
                fwrite(contenido, sizeof(char) * strlen(contenido), 1, fParticion);
                fclose(fParticion);
                free(contenido);
                retorno->tipoRespuesta = RESPUESTA;
                retorno->valor = concat(1, "Se inserto el valor con exito.\n");

            } else {
                retorno->tipoRespuesta = ERR;
                retorno->valor = concat(5, "No se pudo insertar en ", nombreTabla, ". No existe metadata en ", path,
                                        ".\n");
            }
        } else {
            retorno->tipoRespuesta = ERR;
            retorno->valor = concat(3, "No existe la tabla ", nombreTabla, ".\n");
        }
        return retorno;

    }
}


char *generarContenidoParaParticion(char *tamanio, char *bloques) {
    char *contenido = string_new();
    string_append(&contenido, "SIZE=");
    string_append(&contenido, tamanio);
    string_append(&contenido, "\n");
    string_append(&contenido, "BLOCKS=");
    string_append(&contenido, bloques);
    string_append(&contenido, "\n");
    return contenido;
}

t_response *lfsSelect(char *nombreTabla, char *key) {
    t_response *retorno = (t_response *) malloc(sizeof(t_response));
    //1. Verificar que la tabla exista en el File System
    if (existeTabla(nombreTabla) == 0) {

        //2. Obtener la metadata asociada a dicha tabla
        obtenerMetadata(nombreTabla);
        t_metadata *meta = dictionary_get(metadatas, nombreTabla);

        //3. Calcular cual es la partición que contiene dicho KEY
        int particion = calcularParticion(key, meta);

        //4. Escanear la partición objetivo, todos los archivos temporales y la memoria temporal de dicha tabla (si existe) buscando la key deseada

        char *nombreArchivoParticion = obtenerNombreArchivoParticion(particion);
        char *binaryPath = obtenerPathArchivo(nombreTabla, nombreArchivoParticion);
        FILE *binarioParticion = fopen(binaryPath, "r");
        FILE *binarioBloque;

        char seek;
        char **palabras;
        char *linea;
        char str[2];
        str[1] = '\0';
        char *keyEncontrado;
        char *timestampEncontrado;
        int mayorTimestamp = 0;
        char *valorMayorTimestamp = string_new();
        char *mayorLinea = string_new();
        mayorLinea = concat(1, "");
        bool lineaContinuaEnOtroBloque = false;

        //4.0 Obtengo los bloques asignados a la particion obtenida
        int tamanioArray = tamanioDeArrayDeStrings(bloquesEnParticion(nombreTabla, nombreArchivoParticion));
        char **bloques = bloquesEnParticion(nombreTabla, nombreArchivoParticion);
        for (int i = 0; i < tamanioArray; i++) {

            //4.1. Escaneo particion objetivo
            char *blockPath = obtenerPathBloque(atoi(bloques[i]));
            binarioBloque = fopen(blockPath, "r");

            while (!feof(binarioBloque)) {
                if (!lineaContinuaEnOtroBloque) {
                    linea = string_new();
                }
                keyEncontrado = string_new();
                timestampEncontrado = string_new();
                while ((seek = getc(binarioBloque)) != EOF && seek != '\n') {
                    str[0] = seek;
                    string_append(&linea, str);
                }
                if (strcmp(linea, "") != 0) {
                    palabras = desarmarLinea(linea);
                    if (tamanioDeArrayDeStrings(palabras) == 3 &&
                        seek == '\n') { // Si la línea no continua en otro bloque
                        lineaContinuaEnOtroBloque = false;
                        string_append(&timestampEncontrado, palabras[0]);
                        string_append(&keyEncontrado, palabras[1]);
                        if (strcmp(keyEncontrado, key) == 0 && (atoi(timestampEncontrado) > mayorTimestamp)) {
                            mayorTimestamp = atoi(timestampEncontrado);
                            valorMayorTimestamp = string_new();
                            valorMayorTimestamp = concat(1, palabras[2]);
                            mayorLinea = string_new();
                            mayorLinea = concat(1, linea);
                        }
                    } else {
                        lineaContinuaEnOtroBloque = true;
                    }
                }
            }
        }

        fclose(binarioBloque);
        fclose(binarioParticion);

        //4.2. Escaneo los archivos temporales


        //4.2. Escaneo la memoria temporal de la tabla


        //5. Encontradas las entradas para dicha Key, se retorna el valor con el Timestamp más grande
        if (strcmp(valorMayorTimestamp, "") != 0) {
            retorno->tipoRespuesta = RESPUESTA;
            retorno->valor = concat(1, mayorLinea);
            return retorno;
        } else {
            retorno->tipoRespuesta = ERR;
            retorno->valor = concat(1, "No se encontro ningun valor con esa key.\n");
            free(mayorLinea);
            return retorno;
        }
    } else {
        retorno->tipoRespuesta = ERR;
        retorno->valor = concat(3, "No existe la tabla ", nombreTabla, ".\n");
        return retorno;
    }
}

char *obtenerNombreArchivoParticion(int particion) {
    char *nombreArchivo = string_new();
    string_append(&nombreArchivo, string_itoa(particion));
    string_append(&nombreArchivo, ".bin");
    return nombreArchivo;
}

char **bloquesEnParticion(char *nombreTabla, char *nombreArchivo) {
    char *path = obtenerPathArchivo(nombreTabla, nombreArchivo);
    char **arrayDeBloques;
    if (existeElArchivo(path)) {
        t_config *archivoConfig = abrirArchivoConfiguracion(path, logger);
        arrayDeBloques = string_get_string_as_array(config_get_string_value(archivoConfig, "BLOCKS"));

    } else {
        return NULL;
    }
    free(path);
    return arrayDeBloques;
}

void ejecutarConsola() {
    t_comando comando;

    do {
        if (bloquesAsignados->table_current_size) {
            for (int i = 0; i < 16; i++) {
                printf("%d", bitarray_test_bit(bitmap, i));
            }
        }

        char *leido = readline("Lissandra@suck-ets:~$ ");
        char **comandoParseado = parser(leido);
        if (comandoParseado == NULL) {
            free(comandoParseado);
            continue;
        }
        comando = instanciarComando(comandoParseado);
        free(leido);
        free(comandoParseado);
        if (validarComandosComunes(comando, logger)) {
            t_response *retorno = gestionarRequest(comando);
            if(!string_ends_with(retorno->valor,"\n")){
                string_append(&retorno->valor,"\n");
            }
            printf("%s", retorno->valor);
            free(retorno);
            log_info(logger, "Request procesada correctamente.");
        }

    } while (comando.tipoRequest != EXIT);
    printf("Ya se analizo todo lo solicitado.\n");
}

t_response *gestionarRequest(t_comando comando) {

    t_response *retorno;
    switch (comando.tipoRequest) {
        case SELECT:
//            printf("Tabla: %s\n", comando.parametros[0]);
//            printf("Key: %s\n", comando.parametros[1]);
            retorno = lfsSelect(comando.parametros[0], comando.parametros[1]);
            return retorno;

        case INSERT:;
//            printf("Tabla: %s\n", comando.parametros[0]);
//            printf("Key: %s\n", comando.parametros[1]);
//            printf("Valor: %s\n", comando.parametros[2]);
            time_t timestamp;
            // El parámetro Timestamp es opcional.
            // En caso que un request no lo provea (por ejemplo insertando un valor desde la consola),
            // se usará el valor actual del Epoch UNIX.
            if (comando.cantidadParametros == 4 && comando.parametros[3] != NULL) {
                timestamp = (time_t) strtol(comando.parametros[3], NULL, 10);
            } else {
                timestamp = (time_t) time(NULL);
            }
            printf("Timestamp: %i\n", (int) timestamp);
            retorno = lfsInsert(comando.parametros[0], comando.parametros[1], comando.parametros[2], timestamp);
            return retorno;

        case CREATE:;
//            printf("Tabla: %s\n", comando.parametros[0]);
//            printf("TIpo de consistencia: %s\n", comando.parametros[1]);
//            printf("Numero de particiones: %s\n", comando.parametros[2]);
//            printf("Tiempo de compactacion: %s\n", comando.parametros[3]);
            retorno = lfsCreate(comando.parametros[0], comando.parametros[1], comando.parametros[2],
                                comando.parametros[3]);
            return retorno;

        case DESCRIBE:
            if (comando.parametros[0] == NULL) {
                // Hacer describe global
            } else {
//                printf("Tabla: %s\n", comando.parametros[0]);
                // Hacer describe de una tabla especifica
            }

            retorno = (t_response *) malloc(sizeof(t_response));
            retorno->valor = concat(1, "Describe");
            retorno->tipoRespuesta = RESPUESTA;
            return retorno;

        case DROP:
            retorno = (t_response *) malloc(sizeof(t_response));
            retorno->valor = concat(1, "Drop");
            retorno->tipoRespuesta = RESPUESTA;
            return retorno;

        case HELP:
            printf("************ Comandos disponibles ************\n");
            printf("- SELECT [NOMBRE_TABLA] [KEY]\n");
            printf("- INSERT [NOMBRE_TABLA] [KEY] “[VALUE]” [Timestamp](Opcional)\n");
            printf("- CREATE [NOMBRE_TABLA] [TIPO_CONSISTENCIA] [NUMERO_PARTICIONES] [COMPACTION_TIME]\n");
            printf("- DESCRIBE [NOMBRE_TABLA](Opcional)\n");
            printf("- DROP [NOMBRE_TABLA]\n");
            printf("- EXIT\n");
            retorno = (t_response *) malloc(sizeof(t_response));
            retorno->valor = concat(1, "Help");
            retorno->tipoRespuesta = RESPUESTA;
            return retorno;

        case EXIT:
            retorno = (t_response *) malloc(sizeof(t_response));
            retorno->valor = concat(1, "Exit");
            retorno->tipoRespuesta = RESPUESTA;
            return retorno;

        default:
            retorno = (t_response *) malloc(sizeof(t_response));
            retorno->valor = concat(1, "Ingrese un comando valido.\n");
            retorno->tipoRespuesta = RESPUESTA;
            return retorno;
    }

}

int existeTabla(char *tabla) {
    char *tablePath = obtenerPathTabla(tabla, configuracion.puntoMontaje);
    if (!existeElArchivo(tablePath)) {
        log_error(logger, "No se encontro o no tiene permisos para acceder a la tabla %s.", tabla);
        free(tablePath);
        return -1;
    }
    free(tablePath);
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
    free(metadataPath);
    free(metadata);
    config_destroy(config);
}

int calcularParticion(char *key, t_metadata *metadata) {
    int k = atoi(key);
    int b = metadata->partitions;
    return k % b;
}

pthread_t *crearHiloConexiones(GestorConexiones *unaConexion, int tamanioValue, t_log *logger) {
    pthread_t *hiloConexiones = malloc(sizeof(pthread_t));

    parametros_thread_lfs *parametros = (parametros_thread_lfs *) malloc(sizeof(parametros_thread_lfs));

    parametros->conexion = unaConexion;
    parametros->logger = logger;
    parametros->tamanioValue = tamanioValue;

    pthread_create(hiloConexiones, NULL, &atenderConexiones, parametros);

    return hiloConexiones;
}

char *crearDirEnPuntoDeMontaje(char *puntoMontaje, char *nombreDir) {
    char *nombreArchivo = concat(2, puntoMontaje, nombreDir);

    if (!existeElArchivo(nombreArchivo)) {
        mkdir_recursive(nombreArchivo);
    }

    return nombreArchivo;
}

void crearDirTables(char *puntoMontaje) {
    char *nombreArchivo = string_new();
    string_append(&nombreArchivo, puntoMontaje);
    string_append(&nombreArchivo, "Tables");

    if (!existeElArchivo(nombreArchivo)) {
        mkdir_recursive(nombreArchivo);
    } else {
        cargarBloquesAsignados(nombreArchivo);
    }
    free(nombreArchivo);
}

void cargarBloquesAsignados(char *path) {
    DIR *d;
    struct dirent *dir;
    d = opendir(path);
    if (d) {
        while ((dir = readdir(d)) != NULL) {
            if (strcmp(dir->d_name, ".") != 0 && strcmp(dir->d_name, "..") != 0) {
                char *nombreTabla = dir->d_name;
                obtenerMetadata(nombreTabla);
                t_metadata *meta = dictionary_get(metadatas, nombreTabla);
                int partitions = meta->partitions;
                for (int i = 0; i < partitions; i++) {
                    char *nombreArchivo = string_new();
                    string_append(&nombreArchivo, string_from_format("%i", i));
                    string_append(&nombreArchivo, ".bin");
                    char **arrayDeBloques = bloquesEnParticion(nombreTabla, nombreArchivo);
                    for (int j = 0; j < tamanioDeArrayDeStrings(arrayDeBloques); j++) {
                        t_bloqueAsignado *bloque = (t_bloqueAsignado *) malloc(sizeof(t_bloqueAsignado));
                        bloque->tabla = concat(1, nombreTabla);
                        bloque->particion = i;
                        dictionary_put(bloquesAsignados, arrayDeBloques[j], bloque);
                    }
                    free(nombreArchivo);
                }
            }
        }
        closedir(d);
    }
}

int obtenerCantidadBloques(char *puntoMontaje) {
    char *nombreArchivo = string_new();
    string_append(&nombreArchivo, puntoMontaje);
    string_append(&nombreArchivo, "Metadata/Metadata.bin");
    if (existeElArchivo(nombreArchivo) && !archivoVacio(nombreArchivo)) {
        t_config *archivoConfig = abrirArchivoConfiguracion(nombreArchivo, logger);
        int cantidadDeBloques = config_get_int_value(archivoConfig, "BLOCKS");
        free(nombreArchivo);
        config_destroy(archivoConfig);
        return cantidadDeBloques;
    }
    free(nombreArchivo);
    return -1;
}

int obtenerTamanioBloques(char *puntoMontaje) {
    char *nombreArchivo = string_new();
    string_append(&nombreArchivo, puntoMontaje);
    string_append(&nombreArchivo, "Metadata/Metadata.bin");
    if (existeElArchivo(nombreArchivo) && !archivoVacio(nombreArchivo)) {
        t_config *archivoConfig = abrirArchivoConfiguracion(nombreArchivo, logger);
        int cantidadDeBloques = config_get_int_value(archivoConfig, "BLOCK_SIZE");
        free(nombreArchivo);
        config_destroy(archivoConfig);
        return cantidadDeBloques;
    }
    free(nombreArchivo);
    return -1;
}


void crearDirBloques(char *puntoMontaje) {
    char *nombreArchivo = crearDirEnPuntoDeMontaje(puntoMontaje, "Bloques");
    int bloques = obtenerCantidadBloques(puntoMontaje);
    if (bloques != -1) {

        int tamanioBitarray = bloques / 8;
        char *data = malloc(sizeof(char) * tamanioBitarray);

        for (int i = 0; i < tamanioBitarray; i++) {
            data[i] = 0;
        }

        char *archivoBitmap = string_new();
        string_append(&archivoBitmap, puntoMontaje);
        string_append(&archivoBitmap, "Metadata/Bitmap.bin");
        //int file = open(archivoBitmap, O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);

        /*char *data = (char *) mmap(NULL, sizeof(data), PROT_WRITE | PROT_READ, MAP_SHARED, file, 0);
        if (data == MAP_FAILED) {                                           //
            printf("Error al mapear a memoria: %s\n", strerror(errno));     // Hay que borrar esto una vez
            close(file);                                                    // que quede andando el bitmap
        }                                                                   //*/
        bitmap = bitarray_create_with_mode(data, sizeof(data), MSB_FIRST);

        free(archivoBitmap);

        for (int i = 0; i < bloques; i++) {
            char *unArchivoDeBloque = concat(4, nombreArchivo, "/", string_from_format("%i", i), ".bin");
            t_bloqueAsignado *bloque = (t_bloqueAsignado *) malloc(sizeof(t_bloqueAsignado));
            bloque->tabla = concat(1, "");
            bloque->particion = NULL;
            dictionary_put(bloquesAsignados, (char *) string_from_format("%i", i), bloque);
            if (!existeElArchivo(unArchivoDeBloque)) {
                FILE *file = fopen(unArchivoDeBloque, "w");
                fclose(file);
            } else if (!archivoVacio(unArchivoDeBloque) &&
                       obtenerTamanioBloque(i) >= obtenerTamanioBloques(configuracion.puntoMontaje)) {
                bitarray_set_bit(bitmap, i);
            }
            free(unArchivoDeBloque);
            //printf("%i", bitarray_test_bit(bitmap, i));
        }
        free(nombreArchivo);
    }
}

void crearDirMetadata(char *puntoMontaje) {
    char *nombreArchivo = crearDirEnPuntoDeMontaje(puntoMontaje, "Metadata");
    char *pathArchivoMetadata = string_new();
    pathArchivoMetadata = concat(2, nombreArchivo, "/Metadata.bin");
    char *pathArchivoBitmap = string_new();
    pathArchivoBitmap = concat(2, nombreArchivo, "/Bitmap.bin");
    if (!existeElArchivo(pathArchivoMetadata)) {
        FILE *fm = fopen(pathArchivoMetadata, "w");
        fclose(fm);
    }
    if (!existeElArchivo(pathArchivoBitmap)) {
        FILE *fb = fopen(pathArchivoBitmap, "w");
        fclose(fb);
    }
    free(nombreArchivo);
    free(pathArchivoMetadata);
    free(pathArchivoBitmap);
}

int crearDirectoriosBase(char *puntoMontaje) {
    crearDirMetadata(puntoMontaje);
    crearDirBloques(puntoMontaje);
    crearDirTables(puntoMontaje);
}


void inicializarLFS(char *puntoMontaje) {
    crearDirectoriosBase(puntoMontaje);
}

void atenderHandshake(Header header, Componente componente, parametros_thread_lfs *parametros) {
    if (componente == MEMORIA) {
        char *tamanioValue = string_itoa(parametros->tamanioValue);
        enviarPaquete(header.fdRemitente, HANDSHAKE, tamanioValue);
        free(tamanioValue);
    }
}

void *atenderConexiones(void *parametrosThread) {
    parametros_thread_lfs *parametros = (parametros_thread_lfs *) parametrosThread;
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
                                if (header.tipoMensaje == HANDSHAKE) {
                                    Componente componente = *((Componente *) mensaje);
                                    atenderHandshake(header, componente, parametros);
                                } else
                                    atenderMensajes(header, mensaje, parametrosThread);
                            }
                            free(mensaje);
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


int main(void) {
    logger = log_create("../lfs.log", "lfs", true, LOG_LEVEL_INFO);

    log_info(logger, "Iniciando proceso Lissandra File System");

    configuracion = cargarConfiguracion("../lfs.cfg", logger);
    metadatas = dictionary_create();
    bloquesAsignados = dictionary_create();
    inicializarLFS(configuracion.puntoMontaje);

    log_info(logger, "Puerto Escucha: %i", configuracion.puertoEscucha);
    log_info(logger, "Punto Montaje: %s", configuracion.puntoMontaje);
    log_info(logger, "Retardo: %i", configuracion.retardo);
    log_info(logger, "Tamaño value: %i", configuracion.tamanioValue);
    log_info(logger, "Tiempo dump: %i", configuracion.tiempoDump);

    GestorConexiones *misConexiones = inicializarConexion();

    levantarServidor(configuracion.puertoEscucha, misConexiones, logger);

    pthread_t *hiloConexiones = crearHiloConexiones(misConexiones, configuracion.tamanioValue, logger);

    ejecutarConsola();

    pthread_join(&hiloConexiones, NULL);

    return 0;
}