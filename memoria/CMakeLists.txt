cmake_minimum_required(VERSION 3.5)
set(CMAKE_C_STANDARD 99)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ".")
set(PROYECTO memoria)
set(PROYECTO_SRC
        "memoria.c"
        "../libs/config.c"
        "../libs/generales.c"
        "../libs/sockets.c"
        "../libs/consola.c"
        "conexiones.c")
set(LIBRERIAS
        "commons"
        "pthread"
        "readline"
        "rt")

project(${PROYECTO})

include_directories(.
        ../libs)

add_executable(${PROYECTO} ${PROYECTO_SRC})

target_link_libraries(${PROYECTO} ${LIBRERIAS})