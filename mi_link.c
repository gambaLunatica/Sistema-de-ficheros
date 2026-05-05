#include <stdio.h>
#include <string.h>
#include "directorios.h"

int main(int argc, char **argv) {

    //Comprobamos la sintaxis
    if (argc != 4) {
        fprintf(stderr, "Sintaxis: ./mi_link <disco> </ruta_original> </ruta_enlace>\n");
        return FALLO;
    }

    //Comprobamos que no sean directorios (no deben acabar en '/')
    if (argv[2][strlen(argv[2]) - 1] == '/' || argv[3][strlen(argv[3]) - 1] == '/') {
        fprintf(stderr, "Error: no se permiten enlaces a directorios\n");
        return FALLO;
    }

    //Montamos el  dispositivo
    if (bmount(argv[1]) == FALLO) {
        perror("Error en bmount");
        return FALLO;
    }

    //Ejecutamos mi_link
    if (mi_link(argv[2], argv[3]) == FALLO) {
        perror("Error en mi_link");
        bumount();
        return FALLO;
    }

    //Desmontamos dispositivo
    bumount();

    return EXITO;
}