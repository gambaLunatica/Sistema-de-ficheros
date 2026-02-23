
/**
 * @author Juana Luna
 */
#include "bloques.h"
    // Para llamadas al sistema como open, close.

static int descriptor = 0; // Variable que nadie fuera de este archivo puede tocar, no se cierra por

int bmount(const char *camino) {
    //Llamamos a umask para asegurarnos de que los permisos se apliquen correctamente al crear el archivo
    umask(000);
    // Abrimos con lectura/escritura y creación si no existe
    // Los permisos 0666 significan rw-rw-rw-
    descriptor = open(camino, O_RDWR | O_CREAT, 0666);
    
    if (descriptor == FALLO) {
        // Si hay error, podrías imprimirlo con perror para saber qué pasó
        perror("Error en bmount"); 
        return FALLO;
    }
    
    return descriptor;
}
/**
 * Desmonta el dispositivo virtual
*/
int bumount() {
    // Intentamos cerrar el descriptor actual
    if (close(descriptor) == FALLO) {
        perror("Error en bumount al cerrar el dispositivo");
        return FALLO;
    }

    //Marcamos el descriptor como no válido tras cerrarlo
    descriptor = FALLO;
    
    return EXITO; 
}
// El descriptor debe estar definido globalmente en el archivo como vimos antes
// static int descriptor;

/**
 * Escribe 1 bloque del dispositivo virtual
*/
int bwrite(unsigned int nbloque, const void *buf) {
    // 1. Calcular el desplazamiento
    off_t desplazamiento = (off_t)nbloque * BLOCKSIZE; //Por si * supera el rango de int

    // 2. Posicionar el puntero del fichero
    // SEEK_SET indica que el desplazamiento es respecto al inicio del fichero
    if (lseek(descriptor, desplazamiento, SEEK_SET) == FALLO) {
        perror("Error en lseek de bwrite");
        return FALLO;
    }

    // 3. Escribir el bloque
    ssize_t bytes_escritos = write(descriptor, buf, BLOCKSIZE);

    // 4. Control de errores
    if (bytes_escritos == FALLO) {
        perror("Error en write de bwrite");
        return FALLO;
    }

    // Devolvemos la cantidad de bytes escritos (debería ser BLOCKSIZE)
    return (int)bytes_escritos; //casting int para devolver un valor entero, aunque write devuelve ssize_t que es un tipo de dato para representar el número de bytes escritos o un error.
}
// El descriptor debe estar definido globalmente en el archivo
// static int descriptor;

int bread(unsigned int nbloque, void *buf) {
    // 1. Calcular el desplazamiento (offset)
    off_t desplazamiento = (off_t)nbloque * BLOCKSIZE; 

    // 2. Mover el puntero del fichero
    if (lseek(descriptor, desplazamiento, SEEK_SET) == FALLO) {
        perror("Error en lseek de bread");
        return FALLO;
    }

    // 3. Leer el bloque
    // Guardamos en buf los datos que sacamos del descriptor
    ssize_t bytes_leidos = read(descriptor, buf, BLOCKSIZE);

    // 4. Control de errores
    if (bytes_leidos == FALLO) {
        perror("Error en read de bread");
        return FALLO;
    }

    // Devolvemos la cantidad de bytes leídos (debería ser BLOCKSIZE)
    return (int)bytes_leidos;
}