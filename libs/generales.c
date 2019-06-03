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
    while (arrayDeString[count] != NULL) {
        count++;
    }
    printf("Cantidad: %i\n", count);
}

int pesoString(char* string)    {
    return string == NULL? 0 : sizeof(char) * (strlen(string) + 1);
}

char** parser(char* input){
    return string_split(input, " ");
}

t_comando instanciarComando(char** request) {//request ya parseada
    int cantidadParametros = obtenerCantidadParametros(request);
    t_comando comando = {.cantidadParametros = cantidadParametros, .parametros = malloc(sizeof(char*) * cantidadParametros)};
    for (int i = 0; i <= cantidadParametros; i++) {
        comando.parametros[i] = string_duplicate(request[i + 1]);//primero lo evitamos porque es de "TipoRequest"
    }
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