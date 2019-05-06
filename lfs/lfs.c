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
        configuracion.puntoMontaje = config_get_string_value(archivoConfig, "PUNTO_MONTAJE");
        configuracion.retardo = config_get_int_value(archivoConfig, "RETARDO");
        configuracion.tamanioValue = config_get_int_value(archivoConfig, "TAMAÑO_VALUE");
        configuracion.tiempoDump = config_get_int_value(archivoConfig, "TIEMPO_DUMP");

        return configuracion;
    }
}

void atenderMensajes(Header header, char *mensaje) {
    enviarPaquete(header.fdRemitente, "Hola, soy Lissandra");
    printf("Estoy recibiendo un mensaje del File Descriptor %i: %s", header.fdRemitente, mensaje);
    fflush(stdout);
}

char *obtenerPathArchivo(char *nombreTabla, char *nombreArchivo) {
    char *path = string_new();
    char *tablePath = obtenerPathTabla(nombreTabla);
    string_append(&path, tablePath);
    string_append(&path, "/");
    string_append(&path, nombreArchivo);
    return path;
}

char *armarLinea(char *key, char *valor, time_t timestamp) {
    char *linea = string_new();
    char *timestampString;
    sprintf(timestampString, "%ld", timestamp);
    string_append(&linea, timestampString);
    string_append(&linea, ";");
    string_append(&linea, key);
    string_append(&linea, ";");
    string_append(&linea, valor);
    string_append(&linea, "\n");
    return linea;
}

char **desarmarLinea(char *linea) {
    return string_split(linea, ";");
}

void lfsInsert(char *nombreTabla, char *key, char *valor, time_t timestamp) {
    if (existeTabla(nombreTabla)) {
        char *path = obtenerPathMetadata(nombreTabla);
        if (existeElArchivo(path)) {
            printf("Existe metadata en %s\n", path);
        } else {
            printf("No existe metadata en %s\n", path);
        }
        char *linea = armarLinea(key, valor, timestamp);
        FILE *f = fopen(obtenerPathArchivo(nombreTabla, "1.bin"), "a");
        fwrite(linea, sizeof(char *), sizeof(linea), f);
        fclose(f);
        free(path);
    }
}

void lfsSelect(char* nombreTabla, char* key){
    char ch;

    //1. Verificar que la tabla exista en el File System
    existeTabla(nombreTabla);

    //2. Obtener la metadata asociada a dicha tabla
    obtenerMetadata(nombreTabla);
    t_metadata* meta = dictionary_get(metadatas, nombreTabla);

    //3. Calcular cual es la partición que contiene dicho KEY
    int particion = calcularParticion(key, meta);

    //4. Escanear la partición objetivo, todos los archivos temporales y la memoria temporal de dicha tabla (si existe) buscando la key deseada
    char *nombreArchivo = string_new();
    char* p = string_itoa(particion);
    string_append(&nombreArchivo, p);
    string_append(&nombreArchivo, ".bin");
    char *archivePath = obtenerPathArchivo(nombreTabla, nombreArchivo);
    FILE *fd = fopen(archivePath, "r");
    char *contenido = string_new();
    string_append(&contenido, "El contenido de ");
    string_append(&contenido, archivePath);
    string_append(&contenido, " es:");
    log_info(logger, contenido);

    int i = 0;
    char *linea = string_new();
    char str[2];
    str[1] = '\0';
    char **entradas;
    while(!feof(fd)) {
        while((ch = fgetc(fd)) != '\r') {
            str[0] = ch;
            string_append(&linea, str);
            printf("%c", ch);
        }
        char **dato = desarmarLinea(linea);
        if(dato[1] == key){
            entradas[i] = armarLinea(dato[0], dato[1], (time_t) dato[2]);
            i++;
        }
    }

    fclose(fd);

    log_info(logger, entradas[0]);

    //5. Encontradas las entradas para dicha Key, se retorna el valor con el Timestamp más grande
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

    } else {
        printf("Ingrese un comando valido.");
        return -2;
    }

}

int existeTabla(char *tabla) {
    char *tablePath = obtenerPathTabla(tabla);
    if (!existeElArchivo(tablePath)){
        log_error(logger, "No se encontro o no tiene permisos para acceder a la tabla %s", tabla);
        return -1;
    }
    return 0;
}

void obtenerMetadata(char* tabla) {
    char *metadataPath = obtenerPathMetadata(tabla);
    t_config* config = config_create(metadataPath);
    t_metadata metadata;

    if(config == NULL) {
        log_error(logger, "No se pudo obtener el archivo Metadata.");
        exit(1);
    }

    if (config_has_property(config, "BLOCK_SIZE")) {
        metadata.block_size = config_get_int_value(config, "BLOCK_SIZE");
    }

    if (config_has_property(config, "BLOCKS")) {
        metadata.blocks = config_get_int_value(config, "BLOCKS");
    }

    if (config_has_property(config, "MAGIC_NUMBER")) {
        char* magic_num = config_get_string_value(config, "MAGIC_NUMBER");
        metadata.magic_number = malloc(strlen(magic_num) + 1);
        memcpy(metadata.magic_number, magic_num, strlen(magic_num) + 1);
    }

    dictionary_put(metadatas, tabla, &metadata);
    config_destroy(config);
}

int calcularParticion(char* key, t_metadata* metadata) {
    int k = atoi(key);
    int b = metadata->blocks;
    return k % b;
}


int main(void) {
    logger = log_create("../lfs.log", "lfs", true, LOG_LEVEL_INFO);

    log_info(logger, "Iniciando proceso Lissandra File System");

    configuracion = cargarConfiguracion("../lfs.cfg", logger);
    metadatas = dictionary_create();

    log_info(logger, "Puerto Escucha: %i", configuracion.puertoEscucha);
    log_info(logger, "Punto Montaje: %s", configuracion.puntoMontaje);
    log_info(logger, "Retardo: %i", configuracion.retardo);
    log_info(logger, "Tamaño value: %i", configuracion.tamanioValue);
    log_info(logger, "Tiempo dump: %i", configuracion.tiempoDump);

    ejecutarConsola(&gestionarRequest, "lissandra", logger);
    // crearHiloServidor(configuracion.puertoEscucha, &atenderMensajes, NULL, NULL);
    //int cliente = crearSocketCliente("192.168.0.30", 8000);

    while (1);

    return 0;
}