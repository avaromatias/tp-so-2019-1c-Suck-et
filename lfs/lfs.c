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
void insert(char* nombreTabla,char* key, char*valor,char* nuevoTimestamp){
    printf("Tabla: %s\n", nombreTabla);
    printf("Key: %s\n", key);
    printf("Valor: %s\n", valor);
    time_t timestamp;
    if(nuevoTimestamp){
        timestamp=(time_t)time(nuevoTimestamp);
    }else{
        timestamp=(time_t)time(NULL);
    }
    printf("Timestamp: %i\n", (int)timestamp);
}
void gestionarRequest(char **request) {
    char *tipoDeRequest = request[0];
    char *nombreTabla = request[1];
    char *param1 = request[2];
    char *param2 = request[3];
    char *param3 = request[4];
    printf("Interpretando request...\n");
    string_to_upper(tipoDeRequest);

    if (strcmp(tipoDeRequest,"SELECT")==0) {
        printf("Tipo de Request: %s\n", tipoDeRequest);
        printf("Tabla: %s\n", nombreTabla);
        printf("Key: %s\n", param1);
    } else if (strcmp(tipoDeRequest,"INSERT")==0) {
        printf("Tipo de Request: %s\n", tipoDeRequest);
        insert(nombreTabla,param1,param2,param3);

    } else if (strcmp(tipoDeRequest,"CREATE")==0) {
        printf("Tipo de Request: %s\n", tipoDeRequest);

    } else if (strcmp(tipoDeRequest,"DESCRIBE")==0) {
        printf("Tipo de Request: %s\n", tipoDeRequest);

    } else if (strcmp(tipoDeRequest,"DROP")==0) {
        printf("Tipo de Request: %s\n", tipoDeRequest);
    }

}


int main(void) {
    t_log *logger = log_create("../lfs.log", "lfs", true, LOG_LEVEL_INFO);

    log_info(logger, "Iniciando proceso Lissandra File System");

    configuracion = cargarConfiguracion("../lfs.cfg", logger);

    log_info(logger, "Puerto Escucha: %i", configuracion.puertoEscucha);
    log_info(logger, "Punto Montaje: %s", configuracion.puntoMontaje);
    log_info(logger, "Retardo: %i", configuracion.retardo);
    log_info(logger, "Tamaño value: %i", configuracion.tamanioValue);
    log_info(logger, "Tiempo dump: %i", configuracion.tiempoDump);

    ejecutarConsola(&gestionarRequest);
    // crearHiloServidor(configuracion.puertoEscucha, &atenderMensajes, NULL, NULL);
    //int cliente = crearSocketCliente("192.168.0.30", 8000);

    while (1);

    return 0;
}
