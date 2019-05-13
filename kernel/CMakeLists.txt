cmake_minimum_required(VERSION 3.5)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ".")
set(PROYECTO kernel)
set(PROYECTO_SRC
        "kernel.c"
        "../libs/config.c"
        "../libs/sockets.c"
        "../libs/consola.c"
        "../libs/generales.c" conexiones.c conexiones.h)
set(LIBRERIAS
        "commons"
        "readline"
        "pthread")

project(${PROYECTO})

set(CMAKE_C_STANDARD 99)

include_directories(.)

add_executable(${PROYECTO} ${PROYECTO_SRC})

target_link_libraries(${PROYECTO} ${LIBRERIAS})