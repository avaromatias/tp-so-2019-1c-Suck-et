#include "consola.h"

bool cantidadParametrosEsValida(t_comando comando)  {
    switch(comando.tipoRequest) {
        case JOURNAL:
            return comando.cantidadParametros == 0;
        case METRICS:
            return comando.cantidadParametros == 0;
        case DROP:
        case RUN:
            return comando.cantidadParametros == 1;
        case SELECT:
            return comando.cantidadParametros == 2;
        case INSERT:
            return comando.cantidadParametros == 3 || comando.cantidadParametros == 4;
        case DESCRIBE:
            return comando.cantidadParametros == 1 || comando.cantidadParametros == 0;
        case CREATE:
            return comando.cantidadParametros == 4;
        default:
            return false;
    }
}

bool parametrosSonValidos(t_comando comando)    {
    switch(comando.tipoRequest) {
        case SELECT:
            return esString(comando.parametros[0]) && esEntero(comando.parametros[1]) && (comando.cantidadParametros == 2 || (comando.cantidadParametros == 3 && esEntero(comando.parametros[3])));
        case INSERT:
            return esString(comando.parametros[0]) && esEntero(comando.parametros[1]) && esString(comando.parametros[2]);
        case CREATE:
            return esString(comando.parametros[0]) && esString(comando.parametros[1]) && esEntero(comando.parametros[2]) && esEntero(comando.parametros[3]);
    }
}

bool validarComandosComunes(t_comando comando, t_log* logger) {
    bool esValido = cantidadParametrosEsValida(comando) && parametrosSonValidos(comando);

    if(!esValido)
        log_warning(logger, "Alguno de los parámetros ingresados es incorrecto. Por favor verifique su entrada.");

    return esValido;
}

void imprimirErrorParametros() {
    printf("Alguno de los parámetros ingresados es incorrecto. Por favor verifique su entrada.\n");
}

char *obtenerPathTabla(char *nombreTabla, char* puntoMontaje) {
    char *basePath = string_duplicate(puntoMontaje);
    if(!string_ends_with(puntoMontaje,"/")){
        string_append(&basePath, "/");
    }
    string_append(&basePath, "Tables/");
    string_append(&basePath, nombreTabla);
    return basePath;
}

char *obtenerPathMetadata(char *nombreTabla, char* puntoMontaje) {
    char *tablePath = obtenerPathTabla(nombreTabla, puntoMontaje);
    string_append(&tablePath, "/Metadata");
    return tablePath;
}

bool matchea(char* palabra, char* expresion) {
    regex_t regex;
    int reg=regcomp(&regex, expresion, REG_EXTENDED);
    bool result= regexec(&regex, palabra, 0, NULL, 0) != REG_NOMATCH;
    regfree(&regex);
    return result;
}

bool esEntero(char* palabra) {
    return matchea(palabra, "[[:digit:]]+");
}

bool esString(char* palabra)    {
    return matchea(palabra, "[[:alnum:]]+");
}