#include "consola.h"

int validarComandosComunes(char** comando){
    char *tipoDeRequest = comando[0];
    char *nombreTabla = comando[1];
    char *param1 = comando[2];
    char *param2 = comando[3];
    char *param3 = comando[4];
    string_to_upper(tipoDeRequest);
    if (strcmp(tipoDeRequest, "SELECT") == 0) {
        if(nombreTabla == NULL || param1 == NULL){
            printf("Número de parámetros inválido.\n");
            return 0;
        }

    } else if (strcmp(tipoDeRequest, "INSERT") == 0) {
        if(nombreTabla == NULL || param1 == NULL || param2 == NULL){
            printf("Número de parámetros inválido.\n");
            return 0;
        }

    } else if (strcmp(tipoDeRequest, "CREATE") == 0) {
        if (nombreTabla == NULL || param1 == NULL || param2 == NULL || param3 == NULL) {
            printf("Número de parámetros inválido.\n");
            return 0;
        }
    } else if (strcmp(tipoDeRequest, "DESCRIBE") == 0) {
       if(param1 != NULL || param2 != NULL || param3 != NULL){
           printf("Número de parámetros inválido.\n");
           return 0;
       }

    } else if (strcmp(tipoDeRequest, "DROP") == 0) {
        if(nombreTabla == NULL || param1 != NULL || param2 != NULL || param3 != NULL){
            printf("Número de parámetros inválido.\n");
            return 0;
        }
    } else if ((strcmp(tipoDeRequest, "ADD") == 0) || (strcmp(tipoDeRequest, "RUN") == 0) ||
                (strcmp(tipoDeRequest, "JOURNAL") || (strcmp(tipoDeRequest, "METRICS")))) {
        validarComandosKernel(comando);
    }
    return 1;
}

int validarComandosKernel(char** comando){
    char * tipoDeRequest = comando[0]; string_to_upper(tipoDeRequest);
    char *param1 = comando[1];
    string_to_upper(param1); //palabra "MEMORY" necesaria para hacer el ADD
    char *param2 = comando[2];
    char *param3 = comando[3];
    string_to_upper(param3); //el TO necesario para hacer el ADD
    char *param4 = comando[4];
    string_to_upper(param4); //tipoDeConsistencia
    if (strcmp(tipoDeRequest, "ADD") == 0) {
        if(param1 != "MEMORY" || param2 == NULL || param3 != "TO" ||
          (param4 != "SC" || param4 != "SHC" || param4 != "EC")){
            printf("Alguno de los parámetros ingresados es incorrecto. Por favor verifíque su entrada.\n");
            return 0;
          }
    } else if (strcmp(tipoDeRequest, "RUN") == 0) {
       char * path = comando[1];
       if(path == NULL){
          printf("El Path recibido es inválido.\n");
          return 0;
          }
    } else if (strcmp(tipoDeRequest, "JOURNAL") == 0) {
       if (param1 != NULL || param2 != NULL || param3 != NULL || param4 != NULL) {
          printf("Los parámetros son innecesarios.\n");
          return 0;
          }
    } else if (strcmp(tipoDeRequest, "METRICS") == 0) {
       if(param1 != NULL || param2 != NULL || param3 != NULL || param4 != NULL){
          printf("Los parámetros son innecesarios.\n");
          return 0;
          }
    }
}
void ejecutarConsola(int (*gestionarComando)(char**), char* nombreDelProceso, t_log *logger) {
    char* comando;
    char* nombreDelGrupo = "@suck-ets:~$ ";
    char* prompt = string_new();
    string_append(&prompt, nombreDelProceso);
    string_append(&prompt, nombreDelGrupo);
    do {
        char* leido = readline(prompt);
        comando = malloc(sizeof(char) * strlen(leido) + 1);
        memcpy(comando, leido, strlen(leido));
        comando[strlen(leido)] = '\0';
        char** comandoParseado = parser(comando);
        if(validarComandosComunes(comandoParseado)==1){
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
}

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