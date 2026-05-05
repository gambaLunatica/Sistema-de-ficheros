

#include "directorios.h"


int main(int argc, char **argv) {
    // 1. Validación de argumentos
    if (argc != 3) {
        fprintf(stderr, "Error de sintaxis: ./mi_cat <disco> </ruta_fichero>\n");
        return FALLO;
    }

    char *nombre_disco = argv[1];
    char *ruta = argv[2];

    // 2. Montar el disco
    if (bmount(nombre_disco) == FALLO) {
        fprintf(stderr, "Error: No se ha podido montar el disco %s\n", nombre_disco);
        return FALLO;
    }

    // 3. Leer el contenido del fichero
    char buffer[TAMBUFFER];
    int bytes_leidos = 0;
    int total_leidos = 0;
    int offset = 0;

    // Usamos un bucle para leer todo el fichero en trozos de TAMBUFFER
    // mi_read() internamente llamará a buscar_entrada y mi_read_f
    memset(buffer, 0, sizeof(buffer));
    bytes_leidos = mi_read(ruta, buffer, offset, TAMBUFFER);

    while (bytes_leidos > 0) {
        // Escribir en la salida estándar (pantalla) lo que hemos leído
        write(1, buffer, bytes_leidos); 
        
        total_leidos += bytes_leidos;
        offset += bytes_leidos;
        
        memset(buffer, 0, sizeof(buffer));
        bytes_leidos = mi_read(ruta, buffer, offset, TAMBUFFER);
    }

    if (bytes_leidos < 0) {
        // Si mi_read devuelve un error (ej. es un directorio o no existe)
        // mostrar_error_buscar_entrada ya lo gestiona internamente si lo pusiste en mi_read
        fprintf(stderr, "\nError al leer el fichero (Código: %d)\n", bytes_leidos);
    } else {
        fprintf(stderr, "\n------------------------------\n");
        fprintf(stderr, "Total bytes leídos: %d\n", total_leidos);
    }

    // 4. Desmontar
    bumount();

    return EXITO;
}