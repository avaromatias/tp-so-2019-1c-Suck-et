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
            if (!config_has_property(archivoConfig, clavesObligatorias[i])) {
                return false;
            }
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
            configuracion.puntoMontaje = concat(2, puntoMontaje, "/");
        } else {
            configuracion.puntoMontaje = concat(1, puntoMontaje);
        }
        configuracion.retardo = config_get_int_value(archivoConfig, "RETARDO");
        configuracion.tamanioValue = config_get_int_value(archivoConfig, "TAMAÑO_VALUE");
        configuracion.tiempoDump = config_get_int_value(archivoConfig, "TIEMPO_DUMP");
        config_destroy(archivoConfig);
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
    char *tablePath = obtenerPathTabla(nombreTabla, configuracion.puntoMontaje);
    char *path = string_duplicate(tablePath);
    free(tablePath);
    string_append(&path, "/");
    string_append(&path, nombreArchivo);
    return path;
}

char *obtenerPathBloque(int numberoBloque) {
    char *unArchivoDeBloque = concat(1, configuracion.puntoMontaje);
    if (!string_ends_with(configuracion.puntoMontaje, "/")) {
        unArchivoDeBloque = concat(2, unArchivoDeBloque, "/");
    }
    char *nroDeBloque = string_from_format("%i", numberoBloque);
    unArchivoDeBloque = concat(4, unArchivoDeBloque, "Bloques/", nroDeBloque, ".bin");
    free(nroDeBloque);
    return unArchivoDeBloque;
}


int validarConsistencia(char *tipoConsistencia) {
    if (strcmp(tipoConsistencia, "SC") == 0 || strcmp(tipoConsistencia, "SHC") == 0 ||
        strcmp(tipoConsistencia, "EC") == 0) {
        return 0;
    }
    return -1;
}

int validarTamanioValor(char *valor) {
    char *copiaValor = string_duplicate(valor);
    valorSinComillas(copiaValor);
    if (strlen(copiaValor) <= configuracion.tamanioValue) {
        free(copiaValor);
        return 0;
    }
    free(copiaValor);
    return -1;
}

int validarComillasValor(char *valor) {
    if (string_starts_with(valor, "\"") && string_ends_with(valor, "\"")) {
        return 0;
    }
    return -1;
}

void crearMetadata(char *nombreTabla, char *tipoConsistencia, char *particiones, char *tiempoCompactacion) {
    sem_t *semMetadata = obtenerSemaforoPath(obtenerPathArchivo(nombreTabla, "Metadata"));
    sem_wait(semMetadata);
    FILE *f = fopen(obtenerPathArchivo(nombreTabla, "Metadata"), "w");
    char *linea = concat(9, "CONSISTENCY=", tipoConsistencia, "\n", "PARTITIONS=", particiones, "\n",
                         "COMPACTION_TIME=",
                         tiempoCompactacion, "\n");
    fwrite(linea, sizeof(char) * (strlen(linea) + 1), 1, f);
    free(linea);
    fclose(f);
    sem_post(semMetadata);
}

void crearBinarios(char *nombreTabla, int particiones) {
    pthread_mutex_lock(&mutexAsignacionBloques);
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
    pthread_mutex_unlock(&mutexAsignacionBloques);
}

int obtenerTamanioBloque(int bloque) {
    char *pathBloque = obtenerPathBloque(bloque);
    if (archivoVacio(pathBloque)) {
        free(pathBloque);
        return 0;
    } else {
        FILE *bloque = fopen(pathBloque, "r");
        char ch;
        int count = 0;
        while ((ch = fgetc(bloque)) != EOF) {
            count++;
        }
        fclose(bloque);
        free(pathBloque);
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


void lfsDump() {
    t_config *archivoConfig = abrirArchivoConfiguracion("../lfs.cfg", logger);
    int tiempoDump=config_get_int_value(archivoConfig,"TIEMPO_DUMP");
    if(!tiempoDump){
        log_error(logger,"El TIEMPO_DUMP no esta seteado en el Archivo de Configuracion.");
    }
    tiempoDump=tiempoDump/1000;
    sleep(tiempoDump);
    void dumpTabla(char *nombreTabla, t_dictionary *tablaDeKeys) {
        int nroDump = 0;
        char* nombreArchivo = obtenerPathArchivo(nombreTabla, string_from_format("%s%i%s", nombreTabla, nroDump, ".tmp"));
        while (existeElArchivo(nombreArchivo)) {
            nroDump++;
            nombreArchivo = obtenerPathArchivo(nombreTabla, string_from_format("%s%i%s", nombreTabla, nroDump, ".tmp"));
        }
        void _dumpKey(char *key, t_list *listaDeRegistros) {
            void _dumpRegistro(char* linea) {
                FILE *archivoDump = fopen(nombreArchivo, "a");
                fwrite(linea,1,strlen(linea),archivoDump);
                fclose(archivoDump);
            }
            list_iterate(listaDeRegistros, _dumpRegistro);

        }
        dictionary_iterator(tablaDeKeys, _dumpKey);
    }
    if(!dictionary_is_empty(memTable)){
        dictionary_iterator(memTable, dumpTabla);
    }
    dictionary_clean(memTable);
    lfsDump();
}


t_response *lfsDescribeAll() {
    t_response *retorno = (t_response *) malloc(sizeof(t_response));
    char **tablas = obtenerTablas();
    char *respuesta = string_new();
    int cantidadDeTablas = tamanioDeArrayDeStrings(tablas);
    if (cantidadDeTablas == 0) {
        retorno->tipoRespuesta = ERR;
        retorno->valor = concat(1, "No hay tablas.");
    } else {
        for (int i = 0; i < cantidadDeTablas; i++) {
            char *nombreTabla = string_duplicate(tablas[i]);
            t_response *retornoTabla = lfsDescribe(nombreTabla);
            char *respuestaTabla = string_new();
            respuestaTabla = concat(4, "-----------------\nTABLE=", string_duplicate(nombreTabla), "\n",
                                    string_duplicate(retornoTabla->valor));
            string_append(&respuesta, respuestaTabla);
            free(retornoTabla->valor);
            free(retornoTabla);
        }
        retorno->tipoRespuesta = RESPUESTA;
        retorno->valor = string_duplicate(respuesta);
        free(respuesta);
    }
    free(tablas);
    return retorno;
}

t_response *lfsDescribe(char *nombreTabla) {
    t_response *retorno = (t_response *) malloc(sizeof(t_response));
    if (existeTabla(nombreTabla) == 0) {
        char *pathMetadata = obtenerPathArchivo(nombreTabla, "Metadata");
        sem_t *semMetadata = obtenerSemaforoPath(pathMetadata);
        sem_wait(semMetadata);
        FILE *metadataTabla = fopen(pathMetadata, "r");
        if (metadataTabla != NULL) {
            fseek(metadataTabla, 0, SEEK_END);
            long fsize = ftell(metadataTabla);
            fseek(metadataTabla, 0, SEEK_SET);  /* same as rewind(f); */
            char *string = malloc(fsize + 1);
            fread(string, 1, fsize, metadataTabla);
            fseek(metadataTabla, 0, SEEK_SET);
            fclose(metadataTabla);
            string[fsize] = 0;
            free(pathMetadata);
            retorno->tipoRespuesta = RESPUESTA;
            retorno->valor = string;

        } else {
            retorno->tipoRespuesta = ERR;
            retorno->valor = concat(3, "No se encontro la Metadata de la tabla ", nombreTabla, ".");
        }

        sem_post(semMetadata);
    } else {
        retorno->tipoRespuesta = ERR;
        retorno->valor = concat(3, "La tabla ", nombreTabla, " no existe.");
    }

    return retorno;
}

int deleteFile(char *nombreArchivo) {
    int status;
    status = remove(nombreArchivo);
    return status;
}


int borrarContenidoDeDirectorio(char *dirPath) {
    DIR *d;
    struct dirent *dir;
    sem_t *semDir = obtenerSemaforoPath(dirPath);
    sem_wait(dirPath);
    d = opendir(dirPath);
    if (d) {
        while ((dir = readdir(d)) != NULL) {
            char *nombreTabla = string_new();
            nombreTabla = string_duplicate(dir->d_name);
            char *pathArchivo = string_new();
            string_append(&pathArchivo, dirPath);
            string_append(&pathArchivo, "/");
            string_append(&pathArchivo, nombreTabla);
            if (strcmp(dir->d_name, ".") != 0 && strcmp(dir->d_name, "..") != 0) {
                if (strcmp(dir->d_name, "Metadata") != 0) {
                    t_config *archivoConfig = abrirArchivoConfiguracion(pathArchivo, logger);
                    char **blocks = config_get_array_value(archivoConfig, "BLOCKS");
                    for (int i = 0; i < tamanioDeArrayDeStrings(blocks); i++) {
                        vaciarArchivo(obtenerPathBloque(atoi(blocks[i])));
                        t_bloqueAsignado *bloque = (t_bloqueAsignado *) malloc(sizeof(t_bloqueAsignado));
                        bloque->tabla = concat(1, "");
                        bloque->particion = NULL;
                        dictionary_put(bloquesAsignados, string_duplicate(blocks[i]), bloque);
                        free(blocks[i]);
                    }
                    free(blocks);
                    config_destroy(archivoConfig);
                }

            }
            if (deleteFile(pathArchivo) != 0) {
                return 0;
            }
            free(pathArchivo);
            free(nombreTabla);
        }
        closedir(d);
    }
    sem_post(semDir);
    free(dirPath);
    return 1;
}

void vaciarArchivo(char *path) {
    sem_t *semArchivo = obtenerSemaforoPath(path);
    sem_wait(semArchivo);
    FILE *archivo = fopen(path, "w");
    fclose(archivo);
    sem_post(semArchivo);
}

static void liberarTabla(t_dictionary *tablaDeKeys) {
    dictionary_destroy(tablaDeKeys);
}

t_response *lfsDrop(char *nombreTabla) {
    t_response *retorno = (t_response *) malloc(sizeof(t_response));
    char *pathTabla = obtenerPathTabla(nombreTabla, configuracion.puntoMontaje);
    if (existeTabla(nombreTabla) == 0) {
        if (dictionary_has_key(memTable, nombreTabla)) {
            dictionary_remove_and_destroy(memTable, nombreTabla, liberarTabla);
        }
        borrarContenidoDeDirectorio(pathTabla);
        rmdir(pathTabla);
        retorno->tipoRespuesta = RESPUESTA;
        retorno->valor = concat(3, "La tabla ", nombreTabla, " fue eliminada con exito.");
    } else {
        retorno->tipoRespuesta = ERR;
        retorno->valor = concat(3, "La tabla ", nombreTabla, " no existe.\n");
    }

    free(nombreTabla);
    return retorno;
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
                retorno->valor = concat(3, "La tabla ", nombreTabla, " ya existe. Se creo su Metadata.");
            } else {
                retorno->tipoRespuesta = ERR;
                retorno->valor = concat(3, "La tabla ", nombreTabla, " ya existe.");
            }
            free(path);
        } else {
            t_dictionary *keysEnTabla = dictionary_create();
            dictionary_put(memTable, nombreTabla, keysEnTabla);

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

void validarValor(char *valor, t_response *retorno) {
    if (validarComillasValor(valor) != 0) {
        retorno->tipoRespuesta = ERR;
        retorno->valor = concat(1, "El valor debe estar enmascarado con \"\"");
        return retorno;
    } else if (validarTamanioValor(valor) != 0) {
        retorno->tipoRespuesta = ERR;
        retorno->valor = concat(3, "El valor no puede ser mayor a ",
                                string_from_format("%i", configuracion.tamanioValue), " bytes.");
        return retorno;
    }
    return retorno;
}

t_response *lfsInsert(char *nombreTabla, char *key, char *valor, time_t timestamp) {
    t_response *retorno = (t_response *) malloc(sizeof(t_response));
    retorno->tipoRespuesta = RESPUESTA;
    validarValor(valor, retorno);
    if (retorno->tipoRespuesta == ERR) {
        return retorno;
    } else {
        valorSinComillas(valor);
        // Verificar que la tabla exista en el file system. En caso que no exista, informa el error y continúa su ejecución.
        if (existeTabla(nombreTabla) == 0) {
            // Obtener la metadata asociada a dicha tabla.
            char *path = obtenerPathMetadata(nombreTabla, configuracion.puntoMontaje);
            if (existeElArchivo(path)) {
                char *linea = armarLinea(key, valor, timestamp);
                t_dictionary *keysEnTabla;
                if (dictionary_has_key(memTable, nombreTabla)) {
                    keysEnTabla = dictionary_get(memTable, nombreTabla);
                } else {
                    keysEnTabla = dictionary_create();
                    dictionary_put(memTable, nombreTabla, keysEnTabla);
                }
                t_list *listaDeRegistros;
                if (dictionary_has_key(keysEnTabla, key)) {
                    listaDeRegistros = (t_list *) dictionary_get(keysEnTabla, key);
                } else {
                    listaDeRegistros = list_create();
                    dictionary_put(keysEnTabla, key, listaDeRegistros);
                }
                list_add(listaDeRegistros, linea);
                retorno->tipoRespuesta = RESPUESTA;
                retorno->valor = concat(1, "Se inserto el valor con exito.");
            } else {
                retorno->tipoRespuesta = ERR;
                retorno->valor = concat(5, "No se pudo insertar en ", nombreTabla, ". No existe metadata en ", path,
                                        ".");
            }
        } else {
            retorno->tipoRespuesta = ERR;
            retorno->valor = concat(3, "No existe la tabla ", nombreTabla, ".");
        }
    }
    return retorno;
}

t_response *lfsInsertCompactacion(char *nombreTabla, char *key, char *valor, time_t timestamp) {
    t_response *retorno = (t_response *) malloc(sizeof(t_response));
    retorno->tipoRespuesta = RESPUESTA;
    validarValor(valor, retorno);
    if (retorno->tipoRespuesta == ERR) {
        return retorno;
    } else {
        valorSinComillas(valor);
        // Verificar que la tabla exista en el file system. En caso que no exista, informa el error y continúa su ejecución.
        if (existeTabla(nombreTabla) == 0) {
            // Obtener la metadata asociada a dicha tabla.
            char *path = obtenerPathMetadata(nombreTabla, configuracion.puntoMontaje);
            if (existeElArchivo(path)) {
                // printf("Existe metadata en %s\n", path);
                // TODO: Verificar si existe en memoria una lista de datos a dumpear. De no existir, alocar dicha memoria.
                char *linea = armarLinea(key, valor, timestamp);
                t_dictionary *keysEnTabla;
                if (dictionary_has_key(memTable, nombreTabla)) {
                    keysEnTabla = dictionary_get(memTable, nombreTabla);
                } else {
                    keysEnTabla = dictionary_create();
                    dictionary_put(memTable, nombreTabla, keysEnTabla);
                }
                t_list *listaDeRegistros;
                if (dictionary_has_key(keysEnTabla, key)) {
                    listaDeRegistros = dictionary_get(keysEnTabla, key);
                } else {
                    listaDeRegistros = list_create();
                }
                list_add(listaDeRegistros, linea);
                obtenerMetadata(nombreTabla);
                t_metadata *meta = dictionary_get(metadatas, nombreTabla);
                int particion = calcularParticion(key, meta);
                char *nombreArchivo = string_new();
                char *p = string_itoa(particion);
                nombreArchivo = concat(4, obtenerPathTabla(nombreTabla, configuracion.puntoMontaje), "/", p, ".bin");
                t_bloqueAsignado *bloqueA = (t_bloqueAsignado *) malloc(sizeof(t_bloqueAsignado));
                bloqueA->tabla = concat(1, nombreTabla);
                bloqueA->particion = particion;
                printf("Linea %s", linea);
                // TODO: Insertar en la memoria temporal del punto anterior una nueva entrada que contenga los datos enviados en la request.
                int indice = 0;
                int bloque;
                while (linea[indice] != '\0' && indice < string_length(linea)) {
                    bloque = obtenerBloqueDisponible(nombreTabla, particion);
                    char *pathBloque = obtenerPathBloque(bloque);
                    sem_t *semBloque = obtenerSemaforoPath(pathBloque);
                    sem_wait(semBloque);
                    FILE *f = fopen(pathBloque, "a");
                    dictionary_put(bloquesAsignados, (char *) string_from_format("%i", bloque), bloqueA);
                    int longitudArchivo = obtenerTamanioBloque(bloque);
                    while (getc(f) != EOF) {
                        longitudArchivo++;
                    }
                    while (longitudArchivo < obtenerTamanioBloques(configuracion.puntoMontaje) &&
                           indice < string_length(linea)) {
                        fputc(linea[indice], f);
                        longitudArchivo++;
                        indice++;
                    }
                    fclose(f);
                    sem_post(semBloque);
                    free(pathBloque);
                    if (obtenerTamanioBloque(bloque) >= obtenerTamanioBloques(configuracion.puntoMontaje)) {
                        bitarray_set_bit(bitmap, bloque);
                    }
                }
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
                sem_t *semParticion = obtenerSemaforoPath(nombreArchivo);
                sem_wait(semParticion);
                FILE *fParticion = fopen(nombreArchivo, "r+");
                char *contenido = generarContenidoParaParticion(string_from_format("%i", size), bloques);
                fwrite(contenido, sizeof(char) * strlen(contenido), 1, fParticion);
                fclose(fParticion);
                sem_post(semParticion);
                free(contenido);
                config_destroy(archivoConfig);
                retorno->tipoRespuesta = RESPUESTA;
                retorno->valor = concat(1, "Se inserto el valor con exito.");

            } else {
                retorno->tipoRespuesta = ERR;
                retorno->valor = concat(5, "No se pudo insertar en ", nombreTabla, ". No existe metadata en ", path,
                                        ".");
            }
        } else {
            retorno->tipoRespuesta = ERR;
            retorno->valor = concat(3, "No existe la tabla ", nombreTabla, ".");
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

int ordenarPorLinea(char *linea, char *linea2) {
    char **lineaDesarmada = desarmarLinea(linea);
    char **linea2Desarmada = desarmarLinea(linea2);
    int esMenor = atoi(lineaDesarmada[0]) < atoi(linea2Desarmada[0]);
    return esMenor;
}

t_response *lfsSelect(char *nombreTabla, char *key) {
    t_response *retorno = (t_response *) malloc(sizeof(t_response));
    //1. Verificar que la tabla exista en el File System
    if (existeTabla(nombreTabla) == 0) {
        char *path = obtenerPathMetadata(nombreTabla, configuracion.puntoMontaje);
        if (existeElArchivo(path)) {
            //2. Obtener la metadata asociada a dicha tabla
            obtenerMetadata(nombreTabla);
            t_metadata *meta = dictionary_get(metadatas, nombreTabla);

            //3. Calcular cual es la partición que contiene dicho KEY
            int particion = calcularParticion(key, meta);

            //4. Escanear la partición objetivo, todos los archivos temporales y la memoria temporal de dicha tabla (si existe) buscando la key deseada

            char *nombreArchivoParticion = obtenerNombreArchivoParticion(particion);
            char *binaryPath = obtenerPathArchivo(nombreTabla, nombreArchivoParticion);
            sem_t *semBinario = obtenerSemaforoPath(binaryPath);
            sem_wait(semBinario);
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
            char **bloques = bloquesEnParticion(nombreTabla, nombreArchivoParticion);
            int tamanioArray = tamanioDeArrayDeStrings(bloques);
            for (int i = 0; i < tamanioArray; i++) {

                //4.1. Escaneo particion objetivo
                char *blockPath = obtenerPathBloque(atoi(bloques[i]));
                sem_t *semBloque = obtenerSemaforoPath(blockPath);
                sem_wait(semBloque);
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
                free(blockPath);
                fclose(binarioBloque);
                sem_post(semBloque);
            }

            for (int i = 0; i < tamanioDeArrayDeStrings(bloques); i++) {
                free(bloques[i]);
            }
            free(bloques);

            fclose(binarioParticion);
            sem_post(semBinario);

            //4.2. Escaneo los archivos temporales


            //4.2. Escaneo la memoria temporal de la tabla
            if (dictionary_has_key(memTable, nombreTabla)) {
                t_dictionary *tabla = dictionary_get(memTable, nombreTabla);
                if (dictionary_has_key(tabla, key)) {
                    t_list *listaDeRegistros = (t_list *) dictionary_get(tabla, key);
                    if (!list_is_empty(listaDeRegistros)) {
                        int tamanioLista = list_size(listaDeRegistros);
                        t_list *listaOrdenada = list_sorted(listaDeRegistros, ordenarPorLinea);
                        char *elemento = list_get(listaOrdenada, (tamanioLista - 1));
                        if (elemento != NULL) {
                            char **lineaPartida = desarmarLinea(elemento);
                            char *mayorTimestampMem = string_duplicate(lineaPartida[0]);
                            if (atoi(mayorTimestampMem) > mayorTimestamp) {
                                mayorTimestamp = atoi(mayorTimestampMem);
                                valorMayorTimestamp = string_duplicate(lineaPartida[2]);
                                mayorLinea = string_new();
                                mayorLinea = concat(1, elemento);
                            }
                        }
                    }
                }
            }


            //5. Encontradas las entradas para dicha Key, se retorna el valor con el Timestamp más grande
            if (strcmp(valorMayorTimestamp, "") != 0) {
                retorno->tipoRespuesta = RESPUESTA;
                retorno->valor = concat(1, mayorLinea);
                return retorno;
            } else {
                retorno->tipoRespuesta = ERR;
                retorno->valor = concat(1, "No se encontro ningun valor con esa key.");
                free(mayorLinea);
                return retorno;
            }
        } else {
            retorno->tipoRespuesta = ERR;
            retorno->valor = concat(3, "No existe la Metadata de la Tabla ", nombreTabla, ".");
            return retorno;
        }
    } else {
        retorno->tipoRespuesta = ERR;
        retorno->valor = concat(3, "No existe la tabla ", nombreTabla, ".");
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
    if (existeElArchivo(path)) {
        t_config *archivoConfig = abrirArchivoConfiguracion(path, logger);
        char **arrayDeBloques = config_get_array_value(archivoConfig, "BLOCKS");
        config_destroy(archivoConfig);
        free(path);
        return arrayDeBloques;
    } else {
        free(path);
        return NULL;
    }
}

void ejecutarConsola() {
    t_comando comando;

    do {
        char *leido = readline("Lissandra@suck-ets:~$ ");
        char **comandoParseado = parser(leido);
        if (comandoParseado == NULL) {
            free(comandoParseado);
            continue;
        }
        comando = instanciarComando(comandoParseado);
        if (validarComandosComunes(comando, logger)) {
            t_response *retorno = gestionarRequest(comando);
            if (retorno->tipoRespuesta == ERR) {
                log_warning(logger, retorno->valor);
            } else {
                if (!string_ends_with(retorno->valor, "\n")) {
                    string_append(&retorno->valor, "\n");
                }
                printf("%s", retorno->valor);
                free(retorno->valor);
                log_info(logger, "Request procesada correctamente.");
            }
            free(retorno);
        }

    } while (comando.tipoRequest != EXIT);
    printf("Ya se analizo todo lo solicitado.\n");
}

t_response *gestionarRequest(t_comando comando) {

    t_response *retorno;
    if (!existeMetadata()) {
        retorno = (t_response *) malloc(sizeof(t_response));
        retorno->valor = concat(1, "La Metadata del File System no existe o esta vacia.");
        retorno->tipoRespuesta = ERR;
        return retorno;
    }
    switch (comando.tipoRequest) {
        case SELECT:
            retorno = lfsSelect(comando.parametros[0], comando.parametros[1]);
            return retorno;

        case INSERT:;
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
            retorno = lfsCreate(comando.parametros[0], comando.parametros[1], comando.parametros[2],
                                comando.parametros[3]);
            return retorno;

        case DESCRIBE:
            if (comando.cantidadParametros == 0) {
                retorno = lfsDescribeAll();
            } else {
                retorno = lfsDescribe(comando.parametros[0]);
            }
            return retorno;

        case DROP:
            lfsDump();
            retorno = lfsDescribe(comando.parametros[0]);
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
    config_destroy(config);
}

int calcularParticion(char *key, t_metadata *metadata) {
    int k = atoi(key);
    int b = metadata->partitions;
    return k % b;
}

pthread_t *crearHiloConexiones(GestorConexiones *unaConexion, int tamanioValue, t_log *logger) {
    pthread_t *hiloConexiones = (pthread_t *) malloc(sizeof(pthread_t));

    parametros_thread_lfs *parametros = (parametros_thread_lfs *) malloc(sizeof(parametros_thread_lfs));

    parametros->conexion = unaConexion;
    parametros->logger = logger;
    parametros->tamanioValue = tamanioValue;

    pthread_create(hiloConexiones, NULL, &atenderConexiones, parametros);

    return hiloConexiones;
}

pthread_t *crearHiloDump(t_log *logger) {
    pthread_t *hiloConexiones = (pthread_t *) malloc(sizeof(pthread_t));

    parametros_thread_lfs *parametros = (parametros_thread_lfs *) malloc(sizeof(parametros_thread_lfs));
    parametros->logger = logger;

    pthread_create(hiloConexiones, NULL, &lfsDump, parametros);

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

char **obtenerTablas() {
    DIR *d;
    struct dirent *dir;
    char **tablas;
    char *nombreArchivo = string_new();
    string_append(&nombreArchivo, configuracion.puntoMontaje);
    string_append(&nombreArchivo, "Tables");
    sem_t *semTable = obtenerSemaforoPath(nombreArchivo);
    sem_wait(semTable);
    d = opendir(nombreArchivo);
    int count = 0;
    int countOfDirectories = 0;
    if (d) {
        struct dirent *ep = readdir(d);
        while (NULL != ep) {
            countOfDirectories++;
            ep = readdir(d);
        }

        rewinddir(d);
        free(ep);
        tablas = calloc(countOfDirectories - 1, sizeof(char *));
        while ((dir = readdir(d)) != NULL) {
            if (strcmp(dir->d_name, ".") != 0 && strcmp(dir->d_name, "..") != 0) {
                tablas[count] = strdup(dir->d_name);
                count++;
            }
        }
        closedir(d);
        sem_post(semTable);
    }
    tablas[count] = NULL;

    free(nombreArchivo);
    return tablas;
}

void cargarBloquesAsignados(char *path) {
    DIR *d;
    struct dirent *dir;
    sem_t *semDir = obtenerSemaforoPath(path);
    sem_wait(semDir);
    d = opendir(path);
    if (d) {
        while ((dir = readdir(d)) != NULL) {
            if (strcmp(dir->d_name, ".") != 0 && strcmp(dir->d_name, "..") != 0) {
                char *nombreTabla = dir->d_name;
                obtenerMetadata(nombreTabla);
                t_metadata *meta = dictionary_get(metadatas, nombreTabla);
                int *partitions = meta->partitions;
                for (int i = 0; i < partitions; i++) {
                    char *nombreArchivo = string_from_format("%i", i);
                    string_append(&nombreArchivo, ".bin");
                    char **arrayDeBloques = bloquesEnParticion(nombreTabla, nombreArchivo);
                    for (int j = 0; j < tamanioDeArrayDeStrings(arrayDeBloques); j++) {
                        t_bloqueAsignado *bloque = (t_bloqueAsignado *) malloc(sizeof(t_bloqueAsignado));
                        bloque->tabla = concat(1, nombreTabla);
                        bloque->particion = i;
                        dictionary_put(bloquesAsignados, arrayDeBloques[j], bloque);
                    }
                    for (int k = 0; k < tamanioDeArrayDeStrings(arrayDeBloques); k++) {
                        free(arrayDeBloques[k]);
                    }
                    free(arrayDeBloques);
                    free(nombreArchivo);
                }
            }
        }
        closedir(d);
        sem_post(semDir);
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

int existeMetadata() {
    char *nombreArchivo = string_new();
    string_append(&nombreArchivo, configuracion.puntoMontaje);
    string_append(&nombreArchivo, "Metadata/Metadata.bin");
    int existe = existeElArchivo(nombreArchivo) && !archivoVacio(nombreArchivo);
    free(nombreArchivo);
    return existe;
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
            char *nroBloque = string_from_format("%i", i);
            char *unArchivoDeBloque = concat(4, nombreArchivo, "/", nroBloque, ".bin");
            t_bloqueAsignado *bloque = (t_bloqueAsignado *) malloc(sizeof(t_bloqueAsignado));
            bloque->tabla = concat(1, "");
            bloque->particion = NULL;
            dictionary_put(bloquesAsignados, nroBloque, bloque);
            free(nroBloque);
            if (!existeElArchivo(unArchivoDeBloque)) {
                sem_t *semBloque = obtenerSemaforoPath(unArchivoDeBloque);
                sem_wait(semBloque);
                FILE *file = fopen(unArchivoDeBloque, "w");
                fclose(file);
                sem_post(semBloque);
            } else if (!archivoVacio(unArchivoDeBloque) &&
                       obtenerTamanioBloque(i) >= obtenerTamanioBloques(configuracion.puntoMontaje)) {
                bitarray_set_bit(bitmap, i);
            }
            free(unArchivoDeBloque);
            //printf("%i", bitarray_test_bit(bitmap, i));
            //No hago un free del bloque porque lo necesito para el diccionario de bloques, en que momento deberia liberarlo?
        }
        free(nombreArchivo);
    }
}

void crearDirMetadata(char *puntoMontaje) {
    char *nombreArchivo = crearDirEnPuntoDeMontaje(puntoMontaje, "Metadata");
    char *pathArchivoMetadata = concat(2, nombreArchivo, "/Metadata.bin");
    char *pathArchivoBitmap = concat(2, nombreArchivo, "/Bitmap.bin");
    if (!existeElArchivo(pathArchivoMetadata)) {
        sem_t *semMetadata = obtenerSemaforoPath(pathArchivoMetadata);
        sem_wait(semMetadata);
        FILE *fm = fopen(pathArchivoMetadata, "w");
        fclose(fm);
        sem_post(semMetadata);
    }
    if (!existeElArchivo(pathArchivoBitmap)) {
        sem_t *semBitmap = obtenerSemaforoPath(pathArchivoBitmap);
        sem_wait(semBitmap);
        FILE *fb = fopen(pathArchivoBitmap, "w");
        fclose(fb);
        sem_post(semBitmap);
    }
    free(nombreArchivo);
    free(pathArchivoMetadata);
    free(pathArchivoBitmap);
}

sem_t *obtenerSemaforoPath(char *path) {
    sem_t *semaforo;
    if (dictionary_has_key(archivosAbiertos, path)) {
        semaforo = dictionary_get(archivosAbiertos, path);
    } else {
        semaforo = (sem_t *) malloc(sizeof(sem_t));
        sem_init(semaforo, 0, 1);
        dictionary_put(archivosAbiertos, path, semaforo);
    }
    return semaforo;
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
    archivosAbiertos = dictionary_create();
    memTable = dictionary_create();
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
    pthread_t *hiloDump = crearHiloDump(logger);

    ejecutarConsola();

    pthread_join(&hiloConexiones, NULL);

    free(misConexiones);
    free(bloquesAsignados);
    free(metadatas);
    return 0;
}