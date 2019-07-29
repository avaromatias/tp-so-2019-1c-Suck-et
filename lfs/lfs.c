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
        //log_error(logger, "Alguna de las claves obligatorias no están setteadas en el archivo de configuración.");
        printf("Alguna de las claves obligatorias no están setteadas en el archivo de configuración.\n");
        fflush(stdout);
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

void* atenderMensajes(void *parametrosRequest) {
    parametros_thread_request *parametros = (parametros_thread_request *) parametrosRequest;
    Header header = parametros->header;
    char *mensaje = parametros->mensaje;
    log_info(logger, "REQUEST RECIBIDA: %s", mensaje);
    //printf("%s\n", mensaje);
    //fflush(stdout);
    char *request = string_duplicate(mensaje);
    char **comandoParseado = parser(mensaje);
    t_response *retorno;
    t_comando comando = instanciarComando(comandoParseado);
    retorno = gestionarRequest(comando);
    loguearRespuesta(request, retorno);

    if (header.tipoRequest == CREATE) {
        if (retorno->tipoRespuesta == RESPUESTA) {
            retorno->valor = concat(4, "CREATE OK |", comando.parametros[0], ";", comando.parametros[1]);
        } else {
            if (string_contains(retorno->valor, "YA_EXISTE")) {
                retorno->valor = concat(4, "CREATE ERROR |", comando.parametros[0], ";", comando.parametros[1]);
            }
        }
    }
    enviarPaquete(header.fdRemitente, retorno->tipoRespuesta, comando.tipoRequest, retorno->valor, header.pid);
    free(request);
    free(retorno);
    freeArrayDeStrings(comandoParseado);
}

char *obtenerPathArchivo(char *nombreTabla, char *nombreArchivo) {
    char *tablePath = obtenerPathTabla(nombreTabla, configuracion.puntoMontaje);
    string_append(&tablePath, "/");
    string_append(&tablePath, nombreArchivo);
    return tablePath;
}

char *obtenerPathBloque(int numberoBloque) {
    char *nroDeBloque = string_itoa(numberoBloque);
    char *unArchivoDeBloque = concat(4, configuracion.puntoMontaje, "Bloques/", nroDeBloque, ".bin");
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
    pthread_mutex_t *semMetadata = (pthread_mutex_t *) obtenerSemaforoPath(obtenerPathArchivo(nombreTabla, "Metadata"));
    pthread_mutex_lock(semMetadata);
    FILE *f = fopen(obtenerPathArchivo(nombreTabla, "Metadata"), "w");
    char *linea = concat(9, "CONSISTENCY=", tipoConsistencia, "\n", "PARTITIONS=", particiones, "\n",
                         "COMPACTION_TIME=",
                         tiempoCompactacion, "\n");
    fwrite(linea, sizeof(char), strlen(linea), f);
    free(linea);
    fclose(f);
    pthread_mutex_unlock(semMetadata);
}

void crearBinarios(char *nombreTabla, int particiones) {
    for (int i = 0; i < particiones; i++) {/*
        t_bloqueAsignado *bloqueA = (t_bloqueAsignado *) malloc(sizeof(t_bloqueAsignado));
        bloqueA->tabla = concat(1, nombreTabla);
        bloqueA->particion = i;*/
        //pthread_mutex_lock(mutexAsignacionBloques);
        int bloque = obtenerBloqueDisponible(nombreTabla, i);
        if (bloque != -1) {
            char *bloqueString = string_itoa(bloque);
            //dictionary_put(bloquesAsignados, bloqueString, bloqueA);
            //pthread_mutex_unlock(mutexAsignacionBloques);
            char *nombreArchivo = string_itoa(i);
            string_append(&nombreArchivo, ".bin");
            char *pathArchivo = obtenerPathArchivo(nombreTabla, nombreArchivo);
            pthread_mutex_t *semArchivo = (pthread_mutex_t *) obtenerSemaforoPath(pathArchivo);
            pthread_mutex_lock(semArchivo);
            FILE *file = fopen(pathArchivo, "w");
            char *bloquesAsignadosAParticion = obtenerBloquesAsignados(nombreTabla, i);
            int cantidadBloquesAsignados = tamanioDeArrayDeStrings(
                    convertirStringDeBloquesAArray(bloquesAsignadosAParticion));
            int tamanio = ((cantidadBloquesAsignados - 1) * metadataFS->block_size) +
                          obtenerTamanioBloque(bloque);
            char *tamanioString = string_itoa(tamanio);
            char *contenido = generarContenidoParaParticion(tamanioString, bloquesAsignadosAParticion);
            fwrite(contenido, sizeof(char) * strlen(contenido), 1, file);
            free(contenido);
            free(bloquesAsignadosAParticion);
            fclose(file);
            pthread_mutex_unlock(semArchivo);
            free(pathArchivo);
            free(nombreArchivo);
            free(bloqueString);
            free(tamanioString);
        } else {
            //pthread_mutex_unlock(mutexAsignacionBloques);
        }
    }
}

char *obtenerBloquesAsignados(char *nombreTabla, int particion) {
    char *bloquesAsignadosParticion = string_duplicate("[");
    for (int i = 0; i < metadataFS->blocks; i++) {
        char *bloqueString = string_itoa(i);
        pthread_mutex_lock(mutexAsignacionBloques);
        if (bloquesAsignados->elements_amount > 0) {
            t_bloqueAsignado *bloque = dictionary_get(bloquesAsignados, bloqueString);
            if (strcmp(bloque->tabla, nombreTabla) == 0 && bloque->particion == particion) {
                string_append(&bloquesAsignadosParticion, bloqueString);
                string_append(&bloquesAsignadosParticion, ",");
            }
        }
        pthread_mutex_unlock(mutexAsignacionBloques);
        free(bloqueString);
    }
    char *retorno = string_substring_until(bloquesAsignadosParticion, strlen(bloquesAsignadosParticion) - 1);
    free(bloquesAsignadosParticion);
    string_append(&retorno, "]");
    if (strlen(retorno) > 1) {
        return retorno;
    } else {
        free(retorno);
        return string_new();
    }
}

int obtenerTamanioBloque(int bloque) {
    char *pathBloque = obtenerPathBloque(bloque);
    if (archivoVacio(pathBloque)) {
        free(pathBloque);
        return 0;
    } else {
        struct stat st;
        if (stat(pathBloque, &st) == 0)
            free(pathBloque);
        return (int) st.st_size;
    }
}

int estaLibreElBloque(int bloque) {
    pthread_mutex_lock(mutexBitarray);
    if (bitarray_test_bit(bitarray, bloque) == 0) {
        pthread_mutex_unlock(mutexBitarray);
        return 1;
    }
    pthread_mutex_unlock(mutexBitarray);
    return 0;
}

int estaDisponibleElBloqueParaTabla(int i, char *nombreTabla, int particion) {
    int bloqueDisponible = 0;
    int bloqueLibre = estaLibreElBloque(i) == 1;
    if (bloquesAsignados->elements_amount > 0) {
        char *bloqueString = string_itoa(i);
        pthread_mutex_lock(mutexAsignacionBloques);
        t_bloqueAsignado *bloque = dictionary_get(bloquesAsignados, bloqueString);
        pthread_mutex_unlock(mutexAsignacionBloques);
        free(bloqueString);
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
            char *bloqueString = string_itoa(i);
            pthread_mutex_lock(mutexAsignacionBloques);
            if (dictionary_has_key(bloquesAsignados, bloqueString)) {
                t_bloqueAsignado *bloqueA = dictionary_get(bloquesAsignados, bloqueString);
                free(bloqueA->tabla);
                bloqueA->tabla = string_duplicate(nombreTabla);
                bloqueA->particion = particion;
                dictionary_put(bloquesAsignados, bloqueString, bloqueA);
                pthread_mutex_unlock(mutexAsignacionBloques);
                return i;
            }
            pthread_mutex_unlock(mutexAsignacionBloques);
        }
    }
    return -1;
}


void retardo() {
    t_config *archivoConfig = abrirArchivoConfiguracion("../lfs.cfg", logger);
    if (config_has_property(archivoConfig, "RETARDO")) {
        int retardo = config_get_int_value(archivoConfig, "RETARDO");
        if (retardo < 0) {
            //log_error(logger, "El RETARDO no esta seteado en el Archivo de Configuracion.");
            printf("El RETARDO no esta seteado en el Archivo de Configuracion.\n");
            fflush(stdout);
            return;
        }
        retardo = retardo / 1000;
        sleep(retardo);
    }
    config_destroy(archivoConfig);
    return;
}

void* lfsDump() {
    while (1) {
        t_config *archivoConfig = abrirArchivoConfiguracion("../lfs.cfg", logger);
        if (config_has_property(archivoConfig, "TIEMPO_DUMP")) {
            int tiempoDump = config_get_int_value(archivoConfig, "TIEMPO_DUMP");
            if (!tiempoDump) {
                //log_error(logger, "El TIEMPO_DUMP no esta seteado en el Archivo de Configuracion.");
                printf("El TIEMPO_DUMP no esta seteado en el Archivo de Configuracion.\n");
                fflush(stdout);
                continue;
            }
            tiempoDump = tiempoDump / 1000;
            config_destroy(archivoConfig);
            sleep(tiempoDump);
            //log_info(logger,"Iniciando Proceso de Dump.");
            void* dumpTabla(char *nombreTabla, t_dictionary *tablaDeKeys) {
                int nroDump = 0;
                char *nombreDump = string_from_format("%s%i%s", nombreTabla, nroDump, ".tmp");
                char *nombreArchivo = obtenerPathArchivo(nombreTabla, nombreDump);
                while (existeElArchivo(nombreArchivo)) {
                    free(nombreArchivo);
                    nroDump++;
                    nombreDump = string_from_format("%s%i%s", nombreTabla, nroDump, ".tmp");
                    nombreArchivo = obtenerPathArchivo(nombreTabla, nombreDump);
                }
                pthread_mutex_lock(mutexTablasEnUso);
                pthread_mutex_t *sem = dictionary_get(tablasEnUso, nombreTabla);
                pthread_mutex_unlock(mutexTablasEnUso);
                pthread_mutex_lock(sem); // Este semaforo comento Fer
                //pthread_mutex_t *semArchivo = obtenerSemaforoPath(nombreArchivo);
                void* _dumpKey(char *key, t_list *listaDeRegistros) {
                    int index = 0;
                    void* _dumpRegistro(char *linea) {
                        /*pthread_mutex_lock(semArchivo);
                        FILE *archivoDump = fopen(nombreArchivo, "a");
                        fclose(archivoDump);
                        pthread_mutex_unlock(semArchivo);*/
                        escribirEnBloque(linea, nombreDump, -1, nombreArchivo);
                        list_remove(listaDeRegistros, index);
                    }
                    list_iterate(listaDeRegistros, (void *)_dumpRegistro);

                }
                dictionary_iterator(tablaDeKeys, (void *)_dumpKey);
                pthread_mutex_unlock(sem);
                free(nombreArchivo);
            }
            pthread_mutex_lock(mutexMemtable);
            int cantidadInsertsActual;
            sem_getvalue(cantidadRegistrosMemtable, &cantidadInsertsActual);
            sem_wait_n(cantidadRegistrosMemtable, cantidadInsertsActual);
            dictionary_iterator(memTable, (void *)dumpTabla);

            pthread_mutex_unlock(mutexMemtable);
        } else {
            //log_error(logger, "El TIEMPO_DUMP no esta seteado en el Archivo de Configuracion.");
            printf("El TIEMPO_DUMP no esta seteado en el Archivo de Configuracion.\n");
            fflush(stdout);
            continue;
        }
    }
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
            char *respuestaTabla;
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
    freeArrayDeStrings(tablas);
    return retorno;
}

t_response *lfsDescribe(char *nombreTabla) {
    t_response *retorno = (t_response *) malloc(sizeof(t_response));
    if (existeTabla(nombreTabla) == 0) {
        char *pathMetadata = obtenerPathArchivo(nombreTabla, "Metadata");
        pthread_mutex_t *semMetadata = (pthread_mutex_t *) obtenerSemaforoPath(pathMetadata);
        pthread_mutex_lock(semMetadata);
        FILE *metadataTabla = fopen(pathMetadata, "r");
        if (metadataTabla != NULL) {
            fseek(metadataTabla, 0, SEEK_END);
            long fsize = ftell(metadataTabla);
            fseek(metadataTabla, 0, SEEK_SET);  /* same as rewind(f); */
            char *string = malloc(fsize + 1);
            fread(string, 1, fsize, metadataTabla);
            fseek(metadataTabla, 0, SEEK_SET);
            fclose(metadataTabla);
            pthread_mutex_unlock(semMetadata);
            string[fsize] = 0;
            free(pathMetadata);
            retorno->tipoRespuesta = RESPUESTA;
            retorno->valor = string;
        } else {
            pthread_mutex_unlock(semMetadata);
            retorno->tipoRespuesta = ERR;
            retorno->valor = concat(3, "No se encontro la Metadata de la tabla ", nombreTabla, ".");
        }
    } else {
        retorno->tipoRespuesta = ERR;
        retorno->valor = concat(3, "La tabla ", nombreTabla, " no existe.");
    }

    return retorno;
}

int deleteFile(char *nombreArchivo) {
    int status;
    pthread_mutex_t *semArchivo = (pthread_mutex_t *) obtenerSemaforoPath(nombreArchivo);
    pthread_mutex_lock(semArchivo);
    status = remove(nombreArchivo);
    pthread_mutex_unlock(semArchivo);
    return status;
}


int borrarContenidoDeDirectorio(char *dirPath) {
    DIR *d;
    struct dirent *dir;
    d = opendir(dirPath);
    if (d) {
        int puntoEncontrado = 0;
        int puntoPuntoEncontrado = 0;
        while ((dir = readdir(d)) != NULL) {
            char *nombreTabla;
            nombreTabla = string_duplicate(dir->d_name);
            char *pathArchivo = string_new();
            string_append(&pathArchivo, dirPath);
            string_append(&pathArchivo, "/");
            string_append(&pathArchivo, nombreTabla);
            if (strcmp(dir->d_name, ".") == 0) {
                puntoEncontrado = 1;
            }
            if (strcmp(dir->d_name, "..") == 0) {
                puntoPuntoEncontrado = 1;
            }
            if (strcmp(dir->d_name, ".") != 0 && strcmp(dir->d_name, "..") != 0) {
                if (string_contains(dir->d_name, ".bin")) {
                    if (!archivoVacio(pathArchivo)) {
                        t_config *archivoConfig = abrirArchivoConfiguracion(pathArchivo, logger);
                        char **blocks = config_get_array_value(archivoConfig, "BLOCKS");
                        liberarBloques(blocks);
                        config_destroy(archivoConfig);
                    }
                }
                if (deleteFile(pathArchivo) != 0) {
                    return 0;
                }
            }
            free(pathArchivo);
            free(nombreTabla);
        }
        if (puntoEncontrado) {
            char *pathArchivo = string_new();
            string_append(&pathArchivo, dirPath);
            string_append(&pathArchivo, "/.");
            if (deleteFile(pathArchivo) != 0) {
                free(pathArchivo);
                return 0;
            }
            free(pathArchivo);
        }
        if (puntoPuntoEncontrado) {
            char *pathArchivo = string_new();
            string_append(&pathArchivo, dirPath);
            string_append(&pathArchivo, "/..");
            if (deleteFile(pathArchivo) != 0) {
                free(pathArchivo);
                return 0;
            }
            free(pathArchivo);
        }
        closedir(d);
    }
    free(dirPath);
    return 1;
}

void vaciarArchivo(char *path) {
    pthread_mutex_t *semArchivo = (pthread_mutex_t *) obtenerSemaforoPath(path);
    pthread_mutex_lock(semArchivo);
    FILE *archivo = fopen(path, "w");
    fclose(archivo);
    pthread_mutex_unlock(semArchivo);
}

void* liberarTabla(t_dictionary *tablaDeKeys) {
    int cantidadDeRegistros = 0;
    void* contarDeRegistros(char* key,t_list *listaDeRegistros) {
        void* eliminarElementosLista(void *elem) {
            free(elem);
        }
        cantidadDeRegistros += list_size(listaDeRegistros);
        list_destroy_and_destroy_elements(listaDeRegistros, (void *)eliminarElementosLista);

    }
    dictionary_iterator(tablaDeKeys, (void *)contarDeRegistros);
    dictionary_destroy(tablaDeKeys);
    sem_wait_n(cantidadRegistrosMemtable, cantidadDeRegistros);
}

t_response *lfsDrop(char *nombreTabla) {
    t_response *retorno = (t_response *) malloc(sizeof(t_response));
    char *pathTabla = obtenerPathTabla(nombreTabla, configuracion.puntoMontaje);
    if (existeTabla(nombreTabla) == 0) {
        pthread_mutex_lock(mutexTablasEnUso);
        if (dictionary_has_key(tablasEnUso, nombreTabla)) {
            pthread_mutex_t *semaforoTabla = dictionary_get(tablasEnUso, nombreTabla);
            pthread_mutex_unlock(mutexTablasEnUso);
            pthread_mutex_lock(semaforoTabla);
            pthread_mutex_unlock(semaforoTabla);
        } else {
            pthread_mutex_t *semaforoTabla = malloc(sizeof(pthread_mutex_t));
            int init = pthread_mutex_init(semaforoTabla, NULL);
            dictionary_put(tablasEnUso, nombreTabla, semaforoTabla);
            pthread_mutex_unlock(mutexTablasEnUso);
            pthread_mutex_lock(semaforoTabla);
            pthread_mutex_unlock(semaforoTabla);
        }

        pthread_mutex_lock(mutexHilosTablas);
        if (dictionary_has_key(hilosTablas, nombreTabla)) {
            pthread_t *hiloTabla = dictionary_get(hilosTablas, nombreTabla);
            pthread_cancel(*hiloTabla);
            dictionary_remove(hilosTablas, nombreTabla);
        }
        pthread_mutex_unlock(mutexHilosTablas);
        borrarContenidoDeDirectorio(pathTabla);
        if (rmdir(pathTabla) != 0) {
            retorno->tipoRespuesta = ERR;
            retorno->valor = concat(3, "Ocurrio un error al eliminar la tabla ", nombreTabla, ".");
        } else {
            pthread_mutex_lock(mutexMemtable);
            if (dictionary_has_key(memTable, nombreTabla)) {
                dictionary_remove_and_destroy(memTable, nombreTabla, (void *)liberarTabla);
            }
            pthread_mutex_unlock(mutexMemtable);
            retorno->tipoRespuesta = RESPUESTA;
            retorno->valor = concat(3, "La tabla ", nombreTabla, " fue eliminada con exito.");
        }
    } else {
        retorno->tipoRespuesta = ERR;
        retorno->valor = concat(3, "La tabla ", nombreTabla, " no existe.");
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
                retorno->tipoRespuesta = RESPUESTA;
                retorno->valor = concat(3, "YA_EXISTE | La tabla ", nombreTabla, " ya existe. Se creo su Metadata.");
            } else {
                retorno->tipoRespuesta = RESPUESTA;
                retorno->valor = concat(3, "YA_EXISTE | La tabla ", nombreTabla, " ya existe.");
            }
            free(path);
        } else {
            t_dictionary *keysEnTabla = dictionary_create();
            pthread_mutex_lock(mutexMemtable);
            dictionary_put(memTable, nombreTabla, keysEnTabla);
            pthread_mutex_unlock(mutexMemtable);
            pthread_mutex_t *semaforoTabla = malloc(sizeof(pthread_mutex_t));
            int init = pthread_mutex_init(semaforoTabla, NULL);
            pthread_mutex_lock(mutexTablasEnUso);
            dictionary_put(tablasEnUso, nombreTabla, semaforoTabla);
            pthread_mutex_unlock(mutexTablasEnUso);
            // Crear el directorio para dicha tabla.
            mkdir(tablePath, 0777);
            // Crear el directorio para dicha tabla.
            // Grabar en dicho archivo los parámetros pasados por el request.
            crearMetadata(nombreTabla, tipoConsistencia, particiones, tiempoCompactacion);
            // Crear los archivos binarios asociados a cada partición de la tabla y
            // asignar a cada uno un bloque
            crearBinarios(nombreTabla, atoi(particiones));
            pthread_t *hilo = crearHiloCompactacion(nombreTabla, tiempoCompactacion);
            pthread_detach(*hilo);
            pthread_mutex_lock(mutexHilosTablas);
            dictionary_put(hilosTablas, nombreTabla, hilo);
            pthread_mutex_unlock(mutexHilosTablas);
            retorno->tipoRespuesta = RESPUESTA;
            retorno->valor = concat(3, "La tabla ", nombreTabla, " fue creada con exito.\n");
        }
        return retorno;
    }
}

char **obtenerLineasDeBloques(char **bloques) {
    char *blockPath = NULL;
    bool lineaContinuaEnOtroArchivo = false;
    char *linea = string_new();
    char seek;
    char str[2];
    str[1] = '\0';
    char **palabras = NULL;
    int tamanioArray = tamanioDeArrayDeStrings(bloques);
    char *stringDeLineas = string_new();

    for (int i = 0; i < tamanioArray; i++) {
        blockPath = obtenerPathBloque(atoi(bloques[i]));
        pthread_mutex_t *semArchivo = (pthread_mutex_t *) obtenerSemaforoPath(blockPath);
        pthread_mutex_lock(semArchivo);
        FILE *binarioBloque = fopen(blockPath, "r");


        while (!feof(binarioBloque)) {
            if (!lineaContinuaEnOtroArchivo) {
                vaciarString(&linea);
            }
            while ((seek = getc(binarioBloque)) != EOF && seek != '\n') {
                str[0] = seek;
                string_append(&linea, str);
            }
            if (strcmp(linea, "") != 0) {
                palabras = desarmarLinea(linea);
                if (tamanioDeArrayDeStrings(palabras) == 3 && seek == '\n') { // Si la línea no continua en otro bloque
                    lineaContinuaEnOtroArchivo = false;
                    string_append(&stringDeLineas, linea);
                    string_trim(&stringDeLineas);
                    string_append(&stringDeLineas, ",");
                } else {
                    lineaContinuaEnOtroArchivo = true;
                }
            }
        }
//        if(tamanioDeArrayDeStrings(palabras)) freeArrayDeStrings(palabras);
        vaciarString(&blockPath);
        fclose(binarioBloque);
        pthread_mutex_unlock(semArchivo);
    }
    if (!string_is_empty(stringDeLineas)) {
        stringDeLineas = string_substring_until(stringDeLineas, strlen(stringDeLineas) - 1);

        if (blockPath != NULL) free(blockPath);
    } else {
        char **bloquesVacios;
        bloquesVacios[0] = NULL;
        return bloquesVacios;
    }
    free(linea);
    return convertirStringDeBloquesAArray(stringDeLineas);
}

void* compactacion(void *parametrosThread) {
    parametros_thread_compactacion *parametros = (parametros_thread_compactacion *) parametrosThread;
    char *nombreTabla = string_duplicate(parametros->tabla);
    int tiempoCompactacion = atoi(parametros->tiempoCompactacion) / 1000;
    while (1) {
        sleep(tiempoCompactacion);
        //log_info(logger,"Iniciando Proceso de Compactacion de tabla %s.",nombreTabla);
        pthread_mutex_lock(mutexTablasEnUso);
        pthread_mutex_t *sem = dictionary_get(tablasEnUso, nombreTabla);
        pthread_mutex_unlock(mutexTablasEnUso);
        time_t start1, end1, start2, end2;
        double total;
        pthread_mutex_lock(sem);
        start1 = clock();
        int existenTemporales = renombrarTemporales(nombreTabla);
        pthread_mutex_unlock(sem);
        end1 = clock();
        total = (double) (end1 - start1);
        if (existenTemporales == 1) {
            char *bloquesBin = obtenerStringBloquesSegunExtension(nombreTabla, ".bin");
            char *bloquesTemp = obtenerStringBloquesSegunExtension(nombreTabla, ".tmpc");
            char *bloquesTotales = string_new();
            if (!string_is_empty(bloquesBin)) {
                string_append(&bloquesTotales, bloquesBin);
            }
            if (!string_is_empty(bloquesTemp)) {
                string_append(&bloquesTotales, ",");
                string_append(&bloquesTotales, bloquesTemp);
            }
            char **bloques = string_split(bloquesTotales, ",");
            char **lineas = obtenerLineasDeBloques(bloques);
            if (tamanioDeArrayDeStrings(lineas) > 0) {
                char **lineasMaximas = filtrarKeyMax(lineas);
                pthread_mutex_lock(sem);
                start2 = clock();
                liberarBloques(bloques);
                int tamanioArray = tamanioDeArrayDeStrings(lineasMaximas);
                for (int j = 0; j < tamanioArray; j++) {
                    char **linea = desarmarLinea(lineasMaximas[j]);
                    char *keyString = string_duplicate(linea[1]);
                    char *valorString = string_duplicate(linea[2]);
                    char *timestampString = string_duplicate(linea[0]);
                    lfsInsertCompactacion(nombreTabla, keyString, valorString, (unsigned long long) strtol(timestampString, NULL, 10));
                    free(keyString);
                    free(valorString);
                    free(timestampString);
                }
                eliminarArchivosSegunExtension(nombreTabla, ".tmpc");
                eliminarArchivosSegunExtension(nombreTabla, ".bin");
                t_metadata *meta = obtenerMetadata(nombreTabla);
                crearBinarios(nombreTabla, meta->partitions);
                pthread_mutex_unlock(sem);
                end2 = clock();
                total += (double) (end2 - start2);
                freeArrayDeStrings(lineasMaximas);
            }
            free(bloquesBin);
            free(bloquesTemp);
            free(bloquesTotales);
            freeArrayDeStrings(lineas);
        }
        //log_info(logger, "La tabla %s estuvo bloqueada por %f segundos.", nombreTabla,(double) (total / CLOCKS_PER_SEC));
    }
    free(nombreTabla);
}

void eliminarArchivosSegunExtension(char *nombreTabla, char *extension) {
    DIR *d;
    struct dirent *dir;
    char *dirPath = obtenerPathTabla(nombreTabla, configuracion.puntoMontaje);
    d = opendir(dirPath);
    if (d) {
        while ((dir = readdir(d)) != NULL) {
            char *nombreTabla = string_duplicate(dir->d_name);
            char *pathArchivo = string_new();
            string_append(&pathArchivo, dirPath);
            string_append(&pathArchivo, "/");
            string_append(&pathArchivo, nombreTabla);
            if (strcmp(dir->d_name, ".") != 0 && strcmp(dir->d_name, "..") != 0) {
                if (string_ends_with(dir->d_name, extension)) {
                    if (deleteFile(pathArchivo) != 0) {
                        return;
                    }
                }

            }

            free(pathArchivo);
            free(nombreTabla);
        }
        closedir(d);
    }
    free(dirPath);
    return;
}

void liberarBloques(char **nroBloques) {
    int tamanioArray = tamanioDeArrayDeStrings(nroBloques);
    for (int i = 0; i < tamanioArray; i++) {
        liberarBloque(nroBloques[i]);
    }
    freeArrayDeStrings(nroBloques);
}

void liberarBloque(char *nroBloque) {
    char *bloquePath = obtenerPathBloque(atoi(nroBloque));
    vaciarArchivo(bloquePath);
    pthread_mutex_lock(mutexAsignacionBloques);
    if (dictionary_has_key(bloquesAsignados, nroBloque)) {
        t_bloqueAsignado *bloque = dictionary_get(bloquesAsignados, nroBloque);
        free(bloque->tabla);
        bloque->tabla = string_new();
        bloque->particion = -1;
    }
    pthread_mutex_lock(mutexBitarray);
    bitarray_clean_bit(bitarray, atoi(nroBloque));
    pthread_mutex_unlock(mutexBitarray);
    free(bloquePath);
    pthread_mutex_unlock(mutexAsignacionBloques);
}

char **filtrarKeyMax(char **listaLineas) {
    t_dictionary *keys = dictionary_create();
    int tamanioArray = tamanioDeArrayDeStrings(listaLineas);
    for (int i = 0; i < tamanioArray; i++) {
        char *key = desarmarLinea(listaLineas[i])[1];
        if (!dictionary_has_key(keys, key)) {
            dictionary_put(keys, key, key);
        }
    }
    char **lineasSinRepetir = calloc(dictionary_size(keys) + 1, sizeof(char *));
    void* obtenerMaxTimestamp(char *keyD, char *valorD) {
        char *mayorLinea = string_new();
        int mayorTimestamp = -1;
        for (int i = 0; i < tamanioArray; i++) {
            char *listaLineasString = string_duplicate(listaLineas[i]);
            char **linea = desarmarLinea(listaLineasString);
            char *key = linea[1];
            int timestamp = atoi(linea[0]);
            if (strcmp(keyD, key) == 0 && timestamp >= mayorTimestamp) {
                vaciarString(&mayorLinea);
                mayorTimestamp = timestamp;
                string_append(&mayorLinea, listaLineas[i]);
            }
            free(listaLineasString);
        }
        lineasSinRepetir[tamanioDeArrayDeStrings(lineasSinRepetir)] = string_duplicate(mayorLinea);
    }
    dictionary_iterator(keys, (void *)obtenerMaxTimestamp);
    lineasSinRepetir[tamanioDeArrayDeStrings(lineasSinRepetir)] = NULL;
    dictionary_destroy(keys);
    return lineasSinRepetir;
}

pthread_t *crearHiloCompactacion(char *nombreTabla, char *tiempoCompactacion) {
    pthread_t *hiloCompactacion = (pthread_t *) malloc(sizeof(pthread_t));

    parametros_thread_compactacion *parametros = (parametros_thread_compactacion *) malloc(
            sizeof(parametros_thread_compactacion));

    parametros->tabla = string_duplicate(nombreTabla);
    parametros->tiempoCompactacion = string_duplicate(tiempoCompactacion);

    pthread_create(hiloCompactacion, NULL, &compactacion, parametros);
    pthread_detach(*hiloCompactacion);

    return hiloCompactacion;
}

void validarValor(char *valor, t_response *retorno) {
    if (validarComillasValor(valor) != 0) {
        retorno->tipoRespuesta = ERR;
        retorno->valor = concat(1, "El valor debe estar enmascarado con \"\"");
    } else if (validarTamanioValor(valor) != 0) {
        retorno->tipoRespuesta = ERR;
        retorno->valor = concat(3, "El valor no puede ser mayor a ",
                                string_from_format("%i", configuracion.tamanioValue), " bytes.");
    }
}

t_response *lfsInsert(char *nombreTabla, char *key, char *valor, unsigned long long timestamp) {
    t_response *retorno = (t_response *) malloc(sizeof(t_response));
    retorno->tipoRespuesta = RESPUESTA;
    validarValor(valor, retorno);
    if (retorno->tipoRespuesta == ERR) {
        return retorno;
    } else {
        valorSinComillas(valor);
        // Verificar que la tabla exista en el file system. En caso que no exista, informa el error y continúa su ejecución.
        if (existeTabla(nombreTabla) == 0) {
            pthread_mutex_lock(mutexTablasEnUso);
            if (dictionary_has_key(tablasEnUso, nombreTabla)) {
                pthread_mutex_t *semaforoTabla = dictionary_get(tablasEnUso, nombreTabla);
                pthread_mutex_unlock(mutexTablasEnUso);
                pthread_mutex_lock(semaforoTabla);
                pthread_mutex_unlock(semaforoTabla);
            } else {
                pthread_mutex_t *semaforoTabla = malloc(sizeof(pthread_mutex_t));
                int init = pthread_mutex_init(semaforoTabla, NULL);
                pthread_mutex_unlock(semaforoTabla);
                dictionary_put(tablasEnUso, nombreTabla, semaforoTabla);
                pthread_mutex_unlock(mutexTablasEnUso);
                pthread_mutex_lock(semaforoTabla);
                pthread_mutex_unlock(semaforoTabla);
            }
            // Obtener la metadata asociada a dicha tabla.
            char *path = obtenerPathMetadata(nombreTabla, configuracion.puntoMontaje);
            if (existeElArchivo(path)) {
                char *linea = armarLinea(key, valor, timestamp);
                t_dictionary *keysEnTabla;
                pthread_mutex_lock(mutexMemtable);
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
                sem_post(cantidadRegistrosMemtable);
                pthread_mutex_unlock(mutexMemtable);
                retorno->tipoRespuesta = RESPUESTA;
                retorno->valor = concat(1, "Se inserto el valor con exito.");
            } else {
                retorno->tipoRespuesta = ERR;
                retorno->valor = concat(5, "No se pudo insertar en ", nombreTabla, ". No existe metadata en ", path,
                                        ".");
            }
            free(path);
        } else {
            retorno->tipoRespuesta = ERR;
            retorno->valor = concat(3, "No existe la tabla ", nombreTabla, ".");
        }
    }
    return retorno;
}

void lfsInsertCompactacion(char *nombreTabla, char *key, char *valor, unsigned long long timestamp) {
    char *path = obtenerPathMetadata(nombreTabla, configuracion.puntoMontaje);
    if (existeElArchivo(path)) {
        char *linea = armarLinea(key, valor, timestamp);
        t_metadata *meta = obtenerMetadata(nombreTabla);
        int particion = calcularParticion(key, meta);
        char *nombreArchivo;
        char *p = string_itoa(particion);
        char *tablePath = obtenerPathTabla(nombreTabla, configuracion.puntoMontaje);
        nombreArchivo = concat(4, tablePath, "/", p, ".bin");
        escribirEnBloque(linea, nombreTabla, particion, nombreArchivo);
        free(path);
        free(tablePath);
        free(nombreArchivo);
    }
}

void escribirEnBloque(char *linea, char *nombreTabla, int particion, char *nombreArchivo) {
    // TODO: Insertar en la memoria temporal del punto anterior una nueva entrada que contenga los datos enviados en la request.
    int indice = 0;
    int bloque;
    while (linea[indice] != '\0' && indice < string_length(linea)) {
        //pthread_mutex_lock(mutexAsignacionBloques);
        bloque = obtenerBloqueDisponible(nombreTabla, particion);
        if (bloque == -1) {
            //pthread_mutex_unlock(mutexAsignacionBloques);
            //log_error(logger, "No hay bloques disponibles.");
            printf("No hay bloques disponibles.\n");
            fflush(stdout);
            deleteFile(nombreArchivo);
            break;
        } else {
            char *bloqueString = string_itoa(bloque);
            char *pathBloque = obtenerPathBloque(bloque);
            pthread_mutex_t *semBloque = (pthread_mutex_t *) obtenerSemaforoPath(pathBloque);
            pthread_mutex_lock(semBloque);
            FILE *f = fopen(pathBloque, "a");
            int longitudArchivo = obtenerTamanioBloque(bloque);
            while (longitudArchivo < metadataFS->block_size &&
                   indice < string_length(linea)) {
                fputc(linea[indice], f);
                longitudArchivo++;
                indice++;
            }
            //log_info(logger,"Se escribio en el archivo de bloque %s.bin",bloqueString);
            fclose(f);
            pthread_mutex_unlock(semBloque);
            free(bloqueString);
            free(pathBloque);
            if (obtenerTamanioBloque(bloque) >= metadataFS->block_size) {
                pthread_mutex_lock(mutexBitarray);
                bitarray_set_bit(bitarray, bloque);
                pthread_mutex_unlock(mutexBitarray);
            }
        }
    }
    if (particion == -1) {
        pthread_mutex_t *semArchivo = (pthread_mutex_t *) obtenerSemaforoPath(nombreArchivo);
        pthread_mutex_lock(semArchivo);
        FILE *fParticion = fopen(nombreArchivo, "w");

        char *bloquesAsignadosAParticion = obtenerBloquesAsignados(nombreTabla, particion);
        int cantidadBloquesAsignados = tamanioDeArrayDeStrings(
                convertirStringDeBloquesAArray(bloquesAsignadosAParticion));
        int tamanio = ((cantidadBloquesAsignados - 1) * metadataFS->block_size) +
                      obtenerTamanioBloque(bloque);
        char *tamanioString = string_itoa(tamanio);
        char *contenido = generarContenidoParaParticion(tamanioString, bloquesAsignadosAParticion);
        fwrite(contenido, sizeof(char) * strlen(contenido), 1, fParticion);
        //log_info(logger,"Se escribio en el archivo %s.",nombreArchivo);
        fclose(fParticion);
        pthread_mutex_unlock(semArchivo);
        free(contenido);
        free(bloquesAsignadosAParticion);
        free(tamanioString);
    }
}

int renombrarTemporales(char *nombreTabla) {
    int seRenombraron = 0;
    DIR *d;
    struct dirent *dir;
    char *pathTabla = obtenerPathTabla(nombreTabla, configuracion.puntoMontaje);

    d = opendir(pathTabla);
    if (d) {
        while ((dir = readdir(d)) != NULL) {
            char *nombreArchivo = string_duplicate(dir->d_name);
            if (string_ends_with(nombreArchivo, ".tmp")) {
                char *pathArchivo = obtenerPathArchivo(nombreTabla, nombreArchivo);
                char *newPathArchivo = string_duplicate(pathArchivo);
                string_append(&newPathArchivo, "c");
                pthread_mutex_t *semTmp = (pthread_mutex_t *) obtenerSemaforoPath(pathArchivo);
                pthread_mutex_lock(semTmp);
                if (rename(pathArchivo, newPathArchivo) == 0 && seRenombraron == 0) {
                    seRenombraron = 1;
                    //log_info(logger,"Se renombro el archivo %s a %sc.",nombreArchivo,nombreArchivo);
                }
                pthread_mutex_unlock(semTmp);
            }
            free(nombreArchivo);
        }
    }
    closedir(d);
    free(pathTabla);
    return seRenombraron;
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

bool ordenarPorLinea(char *linea, char *linea2) {
    char **lineaDesarmada = desarmarLinea(linea);
    char **linea2Desarmada = desarmarLinea(linea2);
    bool esMenor = atoi(lineaDesarmada[0]) < atoi(linea2Desarmada[0]);
    return esMenor;
}

t_response *lfsSelect(char *nombreTabla, char *key) {
    t_response *retorno = (t_response *) malloc(sizeof(t_response));

    //1. Verificar que la tabla exista en el File System
    if (existeTabla(nombreTabla) == 0) {
//        if (dictionary_has_key(tablasEnUso, nombreTabla)) {
//            pthread_mutex_t *semaforoTabla = dictionary_get(tablasEnUso, nombreTabla);
//            pthread_mutex_lock(semaforoTabla);
//            pthread_mutex_unlock(semaforoTabla);
//        } else {
//            pthread_mutex_t *semaforoTabla = malloc(sizeof(pthread_mutex_t));
//            int init = pthread_mutex_init(semaforoTabla, NULL);
//            pthread_mutex_unlock(semaforoTabla);
//            dictionary_put(tablasEnUso, nombreTabla, semaforoTabla);
//            pthread_mutex_lock(semaforoTabla);
//            pthread_mutex_unlock(semaforoTabla);
//        }

        char *path = obtenerPathMetadata(nombreTabla, configuracion.puntoMontaje);
        if (existeElArchivo(path)) {

            //2. Obtener la metadata asociada a dicha tabla
            t_metadata *meta = obtenerMetadata(nombreTabla);

            //3. Calcular cual es la partición que contiene dicho KEY
            int particion = calcularParticion(key, meta);

            //4. Obtengo los bloques de la particion
            char *nombreArchivoParticion = obtenerNombreArchivoParticion(particion);
            char *bloquesParticion = obtenerStringBloquesDeArchivo(nombreTabla, nombreArchivoParticion);

            //5. Obtengo los bloques de los archivos temporales
            char *bloquesTemporales = obtenerStringBloquesSegunExtension(nombreTabla, ".tmp");

            //6. Obtengo los bloques de los archivos temporales que se están compactando
            char *bloquesTemporalesCompactacion = obtenerStringBloquesSegunExtension(nombreTabla, ".tmpc");

            char *todosLosBloques = string_new();
            string_append(&todosLosBloques, bloquesParticion);
            if (!string_is_empty(bloquesTemporales)) {
                string_append(&todosLosBloques, ",");
                string_append(&todosLosBloques, bloquesTemporales);
            }
            if (!string_is_empty(bloquesTemporalesCompactacion)) {
                string_append(&todosLosBloques, ",");
                string_append(&todosLosBloques, bloquesTemporalesCompactacion);
            }

            char **bloques = convertirStringDeBloquesAArray(todosLosBloques);
            char *mayorLinea = string_duplicate(obtenerLineaMasReciente(bloques, key));

            //7. Escaneo la memoria temporal de la tabla
            pthread_mutex_lock(mutexMemtable);
            if (dictionary_has_key(memTable, nombreTabla)) {
                t_dictionary *tabla = dictionary_get(memTable, nombreTabla);
                if (dictionary_has_key(tabla, key)) {
                    t_list *listaDeRegistros = (t_list *) dictionary_get(tabla, key);
                    if (!list_is_empty(listaDeRegistros)) {
                        int tamanioLista = list_size(listaDeRegistros);
                        t_list *listaOrdenada = list_sorted(listaDeRegistros, (void *)ordenarPorLinea);
                        char *elemento = list_get(listaOrdenada, (tamanioLista - 1));
                        if (elemento != NULL) {
                            char **lineaPartida = desarmarLinea(elemento);
                            char **lineaPartida2 = desarmarLinea(mayorLinea);
                            int mayorTimestampMem = atoi(lineaPartida[0]);
                            if (lineaPartida2[0]) {
                                int mayorTimestampBlock = atoi(lineaPartida2[0]);
                                if (mayorTimestampMem > mayorTimestampBlock) {
                                    mayorLinea = elemento;
                                }
                            } else {
                                mayorLinea = elemento;
                            }
                        }
                    }
                }
            }
            pthread_mutex_unlock(mutexMemtable);

            //8. Encontradas las entradas para dicha Key, se retorna el valor con el Timestamp más grande
            if (strcmp(mayorLinea, "") != 0) {
                free(path);
                retorno->tipoRespuesta = RESPUESTA;
                retorno->valor = string_duplicate(mayorLinea);
                return retorno;
            } else {
                free(path);
                retorno->tipoRespuesta = ERR;
                retorno->valor = concat(5, "No se encontro ningun valor en la tabla ",nombreTabla," con la key ",key,".");
                return retorno;
            }
        } else {
            free(path);
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

char *obtenerLineaMasReciente(char **bloques, char *key) {
    char *blockPath = NULL;
    bool lineaContinuaEnOtroArchivo = false;
    char *linea = string_new();
    char *keyEncontrado = string_new();
    char *timestampEncontrado = string_new();
    char seek;
    char str[2];
    str[1] = '\0';
    char **palabras = NULL;
    char *mayorLinea = string_new();
    int mayorTimestamp = 0;
    int tamanioArray = tamanioDeArrayDeStrings(bloques);

    for (int i = 0; i < tamanioArray; i++) {
        blockPath = obtenerPathBloque(atoi(bloques[i]));
        pthread_mutex_t *semBloque = (pthread_mutex_t *) obtenerSemaforoPath(blockPath);
        pthread_mutex_lock(semBloque);

        FILE *binarioBloque = fopen(blockPath, "r");

        while (!feof(binarioBloque)) {
            if (!lineaContinuaEnOtroArchivo) {
                vaciarString(&linea);
            }
            vaciarString(&keyEncontrado);
            vaciarString(&timestampEncontrado);
            while ((seek = getc(binarioBloque)) != EOF && seek != '\n') {
                str[0] = seek;
                string_append(&linea, str);
            }
            if (strcmp(linea, "") != 0) {
                palabras = desarmarLinea(linea);
                if (tamanioDeArrayDeStrings(palabras) == 3 && seek == '\n') { // Si la línea no continua en otro bloque
                    lineaContinuaEnOtroArchivo = false;
                    string_append(&timestampEncontrado, palabras[0]);
                    string_append(&keyEncontrado, palabras[1]);
                    if (strcmp(keyEncontrado, key) == 0 && (atoi(timestampEncontrado) >= mayorTimestamp)) {
                        mayorTimestamp = atoi(timestampEncontrado);
                        vaciarString(&mayorLinea);
                        string_append(&mayorLinea, linea);
                    }
                } else {
                    lineaContinuaEnOtroArchivo = true;
                }
                if (palabras != NULL) freeArrayDeStrings(palabras);
            }
        }
        fclose(binarioBloque);
        pthread_mutex_unlock(semBloque);
        vaciarString(&blockPath);
    }
    free(linea);
    free(keyEncontrado);
    free(timestampEncontrado);
    if (blockPath != NULL) free(blockPath);
    return mayorLinea;
}


char *obtenerNombreArchivoParticion(int particion) {
    char *nombreArchivo = string_new();
    string_append(&nombreArchivo, string_itoa(particion));
    string_append(&nombreArchivo, ".bin");
    return nombreArchivo;
}

char *obtenerStringBloquesDeArchivo(char *nombreTabla, char *nombreArchivo) {
    char *path = obtenerPathArchivo(nombreTabla, nombreArchivo);
    if (existeElArchivo(path)) {
        pthread_mutex_t *semArchivo = (pthread_mutex_t *) obtenerSemaforoPath(path);
        pthread_mutex_lock(semArchivo);
        t_config *archivoConfig = abrirArchivoConfiguracion(path, logger);
        char *valoresBloques = config_get_string_value(archivoConfig, "BLOCKS");
        char *arrayDeBloques = string_duplicate(valoresBloques);
        config_destroy(archivoConfig);
        pthread_mutex_unlock(semArchivo);
        eliminarCharDeString(arrayDeBloques, '[');
        eliminarCharDeString(arrayDeBloques, ']');
        free(path);
        return arrayDeBloques;
    } else {
        free(path);
        return "";
    }
}

void eliminarCharDeString(char *string, char ch) {
    char *src, *dst;
    for (src = dst = string; *src != '\0'; src++) {
        *dst = *src;
        if (*dst != ch) dst++;
    }
    *dst = '\0';
}

char **convertirStringDeBloquesAArray(char *bloques) {
    char *stringArray = string_new();
    string_append(&stringArray, "[");
    string_append(&stringArray, bloques);
    string_append(&stringArray, "]");
    char **arrayDeBloques = string_get_string_as_array(stringArray);
    free(stringArray);
    return arrayDeBloques;
}

char *obtenerStringBloquesSegunExtension(char *nombreTabla, char *ext) {
    int nroTemporal = 0;
    char *bloques = string_new();
    char *archivoTemporalPath;
    char *nombreArchivoTemporal;
    if (string_equals_ignore_case(ext, ".tmpc") || string_equals_ignore_case(ext, ".tmp")) {
        nombreArchivoTemporal = string_from_format("%s%d%s", nombreTabla, nroTemporal, ext);
    } else {
        nombreArchivoTemporal = string_from_format("%d%s", nroTemporal, ext);
    }
    archivoTemporalPath = obtenerPathArchivo(nombreTabla, nombreArchivoTemporal);
    while (existeElArchivo(archivoTemporalPath)) {
        free(archivoTemporalPath);
        char *bloquesDeArchivo = obtenerStringBloquesDeArchivo(nombreTabla, nombreArchivoTemporal);
        string_append(&bloques, bloquesDeArchivo);
        string_append(&bloques, ",");
        nroTemporal++;
        vaciarString(&nombreArchivoTemporal);
        if (strcmp(ext, ".tmp") == 0 || strcmp(ext, ".tmpc") == 0) {
            string_append(&nombreArchivoTemporal, nombreTabla);
        }
        string_append(&nombreArchivoTemporal, string_itoa(nroTemporal));
        string_append(&nombreArchivoTemporal, ext);
        archivoTemporalPath = obtenerPathArchivo(nombreTabla, nombreArchivoTemporal);
        free(bloquesDeArchivo);
    }
    free(archivoTemporalPath);
    if (!string_is_empty(bloques)) {
        bloques = string_substring_until(bloques, strlen(bloques) - 1);
    }
    return bloques;
}

void ejecutarConsola() {
    t_comando comando;

    do {
        char *leido = readline("Lissandra@suck-ets:~$ ");
        char *request = string_duplicate(leido);
        char **comandoParseado = parser(leido);
        if (comandoParseado == NULL) {
            free(comandoParseado);
            continue;
        }
        comando = instanciarComando(comandoParseado);
        if (validarComandosComunes(comando, logger)) {
            t_response *retorno = gestionarRequest(comando);
            loguearRespuesta(request, retorno);
            if (!string_ends_with(retorno->valor, "\n")) {
                string_append(&retorno->valor, "\n");
            }
            printf("%s", retorno->valor);
            fflush(stdout);
            free(retorno->valor);
            free(retorno);
            rl_clear_history();
        } else {
            printf("Alguno de los parámetros ingresados es incorrecto. Por favor verifique su entrada.\n");
            fflush(stdout);
        }

        free(request);
    } while (comando.tipoRequest != EXIT);
    rl_clear_history();
    printf("Ya se analizo todo lo solicitado.\n");
    fflush(stdout);
}


void loguearRespuesta(char *request, t_response *retorno) {
    if (retorno->tipoRespuesta == ERR) {
        log_warning(logger, "REQUEST: %s. \t RESPUESTA: %s", request, retorno->valor);
    } else {

        log_info(logger, "REQUEST: %s. \t RESPUESTA: %s", request, retorno->valor);
    }
}

t_response *gestionarRequest(t_comando comando) {
    retardo();
    t_response *retorno;
    if (!metadataFS) {
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
            unsigned long long timestamp;
            // El parámetro Timestamp es opcional.
            // En caso que un request no lo provea (por ejemplo insertando un valor desde la consola),
            // se usará el valor actual del Epoch UNIX.
            if (comando.cantidadParametros == 4 && comando.parametros[3] != NULL) {
                string_trim(&comando.parametros[3]);
                timestamp = (unsigned long long) strtol(comando.parametros[3], NULL, 10);
            } else {
                timestamp = getCurrentTime();
            }
            //printf("Timestamp: %i\n", (int) timestamp);
            //fflush(stdout);
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
            retorno = lfsDrop(comando.parametros[0]);
            return retorno;

        case HELP:
            printf("************ Comandos disponibles ************\n");
            printf("- SELECT [NOMBRE_TABLA] [KEY]\n");
            printf("- INSERT [NOMBRE_TABLA] [KEY] “[VALUE]” [Timestamp](Opcional)\n");
            printf("- CREATE [NOMBRE_TABLA] [TIPO_CONSISTENCIA] [NUMERO_PARTICIONES] [COMPACTION_TIME]\n");
            printf("- DESCRIBE [NOMBRE_TABLA](Opcional)\n");
            printf("- DROP [NOMBRE_TABLA]\n");
            printf("- EXIT\n");
            fflush(stdout);
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

t_metadata *obtenerMetadata(char *tabla) {
    char *metadataPath = obtenerPathMetadata(tabla, configuracion.puntoMontaje);
    t_config *config = abrirArchivoConfiguracion(metadataPath, logger);

    if (config == NULL) {
        char *pathTabla = obtenerPathTabla(tabla, configuracion.puntoMontaje);
        //log_error(logger, "No se pudo obtener el archivo Metadata de la tabla %s", tabla);
        borrarContenidoDeDirectorio(pathTabla);
        if (rmdir(pathTabla) != 0) {
            printf("El File System se esta restaurando a un estado consistente. Reinicie el proceso.");
            fflush(stdout);
            return NULL;
        }
    } else {

        t_metadata *metadata = (t_metadata *) malloc(sizeof(t_metadata));

        if (config_has_property(config, "PARTITIONS")) {
            metadata->partitions = config_get_int_value(config, "PARTITIONS");
        }

        if (config_has_property(config, "CONSISTENCY")) {
            metadata->consistency = string_duplicate(config_get_string_value(config, "CONSISTENCY"));
        }

        if (config_has_property(config, "COMPACTION_TIME")) {
            metadata->compaction_time = config_get_int_value(config, "COMPACTION_TIME");
        }
        pthread_mutex_lock(mutexMetadatas);
        dictionary_put(metadatas, tabla, metadata);
        pthread_mutex_unlock(mutexMetadatas);
        free(metadataPath);
        config_destroy(config);

        return (t_metadata *) metadata;
    }
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
    pthread_detach(*hiloConexiones);

    return hiloConexiones;
}

pthread_t *crearHiloRequest(Header header, char *mensaje) {
    pthread_t *hiloRequest = (pthread_t *) malloc(sizeof(pthread_t));

    parametros_thread_request *parametros = (parametros_thread_request *) malloc(sizeof(parametros_thread_request));

    parametros->header = header;
    parametros->mensaje = string_duplicate(mensaje);
    pthread_create(hiloRequest, NULL, &atenderMensajes, parametros);
    pthread_detach(*hiloRequest);

    return hiloRequest;
}

pthread_t *crearHiloDump(t_log *logger) {
    pthread_t *hiloConexiones = (pthread_t *) malloc(sizeof(pthread_t));

    parametros_thread_lfs *parametros = (parametros_thread_lfs *) malloc(sizeof(parametros_thread_lfs));
    parametros->logger = logger;

    pthread_create(hiloConexiones, NULL, &lfsDump, parametros);
    pthread_detach(*hiloConexiones);

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
    char *nombreArchivo = concat(2, puntoMontaje, "Tables");

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

    }
    tablas[count] = NULL;

    free(nombreArchivo);
    return tablas;
}

void cargarBloquesAsignados(char *path) {
    char **tablas = obtenerTablas();
    int temporalesConflictivos = 0;
    for (int i = 0; i < tamanioDeArrayDeStrings(tablas); i++) {
        char *nombreTabla = string_duplicate(tablas[i]);
        t_metadata *meta = obtenerMetadata(nombreTabla);
        if (meta == NULL) {
            free(nombreTabla);
            continue;
        }
        pthread_mutex_t *semaforoTabla = malloc(sizeof(pthread_mutex_t));
        int init = pthread_mutex_init(semaforoTabla, NULL);
        pthread_mutex_lock(mutexTablasEnUso);
        dictionary_put(tablasEnUso, nombreTabla, semaforoTabla);
        pthread_mutex_unlock(mutexTablasEnUso);
        char *compactationTimeString = string_itoa(meta->compaction_time);
        pthread_t *hilo = crearHiloCompactacion(nombreTabla, compactationTimeString);
        free(compactationTimeString);
        pthread_detach(*hilo);
        pthread_mutex_lock(mutexHilosTablas);
        dictionary_put(hilosTablas, nombreTabla, hilo);
        pthread_mutex_unlock(mutexHilosTablas);
        char *pathTabla = concat(3, path, "/", nombreTabla);
        DIR *d;
        struct dirent *dir;
        d = opendir(pathTabla);
        if (d) {
            while ((dir = readdir(d)) != NULL) {
                if (strcmp(dir->d_name, ".") != 0 && strcmp(dir->d_name, "..") != 0 &&
                    strcmp(dir->d_name, "Metadata") != 0) {
                    char *nombreArchivo = string_duplicate(dir->d_name);
                    int particion;
                    if (string_ends_with(nombreArchivo, ".bin")) {
                        char **numeroParticion = string_split(nombreArchivo, ".");
                        particion = atoi(numeroParticion[0]);
                        freeArrayDeStrings(numeroParticion);
                        char *pathArchivo = concat(3, pathTabla, "/", nombreArchivo);
                        if (!archivoVacio(pathArchivo)) {
                            char *bloquesArchivo = obtenerStringBloquesDeArchivo(nombreTabla, nombreArchivo);
                            char **arrayDeBloques = convertirStringDeBloquesAArray(bloquesArchivo);
                            for (int j = 0; j < tamanioDeArrayDeStrings(arrayDeBloques); j++) {
                                t_bloqueAsignado *bloque = (t_bloqueAsignado *) malloc(sizeof(t_bloqueAsignado));
                                bloque->tabla = string_duplicate(nombreTabla);
                                bloque->particion = particion;
                                pthread_mutex_lock(mutexAsignacionBloques);
                                dictionary_put(bloquesAsignados, arrayDeBloques[j], bloque);
                                pthread_mutex_unlock(mutexAsignacionBloques);
                            }
                            freeArrayDeStrings(arrayDeBloques);
                            free(bloquesArchivo);
                        } else {
                            if (sePuedeEscribirElArchivo(pathArchivo)) {
                                char *bloqueDisponible = string_itoa(obtenerBloqueDisponible(nombreTabla, particion));
                                char *contenido = generarContenidoParaParticion("0", bloqueDisponible);
                                pthread_mutex_t *semArchivo = (pthread_mutex_t *) obtenerSemaforoPath(pathArchivo);
                                pthread_mutex_lock(semArchivo);
                                FILE *f = fopen(pathArchivo, "w");
                                fwrite(contenido, sizeof(char), strlen(contenido), f);
                                fclose(f);
                                pthread_mutex_unlock(semArchivo);
                                free(contenido);
                                free(bloqueDisponible);
                            } else {
                                //log_error(logger, "No se pudo escribir en la particion %i", particion);
                            }
                        }
                        free(pathArchivo);
                    } else {
                        particion = -1;
                        if (temporalesConflictivos == 0) {
                            log_warning(logger,
                                        "Se detectó que una compactación no pudo terminar su ejecución correctamente. Se procede a restaurar el File System a un estado consistente.");
                        }
                        char *pathArchivo = concat(3, pathTabla, "/", nombreArchivo);
                        if (archivoVacio(pathArchivo)) {
                            deleteFile(pathArchivo);
                        }
                        free(pathArchivo);
                    }
                    free(nombreArchivo);
                }
            }
        }
        closedir(d);
        free(pathTabla);
        free(nombreTabla);
    }
    freeArrayDeStrings(tablas);
}

int obtenerCantidadBloques(char *puntoMontaje) {
    if (metadataFS->blocks) {
        int cantidadDeBloques = metadataFS->blocks;
        return cantidadDeBloques;
    }
    return -1;
}

int existeMetadata() {
    char *nombreArchivo = string_new();
    string_append(&nombreArchivo, configuracion.puntoMontaje);
    string_append(&nombreArchivo, "Metadata/Metadata.bin");
    int existe = existeElArchivo(nombreArchivo);
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
        for (int i = 0; i < bloques; i++) {
            char *nroBloque = string_itoa(i);
            char *unArchivoDeBloque = concat(4, nombreArchivo, "/", nroBloque, ".bin");
            t_bloqueAsignado *bloque = (t_bloqueAsignado *) malloc(sizeof(t_bloqueAsignado));
            bloque->tabla = string_new();
            bloque->particion = -1;
            pthread_mutex_lock(mutexAsignacionBloques);
            dictionary_put(bloquesAsignados, nroBloque, bloque);
            pthread_mutex_unlock(mutexAsignacionBloques);
            free(nroBloque);
            if (!existeElArchivo(unArchivoDeBloque)) {
                pthread_mutex_t *semBloque = (pthread_mutex_t *) obtenerSemaforoPath(unArchivoDeBloque);
                pthread_mutex_lock(semBloque);
                FILE *file = fopen(unArchivoDeBloque, "w");
                fclose(file);
                pthread_mutex_unlock(semBloque);
            } else if (!archivoVacio(unArchivoDeBloque) &&
                       obtenerTamanioBloque(i) >= metadataFS->block_size) {
                pthread_mutex_lock(mutexBitarray);
                bitarray_set_bit(bitarray, i);
                pthread_mutex_unlock(mutexBitarray);
            }
            free(unArchivoDeBloque);
            //printf("%i", bitarray_test_bit(bitarray, i));
            //fflush(stdout);
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
        pthread_mutex_t *semMetadata = (pthread_mutex_t *) obtenerSemaforoPath(pathArchivoMetadata);
        pthread_mutex_lock(semMetadata);
        FILE *fm = fopen(pathArchivoMetadata, "w");
        fclose(fm);
        pthread_mutex_unlock(semMetadata);
    }
    if (!existeElArchivo(pathArchivoBitmap)) {
        pthread_mutex_t *semBitmap = (pthread_mutex_t *) obtenerSemaforoPath(pathArchivoBitmap);
        pthread_mutex_lock(semBitmap);
        FILE *fb = fopen(pathArchivoBitmap, "w");
        fclose(fb);
        pthread_mutex_unlock(semBitmap);
    }
    free(nombreArchivo);
    free(pathArchivoMetadata);
    free(pathArchivoBitmap);
}

void *obtenerSemaforoPath(char *path) {
    pthread_mutex_lock(mutexArchivosAbiertos);
    if (dictionary_has_key(archivosAbiertos, path)) {
        pthread_mutex_t *semaforoArchivo = dictionary_get(archivosAbiertos, path);
        pthread_mutex_unlock(mutexArchivosAbiertos);
        return semaforoArchivo;
    } else {
        pthread_mutex_t *semaforoArchivo = malloc(sizeof(pthread_mutex_t));
        int init = pthread_mutex_init(semaforoArchivo, NULL);
        dictionary_put(archivosAbiertos, path, semaforoArchivo);
        pthread_mutex_unlock(mutexArchivosAbiertos);
        return semaforoArchivo;
    }
}

int crearDirectoriosBase(char *puntoMontaje) {
    crearDirMetadata(puntoMontaje);
    cargarMetadataFS();
    inicializarBitmap();
    crearDirBloques(puntoMontaje);
    crearDirTables(puntoMontaje);
}

void cargarMetadataFS() {
    if (existeMetadata()) {

        bool error = false;
        if (!metadataFS) {
            char *nombreArchivo = string_new();
            string_append(&nombreArchivo, configuracion.puntoMontaje);
            string_append(&nombreArchivo, "Metadata/Metadata.bin");
            if (!archivoVacio(nombreArchivo)) {
                t_config *archivoConfig = abrirArchivoConfiguracion(nombreArchivo, logger);
                int tamanioBloques = config_get_int_value(archivoConfig, "BLOCK_SIZE");
                int cantidadBloques = config_get_int_value(archivoConfig, "BLOCKS");
                if (!tamanioBloques || !cantidadBloques) {
                    error = true;
                } else {
                    metadataFS = (t_metadata_fs *) malloc(sizeof(t_metadata_fs));
                    metadataFS->block_size = tamanioBloques;
                    metadataFS->blocks = cantidadBloques;
                }
                free(nombreArchivo);
                config_destroy(archivoConfig);
            } else {
                error = true;
            }
            if (error) {
                printf("La Metadata del File System debe ser seteada.\n");
                fflush(stdout);
                log_error(logger, "La Metadata del File System debe ser seteada.\n");
                exit(-1);
            }
        }

    }
}

void inicializarLFS(char *puntoMontaje) {
    crearDirectoriosBase(puntoMontaje);
}

void atenderHandshake(Header header, Componente componente, parametros_thread_lfs *parametros) {
    if (componente == MEMORIA) {
        char *tamanioValue = string_itoa(parametros->tamanioValue);
        enviarPaquete(header.fdRemitente, HANDSHAKE, INVALIDO, tamanioValue, header.pid);
        free(tamanioValue);
    }
}

void inicializarBitmap() {
    char *bitmapPath = concat(2, configuracion.puntoMontaje, "Metadata/Bitmap.bin");
    int fd = open(bitmapPath, O_RDWR);
    int tamanioBitarray = metadataFS->blocks / 8;
    if (lseek(fd, 0, SEEK_END) <= 0) {
        ftruncate(fd, tamanioBitarray);
    }
    pthread_mutex_lock(mutexBitarray);
    bitmap = mmap(NULL, tamanioBitarray, PROT_WRITE | PROT_READ, MAP_SHARED, fd, 0);
    bitarray = bitarray_create_with_mode(bitmap, tamanioBitarray, MSB_FIRST);
    pthread_mutex_unlock(mutexBitarray);
    free(bitmapPath);
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
                    int bytesRecibidos = recv(fdConectado, &headerSerializado, sizeof(Header), MSG_WAITALL);

                    switch (bytesRecibidos) {
                        // hubo un error al recibir los datos
                        case -1:
                            desconectarCliente(fdConectado, unaConexion, logger);
                            break;
                            // se desconectó
//                        case 0:
                            // acá cada uno setea una maravillosa función que hace cada uno cuando se le desconecta alguien
                            // nombre_maravillosa_funcion();
//                            desconectarCliente(fdConectado, unaConexion, logger);
//                            break;
                            // recibí algún mensaje
                        default:; // esto es lo más raro que vi pero tuve que hacerlo
                            Header header = deserializarHeader(&headerSerializado);
                            header.fdRemitente = fdConectado;
                            int pesoMensaje = header.tamanioMensaje * sizeof(char);
                            void *mensaje = (void *) malloc(pesoMensaje);
                            bytesRecibidos = recv(fdConectado, mensaje, pesoMensaje, MSG_WAITALL);
                            if (bytesRecibidos == -1 || bytesRecibidos < pesoMensaje)
                                desconectarCliente(fdConectado, unaConexion, logger);
                            else {
                                // acá cada uno setea una maravillosa función que hace cada uno cuando le llega un nuevo mensaje
                                // nombre_maravillosa_funcion();
                                if (header.tipoMensaje == HANDSHAKE) {
                                    Componente componente = (Componente) atoi(mensaje);
                                    atenderHandshake(header, componente, parametros);
                                } else {
                                    pthread_t *hiloRequest = crearHiloRequest(header, mensaje);
                                }
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


int main(int argc, char* argv[]) {
    char *nombrePruebaDebug = string_duplicate("prueba-lfs");
    char *rutaConfig = string_from_format("../../pruebas/%s/lfs/lfs.cfg", nombrePruebaDebug); //Para debuggear
    //char *rutaConfig = string_from_format("../pruebas/%s/lfs/lfs.cfg", argv[1]); //Para ejecutar
    char *rutaLogger = string_from_format("%s.log", nombrePruebaDebug); //Para debuggear
    //char *rutaLogger = string_from_format("%s.log", argv[1]); //Para ejecutar

    logger = log_create(rutaLogger, "Lissandra", false, LOG_LEVEL_INFO);

    log_info(logger, "Iniciando Lissandra File System");

    configuracion = cargarConfiguracion(rutaConfig, logger);
    metadatas = dictionary_create();
    archivosAbiertos = dictionary_create();
    memTable = dictionary_create();
    tablasEnUso = dictionary_create();
    hilosTablas = dictionary_create();
    bloquesAsignados = dictionary_create();
    mutexAsignacionBloques = malloc(sizeof(pthread_mutex_t));
    mutexMemtable = malloc(sizeof(pthread_mutex_t));
    mutexMetadatas = malloc(sizeof(pthread_mutex_t));
    mutexArchivosAbiertos = malloc(sizeof(pthread_mutex_t));
    mutexTablasEnUso = malloc(sizeof(pthread_mutex_t));
    mutexHilosTablas = malloc(sizeof(pthread_mutex_t));
    mutexBitarray = malloc(sizeof(pthread_mutex_t));
    cantidadRegistrosMemtable = malloc(sizeof(sem_t));
    int init = pthread_mutex_init(mutexAsignacionBloques, NULL);
    int init1 = pthread_mutex_init(mutexMemtable, NULL);
    int init2 = pthread_mutex_init(mutexMetadatas, NULL);
    int init3 = pthread_mutex_init(mutexArchivosAbiertos, NULL);
    int init4 = pthread_mutex_init(mutexTablasEnUso, NULL);
    int init5 = pthread_mutex_init(mutexHilosTablas, NULL);
    int init6 = pthread_mutex_init(mutexBitarray, NULL);
    sem_init(cantidadRegistrosMemtable, 0, 0);
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

    pthread_join(*hiloConexiones, NULL);

    free(nombrePruebaDebug); //TODO: Si se esta ejecutando comentar esta linea
    free(rutaConfig);
    free(rutaLogger);
    free(misConexiones);
    free(bloquesAsignados);
    free(metadatas);
    free(hiloDump);
    return 0;
}