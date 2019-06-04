#include "consola.h"

bool validarComandosComunes(t_comando comando) {
    bool esValido;
    switch(comando.tipoRequest) {
        case JOURNAL:
        case METRICS:
            esValido = comando.cantidadParametros == 0;
            break;
        case DROP:
        case DESCRIBE:
        case RUN:
            esValido = comando.cantidadParametros == 1;
            break;
        case SELECT:
            esValido = comando.cantidadParametros == 2;
            break;
        case INSERT:
            esValido = comando.cantidadParametros == 3;
            break;
        case CREATE:
            esValido = comando.cantidadParametros == 4;
            break;
    }

    /*if(!esValido)
        imprimirErrorParametros();*/

    return esValido;
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

char *obtenerPathTabla(char *nombreTabla, char* puntoMontaje) {
    char *basePath = string_new();
    string_append(&basePath, "..");
    string_append(&basePath, puntoMontaje);
    string_append(&basePath, "Tables/");
    char *tablePath = string_new();
    string_append(&tablePath, basePath);
    string_append(&tablePath, nombreTabla);
    return tablePath;
}

char *obtenerPathMetadata(char *nombreTabla,char* puntoMontaje) {
    char *tablePath = obtenerPathTabla(nombreTabla,puntoMontaje);
    string_append(&tablePath, "/Metadata");
    return tablePath;
}