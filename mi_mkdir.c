/**
 * @author Juana Luna
 * @author Paola Chacín
 * @author Yassin EL Gharsa
 * 
 *  Programa que crea directorios.
 */

#include "directorios.h"

int main(int argc, char **argv){

    // Comprobamos sintaxis
    if (argc != 4) {
        fprintf(stderr,
        "Sintaxis: ./mi_mkdir <nombre_dispositivo> <permisos> </ruta_directorio/>\n");
        return FALLO;
    }


    // Obtenemos permisos
    int permisos = atoi(argv[2]);

    if (permisos < 0 || permisos > 7) {
        fprintf(stderr, "Error: modo inválido: <<%s>>\n", argv[2]);
        return FALLO;
    }

 
    // La ruta debe acabar en /
    if (argv[3][strlen(argv[3]) - 1] != '/') {
        fprintf(stderr, "Error: la ruta no es un directorio.\n");
        return FALLO;
    }

    // No permitir crear raíz
    if (strcmp(argv[3], "/") == 0) {
        fprintf(stderr, "Error: no se puede crear la raíz.\n");
        return FALLO;
    }

    // Montar disco
    if (bmount(argv[1]) == FALLO) {
        perror("Error en bmount");
        return FALLO;
    }

    // Crear directorio
    int error = mi_creat(argv[3], permisos);

    if (error < 0) {
        mostrar_error_buscar_entrada(error);
    }

    // Desmontar disco
    bumount();

    return error;
}