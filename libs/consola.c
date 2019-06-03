#include "consola.h"

int validarComandosComunes(char **comando) {
    char *tipoDeRequest = comando[0];
    char *nombreTabla = comando[1];
    char *param1 = comando[2];
    char *param2 = comando[3];
    char *param3 = comando[4];
    string_to_upper(tipoDeRequest);
    if (strcmp(tipoDeRequest, "SELECT") == 0) {
        if (nombreTabla == NULL || param1 == NULL) {
            imprimirErrorParametros();
            return 0;
        }

    } else if (strcmp(tipoDeRequest, "INSERT") == 0) {
        if (nombreTabla == NULL || param1 == NULL || param2 == NULL) {
            imprimirErrorParametros();
            return 0;
        }

    } else if (strcmp(tipoDeRequest, "CREATE") == 0) {
        if (nombreTabla == NULL || param1 == NULL || param2 == NULL || param3 == NULL) {
            imprimirErrorParametros();
            return 0;
        }
    } else if (strcmp(tipoDeRequest, "DESCRIBE") == 0) {
        if (param1 != NULL || param2 != NULL || param3 != NULL) {
            imprimirErrorParametros();
            return 0;
        }

    } else if (strcmp(tipoDeRequest, "DROP") == 0) {
        if (nombreTabla == NULL || param1 != NULL || param2 != NULL || param3 != NULL) {
            imprimirErrorParametros();
            return 0;
        }
    }
    return 1;
}

void imprimirErrorParametros() {
    printf("Alguno de los parámetros ingresados es incorrecto. Por favor verifique su entrada.\n");
}

/*void ejecutarConsola(void* parametrosConsola){

    parametros_consola* parametros = (parametros_consola*) parametrosConsola;

    t_log* logger = parametros->logger;
    int (*gestionarComando)(char**) = parametros->gestionarComando;
    Componente nombreDelProceso = parametros->unComponente;

    char* comando;
    char* nombreDelGrupo = "@suck-ets:~$ ";
    char* prompt = string_new();
    switch (nombreDelProceso){
        case KERNEL:
            string_append(&prompt, "Kernel");
            break;
        case MEMORIA:
            string_append(&prompt, "Memoria");
            break;
        case LISSANDRA:
            string_append(&prompt, "Lissandra");
            break;
    }
    string_append(&prompt, nombreDelGrupo);
    do {
        char* leido = readline(prompt);
        comando = malloc(sizeof(char) * strlen(leido) + 1);
        memcpy(comando, leido, strlen(leido));
        comando[strlen(leido)] = '\0';
        char** comandoParseado = parser(comando);
        if(validarComandosComunes(comandoParseado)== 1){
            if(gestionarComando(comandoParseado) == 0){
                log_info(logger, "Request procesada correctamente.");
            } else {
                log_error(logger, "No se pudo procesar la request solicitada.");
            };
        }
        string_to_lower(comando);
    } while(strcmp(comando, "exit") != 0);
    free(comando);
    printf("Ya analizamos todo lo solicitado.\n");
}*/

char *obtenerPathTabla(char *nombreTabla) {
    char *basePath = "../tables/";
    char *tablePath = string_new();
    string_append(&tablePath, basePath);
    string_append(&tablePath, nombreTabla);
    return tablePath;
}

char *obtenerPathMetadata(char *nombreTabla) {
    char *tablePath = obtenerPathTabla(nombreTabla);
    string_append(&tablePath, "/Metadata");
    return tablePath;
}