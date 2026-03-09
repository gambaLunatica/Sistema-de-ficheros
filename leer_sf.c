#include "ficheros_basico.h"

int main(int argc, char **argv) {

    if (argc != 2) {
        fprintf(stderr, "Uso: ./leer_sf <nombre_dispositivo>\n");
        return FALLO;
    }

    if (bmount(argv[1]) == FALLO) {
        perror("Error en bmount");
        return FALLO;
    }

    struct superbloque SB;

    if (bread(posSB, &SB) == FALLO) {
        perror("Error leyendo superbloque");
        bumount();
        return FALLO;
    }

    printf("DATOS DEL SUPERBLOQUE\n");
    printf("posPrimerBloqueMB = %u\n", SB.posPrimerBloqueMB);
    printf("posUltimoBloqueMB = %u\n", SB.posUltimoBloqueMB);
    printf("posPrimerBloqueAI = %u\n", SB.posPrimerBloqueAI);
    printf("posUltimoBloqueAI = %u\n", SB.posUltimoBloqueAI);
    printf("posPrimerBloqueDatos = %u\n", SB.posPrimerBloqueDatos);
    printf("posUltimoBloqueDatos = %u\n", SB.posUltimoBloqueDatos);
    printf("posInodoRaiz = %u\n", SB.posInodoRaiz);
    printf("posPrimerInodoLibre = %u\n", SB.posPrimerInodoLibre);
    printf("cantBloquesLibres = %u\n", SB.cantBloquesLibres);
    printf("cantInodosLibres = %u\n", SB.cantInodosLibres);
    printf("totBloques = %u\n", SB.totBloques);
    printf("totInodos = %u\n", SB.totInodos);

    printf("\nsizeof struct superbloque: %lu\n", sizeof(struct superbloque));
    printf("sizeof struct inodo: %lu\n", sizeof(struct inodo));


    //struct inodo inodos[BLOCKSIZE / INODOSIZE];

    /**while (ninodo != UINT_MAX) {

        printf("%u ", ninodo);

        unsigned int bloque = SB.posPrimerBloqueAI +
                              (ninodo / (BLOCKSIZE / INODOSIZE));
        

        if (bread(bloque, inodos) == FALLO) {
            perror("Error leyendo bloque de inodos");
            break;
        }

        unsigned int i = ninodo % (BLOCKSIZE / INODOSIZE);
        ninodo = inodos[i].punterosDirectos[0];
    }

    printf("\n");*/

    // Reservar bloque
    int bloque_reservado = reservar_bloque();
    bread(posSB,&SB);
    printf("\nReservado el bloque %d\nBloques libres: %d\n", bloque_reservado, SB.cantBloquesLibres);
    // Liberar bloque
    liberar_bloque(bloque_reservado);
    bread(posSB,&SB);
    printf("Liberado el bloque %d\nBloques libres: %d\n\n", bloque_reservado, SB.cantBloquesLibres);
    printf("bits de las zonas del dispositivo\n");
    printf("bit leido en posSB (bloque n.%d): %d\n", posSB, leer_bit(posSB));
    printf("bit leido en posPrimerBloqueMB (bloque n.%d): %d\n", SB.posPrimerBloqueMB, leer_bit(SB.posPrimerBloqueMB));
    printf("bit leido en posUltimoBloqueMB (bloque n.%d): %d\n", SB.posUltimoBloqueMB, leer_bit(SB.posUltimoBloqueMB));
    printf("bit leido en posPrimerBloqueAI (bloque n.%d): %d\n", SB.posPrimerBloqueAI, leer_bit(SB.posPrimerBloqueAI));
    printf("bit leido en posUltimoBloqueAI (bloque n.%d): %d\n", SB.posUltimoBloqueAI, leer_bit(SB.posUltimoBloqueAI));
    printf("bit leido en posPrimerBloqueDatos (bloque n.%d): %d\n", SB.posPrimerBloqueDatos, leer_bit(SB.posPrimerBloqueDatos));
    printf("bit leido en posUltimoBloqueDatos (bloque n.%d): %d\n\n", SB.posUltimoBloqueDatos, leer_bit(SB.posUltimoBloqueDatos));

    // Lectura datos
    printf("Lectura datos del directorio raíz\n");
    struct tm *ts;
    char atime[80];
    char mtime[80];
    char ctime[80];
    struct inodo inodo;
    int ninodo = 0;
    if (leer_inodo(ninodo,&inodo) == -1) {
      fprintf(stderr, "Error en leer_sf.c %d: %s\n error al leer el inodo", errno, strerror(errno));
      return FALLO;
    }
    ts = localtime(&inodo.atime);
    strftime(atime, sizeof(atime), "%a %Y-%m-%d %H:%M:%S", ts);
    ts = localtime(&inodo.mtime);
    strftime(mtime, sizeof(mtime), "%a %Y-%m-%d %H:%M:%S", ts);
    ts = localtime(&inodo.ctime);
    strftime(ctime, sizeof(ctime), "%a %Y-%m-%d %H:%M:%S", ts);
    printf("tipo: %c\n", inodo.tipo);
    int permisos = (int) inodo.permisos;
    printf("Permisos: %d\n", permisos);
    printf("atime: %s\n", atime);
    printf("mtime: %s\n", mtime);
    printf("ctime: %s\n", ctime);
    printf("nlinks: %d\n", inodo.nlinks);
    printf("tamaño en bytes lógicos: %d\n", inodo.tamEnBytesLog);
    printf("NumBloquesOcupados: %d\n", inodo.numBloquesOcupados);

    printf("\nFunciones de traducciòn de bloques inodos\n ");
    bread(posSB,&SB);
    printf("\nposPrimerInodoLibre: %d\n",SB.posPrimerInodoLibre);
    int inodoReservado = reservar_inodo('f',6);

    printf("Se ha reservado el inodo %d\n", inodoReservado);
    printf("\nTRADUCCIÓN DE LOS BLOQUES LOGICOS 8, 204, 30.004, 400.004 y 468.750\n");
    printf("Traducción bloque lògico 8: %d\n",traducir_bloque_inodo(inodoReservado,8,1));
    printf("Traducción bloque lògico 204: %d\n",traducir_bloque_inodo(inodoReservado,204,1));
    printf("Traducción bloque lògico 30.004: %d\n",traducir_bloque_inodo(inodoReservado,30004,1));
    printf("Traducción bloque lògico 400.004: %d\n",traducir_bloque_inodo(inodoReservado,400004,1));
    printf("Traducción bloque lògico 468.750: %d\n",traducir_bloque_inodo(inodoReservado,468750,1));

    printf("\nLectura datos del inodo %d\n", inodoReservado);
    ninodo = inodoReservado;
    if (leer_inodo(ninodo,&inodo) == -1) {
      fprintf(stderr, "Error en leer_sf.c %d: %s\nImposible leer el inodo", errno, strerror(errno));
      return FALLO;
    }
    ts = localtime(&inodo.atime);
    strftime(atime, sizeof(atime), "%a %Y-%m-%d %H:%M:%S", ts);
    ts = localtime(&inodo.mtime);
    strftime(mtime, sizeof(mtime), "%a %Y-%m-%d %H:%M:%S", ts);
    ts = localtime(&inodo.ctime);
    strftime(ctime, sizeof(ctime), "%a %Y-%m-%d %H:%M:%S", ts);

    printf("Tipo: %c\n", inodo.tipo);
    permisos = (int) inodo.permisos;
    printf("Permisos: %d\n", permisos);
    printf("atime: %s\n", atime);
    printf("mtime: %s\n", mtime);
    printf("ctime: %s\n", ctime);
    printf("N. links: %d\n", inodo.nlinks);
    printf("Tamaño en bytes lógicos: %d\n", inodo.tamEnBytesLog);
    printf("numBloquesOcupados: %d\n", inodo.numBloquesOcupados);
    bread(posSB,&SB);
    printf("posPrimerInodoLibre (después de la reserva): %d\n\n",SB.posPrimerInodoLibre);

    bumount();

    return EXITO;
}