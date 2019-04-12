/*
 ============================================================================
 Name        : Memoria.h
 Author      : Suck et
 	 UTN FRBA - Sistemas Operativos
 	 	 TP-1C-2019-Lissandra
 ============================================================================
 */

#ifndef MEMORIA_H_
#define MEMORIA_H_

#include <commons/config.h>

typedef struct {
    int puerto;
    char* ipFileSystem;
    int puertoFileSystem;
    char** ipSeeds; // no estoy seguro si es char** ipSeeds o char* ipSeeds* para definir un array de strings
    int* puertoSeeds;
    int retardoMemoria;
    int retardoFileSystem;
    int tamanioMemoria;
    int retardoJournal;
    int retardoGossiping;
    int cantidadDeMemorias;
} t_configuracion;

bool existenTodasLasClavesObligatorias(t_config* archivoConfig, t_configuracion configuracion, char* clavesObligatorias[]);
t_configuracion cargarConfiguracion();


#endif /* MEMORIA_H_ */
