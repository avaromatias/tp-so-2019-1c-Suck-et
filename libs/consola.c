#include "consola.h"

void ejecutarConsola(void (*gestionarComando)(char**)) {
    do{
        system("clear");
        printf("************ CONSOLA ************\n\n");
        printf("***** Comandos disponibles *****\n");
        printf("- SELECT [NOMBRE_TABLA] [KEY]\n");
        printf("- INSERT [NOMBRE_TABLA] [KEY] “[VALUE]” [Timestamp]\n");
        printf("- CREATE [NOMBRE_TABLA] [TIPO_CONSISTENCIA] [NUMERO_PARTICIONES] [COMPACTION_TIME]\n");
        printf("- DESCRIBE [NOMBRE_TABLA](Opcional)\n");
        printf("- DROP [NOMBRE_TABLA]\n");
        printf("- EXIT\n");
        char* leido = readline("Ingrese un comando:");
        char* comando = malloc(sizeof(char) * strlen(leido) + 1);
        memcpy(comando, leido, strlen(leido));
        comando[strlen(leido)] = '\0';
        char** comandoParseado = parsear(comando);
        gestionarComando(comandoParseado);
/*      if (strcmp(string_to_lower(comando), "select") == 0) {
            //Has tu magia
        } else if (strcmp(string_to_lower(comando), "insert") == 0) {
            //Has tu magia
        } else if (strcmp(string_to_lower(comando), "create") == 0) {
            //Has tu magia
        } else if (strcmp(string_to_lower(comando), "status") == 0) {
            //Has tu magia
        } else if (strcmp(string_to_lower(comando), "describe") == 0) {
            //Has tu magia
        } else if (strcmp(string_to_lower(comando), "drop") == 0) {
            //Has tu magia
        } else if (strcmp(string_to_lower(comando), "exit") == 0) {
            printf("Saliendo de la consola...\n");
        } else {
            printf("El comando que ha escrito no es válido.\n");
        }*/
        free(comando);
    }while(strcmp(string_to_lower(comando), "exit") == 0)
    printf("Ya analizamos todo lo solicitado.\n");
}