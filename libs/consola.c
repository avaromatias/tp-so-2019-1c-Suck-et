#include "consola.h"

int parametrosValidos(t_comando comando) { //
    char *tabla = comando.parametros[0];
    char *param1 = comando.parametros[1];
    char *param2 = comando.parametros[2];
    char *param3 = comando.parametros[3];
    switch (comando.tipoRequest) {
        case SELECT:
            if (tabla == NULL || param1 == NULL)
                return 0;
            break;
        case INSERT:
            if (tabla == NULL || param1 == NULL || param2 == NULL)
                return 0;
            break;
        case CREATE:
            if (tabla == NULL || param1 == NULL || param2 == NULL || param3 == NULL)
                return 0;
            break;
        case DESCRIBE:
            if (param1 != NULL || param2 != NULL || param3 != NULL)
                return 0;
            break;
        case DROP:
            if (tabla == NULL)
                return 0;
            break;
        case JOURNAL:
            if (tabla != NULL)
                return 0;
            break;
        case ADD:
            if (param3 == "sc" || param3 == "shc" || param3 == "ec")
                string_to_upper(param3);
            if (tabla != "MEMORY" || param1 == NULL || param2 != "TO" || (param3 != "SC" && param3 != "SHC" && param3 != "EC"))
                return 0;
            break;
        case RUN:
            if (!sePuedeLeerElArchivo(param1))
                return 0;
            break;
        case METRICS:
            if (tabla != NULL)
                return 0;
            break;
        case HELP:
            if (tabla != NULL)
                return 0;
            break;
        case EXIT:
            if (tabla != NULL)
                return 0;
            break;
        case INVALIDO:
            comandoInvalido();
            break;
    }
    return 1;
}

bool validarComandosComunes(t_comando comando) {
    bool cantidadCorrectaParametros;
    if (parametrosValidos(comando) == 1) {
        switch (comando.tipoRequest) {
            case SELECT:
                cantidadCorrectaParametros = (comando.cantidadParametros) == 2;
                break;
            case INSERT:
                cantidadCorrectaParametros = (comando.cantidadParametros) == 3;
                break;
            case CREATE:
                cantidadCorrectaParametros = (comando.cantidadParametros) == 4;
                break;
            case DROP:
                cantidadCorrectaParametros = (comando.cantidadParametros) == 1;
            case DESCRIBE:
                break;
            case JOURNAL:
                cantidadCorrectaParametros = comando.cantidadParametros == 0;
                break;
            case METRICS:
                cantidadCorrectaParametros = comando.cantidadParametros == 0;
                break;
            case RUN:
                cantidadCorrectaParametros = comando.cantidadParametros == 1;
                break;
            case HELP:
                cantidadCorrectaParametros = comando.cantidadParametros == 0;
                break;
            case EXIT:
                cantidadCorrectaParametros = comando.cantidadParametros == 0;
                break;
        }
    }
    if (!cantidadCorrectaParametros)
        imprimirErrorParametros();
    return cantidadCorrectaParametros;
}

/*int validarComandosComunes(t_comando requestParseada) {
    char *tabla = requestParseada.parametros[0];
    char *param1 = requestParseada.parametros[1];
    char *param2 = requestParseada.parametros[2];
    char *param3 = requestParseada.parametros[3];
    switch (requestParseada.tipoRequest) {
        case SELECT:
            if (tabla == NULL || param1 == NULL) {
                imprimirErrorParametros();
                return 0;
            }
        case INSERT:
            if (nombreTabla == NULL || param1 == NULL || param2 == NULL) {
                imprimirErrorParametros();
                return 0;
            }
        case CREATE:
            if (tabla == NULL || param1 == NULL || param2 == NULL || param3 == NULL) {
                imprimirErrorParametros();
                return 0;
            }
        case DESCRIBE:
            if (param1 != NULL || param2 != NULL || param3 != NULL) {
                imprimirErrorParametros();
                return 0;
            }
        case DROP:
            if (nombreTabla == NULL || param1 != NULL || param2 != NULL || param3 != NULL) {
                imprimirErrorParametros();
                return 0;
            }
        case HELP:
            printf("************ Comandos disponibles ************\n");
            printf("- SELECT [NOMBRE_TABLA] [KEY]\n");
            printf("- INSERT [NOMBRE_TABLA] [KEY] “[VALUE]” [Timestamp](Opcional)\n");
            printf("- CREATE [NOMBRE_TABLA] [TIPO_CONSISTENCIA] [NUMERO_PARTICIONES] [COMPACTION_TIME]\n");
            printf("- DESCRIBE [NOMBRE_TABLA](Opcional)\n");
            printf("- DROP [NOMBRE_TABLA]\n");
            printf("- ADD MEMORY [NUMERO_MEMORIA] TO [TIPO_CONSISTENCIA]\n");
            printf("- RUN [PATH]\n");
            printf("- JOURNAL\n");
            printf("- METRICS\n");
            printf("- EXIT\n");
            return 0;
        case EXIT:
            return 0;
    }
    return 1;
}
}*/

void cantidadIncorrectaParametros(){
    printf("La cantidad de parámetros son incorrectos. Intente nuevamente.\n");
}
void comandoInvalido(){
    printf("El comando ingresado es inválido. Para más información, puede consultar con el comando HELP.\n");
}
void imprimirErrorParametros() {
    printf("Alguno de los parámetros ingresados es incorrecto. Por favor verifique su entrada.\n");
}

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