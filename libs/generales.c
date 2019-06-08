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

int pesoString(char* string)    {
    return string == NULL? 0 : sizeof(char) * (strlen(string) + 1);
}

char** parser(char* input){
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
        comandoParseado[i] = string_from_format("\"%s\"", comandoParseado[i]);
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