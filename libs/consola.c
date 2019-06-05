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


char *obtenerPathTabla(char *nombreTabla, char* puntoMontaje) {
    char *basePath = string_new();
    string_append(&basePath, puntoMontaje);
    if(!string_ends_with(puntoMontaje,"/")){
        string_append(&basePath, "/");
    }
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

bool matchea(char* palabra, char* expresion) {
    regex_t regex;
    regcomp(&regex, expresion, REG_EXTENDED);

    return regexec(&regex, palabra, 0, NULL, 0) != REG_NOMATCH;
}

bool esEntero(char* palabra) {
    return matchea(palabra, "[[:digit:]]+");
}

bool esString(char* palabra)    {
    return matchea(palabra, "[[:alnum:]]+");
}