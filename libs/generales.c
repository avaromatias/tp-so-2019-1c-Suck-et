/*
 ============================================================================
 Name        : Generales.c
 Author      : Suck et
 	 UTN FRBA - Sistemas Operativos
 	 	 TP-1C-2019-Lissandra
 ============================================================================
 */

#include "generales.h"

void printArrayDeStrings(char** arrayDeStrings){

    int count=0;
    while (arrayDeStrings[count] != NULL) {
        for (int i = 0; i <strlen(arrayDeStrings[count]) ; ++i) {
            printf("%c", arrayDeStrings[count][i]);

        }
        printf("\n");
        count++;
    }
}
int tamanioDeArrayDeStrings(char** arrayDeString){

    int count=0;
    if(arrayDeString == NULL){
        return 0;
    }
    while (arrayDeString[count] != NULL) {
        count++;
    }
    return count;
}
char* convertirArrayAString(char** array){
    char* resultado=string_new();
    string_append(&resultado,"[");
    for (int i = 0; i < tamanioDeArrayDeStrings(array); i++) {
        string_append(&resultado,(char*)array[i]);
        if(i <(tamanioDeArrayDeStrings(array)-1)){
            string_append(&resultado,",");

        }
    }
    string_append(&resultado,"]");
    return resultado;
}

int arrayIncluye(char** array, char* elemento){
    for (int i = 0; i < tamanioDeArrayDeStrings(array); i++) {
        if(strcmp(array[i],elemento)==0){
            return 1;
        }
    }
    return 0;
}

void mkdir_recursive(char *path) {
    char *subpath, *fullpath;

    fullpath = strdup(path);
    subpath = dirname(fullpath);
    if (strlen(subpath) > 1)
        mkdir_recursive(subpath);
    mkdir(path, 0777);
    free(fullpath);
}

void valorSinComillas(char *valor) {
    if (string_starts_with(valor, "\"") && string_ends_with(valor, "\"")) {
        int j = 0;
        for (int i = 0; i < strlen(valor); i++) {
            if (valor[i] == '\\') {
                valor[j++] = valor[i++];
                valor[j++] = valor[i];
                if (valor[i] == '\0')
                    break;
            } else if (valor[i] != '"')
                valor[j++] = valor[i];
        }
        valor[j] = '\0';
    }
}

char *armarLinea(char *key, char *valor, time_t timestamp) {
    char *linea = string_new();
    string_append(&linea, string_from_format("%ld", timestamp));
    string_append(&linea, ";");
    string_append(&linea, key);
    string_append(&linea, ";");
    valorSinComillas(valor);
    string_append(&linea, valor);
    string_append(&linea, "\n");
    return linea;
}

char **desarmarLinea(char *linea) {
    return string_split(linea, ";");
}

int archivoVacio(char *path) {
    FILE *f = fopen(path, "r");
    int c = fgetc(f);
    fclose(f);
    return c == EOF;
}

int pesoString(char* string)    {
    return string == NULL? 0 : sizeof(char) * (strlen(string) + 1);
}

char *concat(int count, ...) {
    va_list ap;
    int i;

    // Find required length to store merged string
    int len = 1; // room for NULL
    va_start(ap, count);
    for (i = 0; i < count; i++)
        len += strlen(va_arg(ap, char*));
    va_end(ap);

    // Allocate memory to concat strings
    char *merged = calloc(sizeof(char), len);
    int null_pos = 0;

    // Actually concatenate strings
    va_start(ap, count);
    for (i = 0; i < count; i++) {
        char *s = va_arg(ap, char*);
        strcpy(merged + null_pos, s);
        null_pos += strlen(s);
    }
    va_end(ap);

    return merged;
}


char** parser(char* input){
    string_trim(&input);
    if(string_is_empty(input))
        return NULL;
    char** parseado = string_split(input, "\"");
    char** parseadoSinVal=string_split(parseado[0]," ");
    int cantidadElementosSinComillas = tamanioDeArrayDeStrings(parseado);
    int cantidadElementosSinEspacios = tamanioDeArrayDeStrings(parseadoSinVal);
    int cantidadElementosComandoParseado = cantidadElementosSinComillas + cantidadElementosSinEspacios;
    char** comandoParseado = (char**) malloc(sizeof(char*) * cantidadElementosComandoParseado);

    int i = 0;
    while(i < cantidadElementosSinEspacios) {
        comandoParseado[i] = parseadoSinVal[i];
        i++;
    }

    char* valor = parseado[1];
    comandoParseado[i] = valor;

    if(valor != NULL) {
        comandoParseado[i] = string_from_format("\"%s\"", valor);
        free(valor);
        i++;
        if(parseado[2]){
            comandoParseado[i]=parseado[2];
            i++;
        }
        comandoParseado[i] = NULL;
    }
    string_to_upper(comandoParseado[0]);
    return comandoParseado;
}

t_comando instanciarComando(char** request) {//request ya parseada
    int cantidadParametros = obtenerCantidadParametros(request);
    t_comando comando;
    comando.cantidadParametros = cantidadParametros;
    comando.parametros=malloc(cantidadParametros*sizeof(char*));
    for (int i = 0; i < cantidadParametros; i++) {
        comando.parametros[i] = string_duplicate(request[i + 1]);//primero lo evitamos porque es de "TipoRequest"
    }

    //TODO extender cantidad de parametros para todas las request
    if (strcmp(request[0], "SELECT") == 0)
        comando.tipoRequest = SELECT;
    else if (strcmp(request[0], "INSERT") == 0)
        comando.tipoRequest = INSERT;
    else if (strcmp(request[0], "CREATE") == 0)
        comando.tipoRequest = CREATE;
    else if (strcmp(request[0], "DROP") == 0)
        comando.tipoRequest = DROP;
    else if (strcmp(request[0], "DESCRIBE") == 0)
        comando.tipoRequest = DESCRIBE;
    else if (strcmp(request[0], "JOURNAL") == 0)
        comando.tipoRequest = JOURNAL;
    else if (strcmp(request[0], "RUN") == 0)
        comando.tipoRequest = RUN;
    else if (strcmp(request[0], "ADD") == 0)
        comando.tipoRequest = ADD;
    else if (strcmp(request[0], "METRICS") == 0)
        comando.tipoRequest = METRICS;
    else if (strcmp(request[0], "HELP") == 0)
        comando.tipoRequest = HELP;
    else if (strcmp(request[0], "EXIT") == 0)
        comando.tipoRequest = EXIT;
    else {
        comando.tipoRequest = INVALIDO;
    }
    return comando;
}

int obtenerCantidadParametros(char** request) {
    int contador = 0;
    for (contador; request[contador] != NULL; contador++);
    return (contador- 1); //restamos el tipoRequest
}