#include "ficheros_basico.h"

/**
 * @param nbloques: Número total de bloques del dispositivo virtual
 * @return: Cantidad de bloques necesarios para el mapa de bits (MB)
 * Calcula el tamaño en bloques necesarios para el mapa de bits.
 */
int tamMB(unsigned int nbloques) {
    int bytes = nbloques / 8;

    if(nbloques % 8 != 0) {
        bytes++;
    }

    // Verificamos si necesitamos un bloque extra
    if (bytes % BLOCKSIZE != 0) {
        // si hay resto necesitaremos un bloque mas
        return (bytes / BLOCKSIZE) + 1;
    }
    
    return bytes / BLOCKSIZE;
}

/**
 * @param ninodos: Número total de inodos del dispositivo virtual
 * @return: Cantidad de bloques necesarios para el array de inodos (AI)
 * Calcula el tamaño en bloques necesarios para el array de inodos.
 */
int tamAI(unsigned int ninodos) {
    
    int inodosBloque = BLOCKSIZE / INODOSIZE; // Cantidad de inodos que caben en un bloque

    if (ninodos % inodosBloque != 0) {
        // Si hay resto necesitaremos 1 bloque mas
        return (ninodos / inodosBloque) + 1;
    }
    // Si no devolvemos el mismo numero
    return ninodos / inodosBloque;
}

/**
 * @param nbloques: Número total de bloques del dispositivo virtual
 * @param ninodos: Número total de inodos del dispositivo virtual
 * @return: EXITO (0) o FALLO (-1)
 * Inicializa el superbloque con la información correspondiente y lo escribe en el bloque 0.
 */
int initSB(unsigned int nbloques, unsigned int ninodos) {
    struct superbloque SB;
    // Completamos la información de las variables de ficheros_basico.h
    SB.posPrimerBloqueMB = posSB + tamSB;
    SB.posUltimoBloqueMB = SB.posPrimerBloqueMB + tamMB(nbloques) - 1;
    SB.posPrimerBloqueAI = SB.posUltimoBloqueMB + 1;
    SB.posUltimoBloqueAI = SB.posPrimerBloqueAI + tamAI(ninodos) - 1;
    SB.posPrimerBloqueDatos = SB.posUltimoBloqueAI + 1;
    SB.posUltimoBloqueDatos = nbloques - 1;
    SB.posInodoRaiz = 0;
    SB.posPrimerInodoLibre = 0;
    SB.cantBloquesLibres = nbloques;
    SB.cantInodosLibres = ninodos;
    SB.totBloques = nbloques;
    SB.totInodos = ninodos;
    // Inicializamos el superbloque
    if (bwrite(posSB, &SB) == FALLO) {
        fprintf(stderr, "Error al escribir el superbloque\n");
        return FALLO;
    }
    return EXITO;
}

/**
 * @return: EXITO (0) o FALLO (-1)
 * Inicializa el mapa de bits (MB) marcando como ocupados los bloques de metadatos (SB, MB y AI).
 */
int initMB(){
    //1. Leemos el superbloque
    struct superbloque SB;
    if(bread(posSB, &SB) == FALLO){
        perror("Error en la lectura del SB");
        return FALLO;
    }

    unsigned int metadatos = SB.posPrimerBloqueDatos; //Los datos emoiezan despues de los metadatos
    unsigned int bitsRes = SB.posPrimerBloqueDatos;
    char bufferMB[BLOCKSIZE];

    //2. Recorremos los bloques del MB 
    for(unsigned int i = SB.posPrimerBloqueMB; i <= SB.posUltimoBloqueMB; i++){
        memset(bufferMB, 0, BLOCKSIZE);//Rellenamos el buffer con 0s

        //3. LLenamos los bloques con 1s
        if(bitsRes >= BLOCKSIZE*8){
            memset(bufferMB, 255, BLOCKSIZE);
            bitsRes -= BLOCKSIZE*8;
        }else if(bitsRes > 0){
            int bytes = bitsRes/8;
            for(int j = 0; j < bytes; j++){//Rellenamos bytes completos
                bufferMB[j] = 255;
            }

            int bits = bitsRes % 8;
            unsigned char resto = 0;
            if (bits > 0) {
                for(int j = 0; j < bits; j++){
                    resto += 1 << (7 - j);
                }
            }
            bufferMB[bytes] = resto;
            bitsRes = 0;

        }
        if (bwrite(i, bufferMB) == FALLO) {
            perror("Error escribiendo MB");
            return FALLO;
        }

    }
    //4. Actaulizamos y guardamos
    SB.cantBloquesLibres -= metadatos;
    if (bwrite(posSB, &SB) == FALLO) {
        perror("Error actualizando SB");
        return FALLO;
    }

    return EXITO;

}

/**
 * @return: EXITO (0) o FALLO (-1)
 * Inicializa el array de inodos (AI) marcando todos los inodos como libres y enlazándolos entre sí.
 */
int initAI() {
    struct superbloque SB;
    if(bread(posSB, &SB) == FALLO){
        perror("Error leyendo SB");
        return FALLO;
    }
    struct inodo inodos[BLOCKSIZE / INODOSIZE];
    unsigned nInodos = SB.posPrimerInodoLibre; // Empezamos a contar los inodos libres desde el primer inodo libre
    for (unsigned i = SB.posPrimerBloqueAI; i <= SB.posUltimoBloqueAI; i++) {
        for (int j = 0; j < BLOCKSIZE / INODOSIZE; j++) {
            inodos[j].tipo = 'l';
            if (nInodos < SB.totInodos - 1) { // Si aún hay inodos libres, asignamos el siguiente inodo libre al puntero directo
                inodos[j].punterosDirectos[0] = nInodos + 1; // El siguiente inodo libre es el siguiente número de inodo
            } else {
                inodos[j].punterosDirectos[0] = UINT_MAX; // No hay más inodos libres
                j = BLOCKSIZE / INODOSIZE;
            }
            nInodos++;
        }
        if (bwrite(i, inodos) == FALLO) {
            perror("Error al escribir el AI");
            return FALLO;
        };
    }
    return EXITO;
}

/**
 * @param nbloque: Número de bloque a modificar
 * @param bit: Valor del bit a escribir (0 o 1)
 * @return: EXITO (0) o FALLO (-1)
 * Escribe un bit en el mapa de bits (MB) para marcar un bloque como libre u ocupado.
 */
int escribir_bit(unsigned int nbloque, unsigned int bit) {


    struct superbloque SB;
    if (bread(posSB, &SB) == FALLO) {
        perror("Error leyendo SB ");
        return FALLO;
    }

    unsigned int posbyte = nbloque / 8;
    unsigned int posbit = nbloque % 8;
    unsigned int numbloqueMB = posbyte / BLOCKSIZE;
    unsigned int nbloqueabs = SB.posPrimerBloqueMB + numbloqueMB;
    unsigned char bufferMB[BLOCKSIZE];
    memset(bufferMB, '\0', BLOCKSIZE);

    if (bread(nbloqueabs, &bufferMB) == FALLO) {
        fprintf(stderr, "Error en la lectura del bloque \n");
        return FALLO;
    }

    posbyte = posbyte % BLOCKSIZE;
    unsigned char mask = 128;
    mask >>= posbit;

    //Modificamos el bit
    if (bit == 1) {
        bufferMB[posbyte] |= mask; //Ponemos a 1
    } else {
        bufferMB[posbyte] &= ~mask; //Ponemos a 0
    }
    
    //Escribimos el bloque modificado
    if (bwrite(nbloqueabs, &bufferMB) == FALLO) {
        fprintf(stderr, "Error en la escritura del bit en el bloque\n");
        return FALLO;
    }
    return EXITO;
}

/**
 * @param nbloque: Número de bloque a leer
 * @return: Valor del bit leído (0 o 1) o FALLO (-1) en caso de error
 * Lee un bit del mapa de bits (MB) para verificar si un bloque está libre u ocupado.
 */
char leer_bit(unsigned int nbloque) {

    struct superbloque SB;
    if (bread(posSB, &SB) == FALLO) {
        fprintf(stderr, "Error en la lectura del superbloque \n");
        return FALLO;
    }

    unsigned int posbyte = nbloque / 8;
    unsigned int posbit = nbloque % 8;
    unsigned int numbloqueMB = posbyte / BLOCKSIZE;
    unsigned int nbloqueabs = SB.posPrimerBloqueMB + numbloqueMB;
    unsigned char bufferMB[BLOCKSIZE];

    if (bread(nbloqueabs, &bufferMB) == FALLO) {
        fprintf(stderr, "Error al leer el bloque\n");
        return FALLO;
    }

    posbyte = posbyte % BLOCKSIZE;
    unsigned char mask = 128;
    mask >>= posbit;
    mask &= bufferMB[posbyte]; //Extraemos el bit
    mask >>= (7 - posbit); //Lo desplazamos a la posición 0 para devolverlo como 0 o 1

    return mask;
}

