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

/**
 * @return: Número de bloque reservado o FALLO (-1) en caso de error
 * Reserva un bloque libre en el mapa de bits (MB) y lo marca como ocupado.
 */
int reservar_bloque() {

    struct superbloque SB;

    // Primero leemos el superbloque 
    if (bread(posSB, &SB) == FALLO) {
        perror("Error leyendo SB");
        return FALLO;
    }

    // Comprobamos si hay bloques libres
    //(Si no hay bloques libres, no podemos reservar ninguno)
    if (SB.cantBloquesLibres == 0) {
        return FALLO;
    }

    unsigned char bufferMB[BLOCKSIZE]; //Guarda un bloque del Mapa de Bits (MB) leído desde disco
    unsigned char bufferAuxiliar[BLOCKSIZE]; //Buffer auxiliar para comparar con el bloque del MB
    memset(bufferAuxiliar, 255, BLOCKSIZE); // incializamos a 1 (11111111) para comparar con el bloque del MB y encontrar el primer bloque con algún bit a 0 (libre)

    //Indica qué bloque del MB estamos analizando
    unsigned int nbloqueMB = 0;

   //Recorremos el MB bloque por bloque.
    while (nbloqueMB <= (SB.posUltimoBloqueMB - SB.posPrimerBloqueMB)) {

        if (bread(SB.posPrimerBloqueMB + nbloqueMB, bufferMB) == FALLO) {
            perror("Error leyendo bloque MB");
            return FALLO;
        }
        //Compara bloque real del MB con el bloque auxiliar lleno de 1s. Si son iguales, significa que el bloque del MB no tiene ningún bit a 0 (libre) y seguimos buscando.
        if (memcmp(bufferMB, bufferAuxiliar, BLOCKSIZE) != 0) {
            break;  // Hemos encontrado bloque con algún 0
        }

        nbloqueMB++;
    }

    // Buscar byte libre dentro del bloque MB
    unsigned int posbyte = 0;
    // Recorremos el bloque del MB byte por byte hasta encontrar un byte que no esté completamente lleno de 1s (255 en decimal)
    while (bufferMB[posbyte] == 255) {
        posbyte++;
    }

    // Buscamos primer bit a 0 dentro del byte usando una mascara and bit a bit
    unsigned char mascara = 128; // 10000000
   
    // Indica qué bit dentro del byte estamos mirando
    unsigned int posbit = 0;

    //Operación AND bit a bit
    while (bufferMB[posbyte] & mascara) {

        //Desplazamos el byte a la izquierda hasta encontrar un bit a 0.
        bufferMB[posbyte] <<= 1;
        posbit++;
    }

    // Calculamos el número de bloque absoluto a reservar
    //Número real de bloque físico en el disco
    unsigned int nbloque = (nbloqueMB * BLOCKSIZE + posbyte) * 8 + posbit;

    // Marcamos el bloque como ocupado en el MB, poniendo el bit a 1
    if (escribir_bit(nbloque, 1) == FALLO) {
        return FALLO;
    }

    // Actualizamos el SB
    SB.cantBloquesLibres--;
    if (bwrite(posSB, &SB) == FALLO) {
        return FALLO;
    }

    // Limpiamos el bloque reservado para que no contenga datos residuales
    unsigned char bufferDatos[BLOCKSIZE];
    memset(bufferDatos, 0, BLOCKSIZE);

    if (bwrite(nbloque, bufferDatos) == FALLO) {
        return FALLO;
    }

    return nbloque;
}

/**
 * @param nbloque: Número de bloque a liberar
 * @return: Número de bloque liberado o FALLO (-1) en caso de error
 * Libera un bloque ocupado en el mapa de bits (MB) y lo marca como libre.
 */
int liberar_bloque(unsigned int nbloque) {

    struct superbloque SB;

    // Leemos el superbloque para obtener la información necesaria para liberar el bloque
    if (bread(posSB, &SB) == FALLO) {
        perror("Error leyendo SB en liberar_bloque");
        return FALLO;
    }

    // Ponemos a 0 el bit correspondiente en el MB (marcar como libre)
    if (escribir_bit(nbloque, 0) == FALLO) {
        perror("Error escribiendo bit en liberar_bloque");
        return FALLO;
    }

    // Incrementamos la cantidad de bloques libres en el superbloque.
    SB.cantBloquesLibres++;

    // Guardamos el superbloque actualizado en disco
    if (bwrite(posSB, &SB) == FALLO) {
        perror("Error escribiendo SB en liberar_bloque");
        return FALLO;
    }

    // Devolvemos el número de bloque liberado
    return nbloque;
}
/**
 * @param ninodo: Número de inodo a escribir
 * @param inodo: Puntero al inodo con la información a escribir
 * @return: EXITO (0) o FALLO (-1) en caso de error
 * Escribe un inodo concreto en el array de inodos del disco virtual.
 */
int escribir_inodo(unsigned int ninodo, struct inodo *inodo) {
//escribe un inodo concreto en el array de inodos del disco virtual.
    struct superbloque SB;

    // Leemos el superbloque para saber dónde empieza el array de inodos
    if (bread(posSB, &SB) == FALLO) {
        perror("Error leyendo SB en escribir_inodo");
        return FALLO;
    }

    // Calculamos cuántos inodos caben en un bloque
    unsigned int inodos_por_bloque = BLOCKSIZE / INODOSIZE;

    // Calculamos qué bloque del array de inodos contiene este inodo
    unsigned int nbloqueAI = ninodo / inodos_por_bloque;

    // Convertimos a bloque absoluto dentro del dispositivo(el número real de bloque)
    unsigned int nbloqueabs = SB.posPrimerBloqueAI + nbloqueAI;

    // Buffer para cargar en memoria todos los inodos de ese bloque
    struct inodo inodos[inodos_por_bloque];

    // Leemos el bloque completo porque no podemos escribir solo un inodo suelto
    if (bread(nbloqueabs, inodos) == FALLO) {
        perror("Error leyendo bloque de inodos");
        return FALLO;
    }

    // Posición del inodo dentro del bloque
    unsigned int posicion_inodo = ninodo % inodos_por_bloque;

    // Sustituimos únicamente ese inodo en memoria
    inodos[posicion_inodo] = *inodo;

    // Escribimos de nuevo el bloque completo ya modificado
    if (bwrite(nbloqueabs, inodos) == FALLO) {
        perror("Error escribiendo bloque de inodos");
        return FALLO;
    }

    return EXITO;
}

/**
 * @param ninodo: Número de inodo a leer
 * @param inodo: Puntero al inodo donde se almacenará la información leída
 * @return: EXITO (0) o FALLO (-1) en caso de error
 * Lee un inodo concreto del array de inodos del disco virtual.
 */
int leer_inodo(unsigned int ninodo, struct inodo* inodo) {
    struct superbloque SB;
    if (bread(posSB, &SB) == FALLO) {
        perror("Error leyendo SB");
        return FALLO;
    }
    unsigned nbloqueAI = (ninodo * INODOSIZE) / BLOCKSIZE;
    unsigned nbloqueabs = nbloqueAI + SB.posPrimerBloqueAI;
    struct inodo inodos[BLOCKSIZE / INODOSIZE];
    if (bread(nbloqueabs, inodos) == FALLO) {
        perror("Error leyendo bloque de inodos");
        return FALLO;
    } 

    *inodo = inodos[ninodo % (BLOCKSIZE / INODOSIZE)];
     return EXITO;
}

/**
 * @param tipo: Tipo de inodo a reservar ('f' para fichero, 'd' para directorio)
 * @param permisos: Permisos del inodo a reservar
 * @return: Número de inodo reservado o FALLO (-1) en caso de error
 * Reserva un inodo libre en el array de inodos, lo inicializa con el tipo y permisos especificados, y lo marca como ocupado.
 */
int reservar_inodo(unsigned char tipo, unsigned char permisos) {

    struct superbloque SB;
    struct inodo inodo;

    if (bread(posSB, &SB) == FALLO) {
        perror("Error leyendo SB");
        return FALLO;
    }

    if (SB.cantInodosLibres == 0) {
        fprintf(stderr, "No hay inodos libres\n");
        return FALLO;
    }

    if (tipo != 'f' && tipo != 'd') {
        return FALLO;
    }

    unsigned int posInodoReservado = SB.posPrimerInodoLibre;

    if (leer_inodo(posInodoReservado, &inodo) == FALLO) {
        perror("Error al leer inodo");
        return FALLO;
    }

    SB.posPrimerInodoLibre = inodo.punterosDirectos[0];
    inodo.tipo = tipo;
    inodo.permisos = permisos;
    inodo.nlinks = 1;
    inodo.tamEnBytesLog = 0;
    inodo.numBloquesOcupados = 0;
    inodo.atime = time(NULL);
    inodo.mtime = time(NULL);
    inodo.ctime = time(NULL);
    memset(inodo.punterosDirectos, 0, sizeof(inodo.punterosDirectos));
    memset(inodo.punterosIndirectos, 0, sizeof(inodo.punterosIndirectos));

    if (escribir_inodo(posInodoReservado, &inodo) == FALLO) {
        perror("Error escribiendo inodo ");
        return FALLO;
    }

    SB.cantInodosLibres--;

    if (bwrite(posSB, &SB) == FALLO) { //Actualizamos el superbloque con la nueva cantidad de inodos libres
        perror("Error escribiendo SB ");
        return FALLO;
    }

    return posInodoReservado;
}

int obtener_nRangoBL(struct inodo* inodo, unsigned int nblogico, unsigned int* ptr) {
    if (nblogico < DIRECTOS) {
        *ptr = inodo->punterosDirectos[nblogico];
        return 0; //<12
    } else if (nblogico < INDIRECTOS0) {
        *ptr = inodo->punterosIndirectos[0];
        return 1; //268
    } else if (nblogico < INDIRECTOS1) {
        *ptr = inodo->punterosIndirectos[1];
        return 2; //65804
    } else if (nblogico < INDIRECTOS2) {
        *ptr = inodo->punterosIndirectos[2];
        return 3; // 16843020
    } else {
        *ptr = 0;
        perror("Bloque logico fuera de rango");
        return FALLO;
    }
}

int obtener_indice(unsigned int nblogico, int nivel_punteros) {
    if (nblogico < DIRECTOS) { //ej. nblogico=8
        return nblogico;
    } else if (nblogico < INDIRECTOS0) { //ej. nblogico=204
        return nblogico - DIRECTOS;
    } else if (nblogico < INDIRECTOS1) { // ej. nblogico=30004
        if (nivel_punteros == 2) {
            return(nblogico - INDIRECTOS0) / NPUNTEROS;
        } else if (nivel_punteros == 1) {
            return(nblogico - INDIRECTOS0) % NPUNTEROS;
        }
    } else if (nblogico < INDIRECTOS2) { // ej nblogico=400004
        if (nivel_punteros == 3) {
            return (nblogico - INDIRECTOS1) / (NPUNTEROS * NPUNTEROS);
        } else if (nivel_punteros == 2) {
            return (nblogico - INDIRECTOS1) % (NPUNTEROS * NPUNTEROS) / NPUNTEROS;
        } else if (nivel_punteros == 1) {
            return(nblogico - INDIRECTOS1) % (NPUNTEROS * NPUNTEROS) % NPUNTEROS;
        }
    }
    return FALLO;
}

int traducir_bloque_inodo(unsigned int ninodo, struct inodo* inodo, unsigned int nblogico, unsigned char reservar) {
    
    unsigned int ptr, ptr_ant;
    int nRangoBL, nivel_punteros, indice, cambios;
    unsigned int buffer[NPUNTEROS];
    ptr = 0, ptr_ant = 0; cambios = 0;

    nRangoBL = obtener_nRangoBL(inodo, nblogico, &ptr); //0:D,1:I0,2:I1,3:I2
    nivel_punteros = nRangoBL; // el nivel_punteros mas alto es el que cuelga directamnete del inodo
    while (nivel_punteros > 0) { // iterar para cada nivel de puneteros indirectos
        if (ptr == 0) { //no cuelgan bloques de punteros
            if (reservar == 0) { //bloque inexistente
                return FALLO;
            } else { // reservar bloques de punteros y crear enlaces desde le inodo hasta el bloque de datos
                cambios = 1;
                ptr = reservar_bloque(); //de punteros
                inodo->numBloquesOcupados++;
                inodo->ctime = time(NULL); //fecha actual
                if (nivel_punteros == nRangoBL) { // el bloque cuelga directamente del inodo
                    inodo->punterosIndirectos[nRangoBL - 1] = ptr;
                } else { //el bloque cuelga de otro bloque de punteros
                    buffer[indice] = ptr;
                    if (bwrite(ptr_ant, buffer) == -1) {
                        return FALLO;
                    } //salvamos en el dispositivo el biffer de punteros modificado
                }
                memset(buffer, 0, BLOCKSIZE); //ponemos a 0 todos los punteros del buffer
            }
        } else {
            if (bread(ptr, buffer) == -1) {
                return FALLO;
            } //leemos del dispositivo el bloque de punteros ya existente
        }
        indice = obtener_indice(nblogico, nivel_punteros);
        ptr_ant = ptr; //guardamos el puntero actual
        ptr = buffer[indice]; //y lo desplazamos al siguiente nivel
        nivel_punteros--;
    }// al salir de este bucle ya estamos al nivel de datos

    if (ptr == 0) { // no existe bloque de datos
        if (reservar == 0) { // error de lectura bloque inexistente
            return FALLO;
        } else {
            cambios = 1;
            ptr = reservar_bloque(); // de datos
            inodo->numBloquesOcupados++;
            inodo->ctime = time(NULL);
            if (nRangoBL == 0) { // si era un puntero directo
                inodo->punterosDirectos[nblogico] = ptr; // asignamos la dirección del bloque de datos en el inodo
            } else {
                buffer[indice] = ptr; // assignamos la direccion del bloque de datos en el buffer
                if (bwrite(ptr_ant, buffer) == -1) {
                    return FALLO;
                } // salvamos en el dispositivo el buffer de punteros modificado
            }
        }
    } //mi_write_f() se encargará de salvar los cambios del inodo en el disco
    if (cambios == 1) {
        if (escribir_inodo(ninodo, inodo) == -1) {
            return FALLO;
        };
    }
    return ptr; // numero de bloque fisico correspondiente al bloque de datos logico, nblogico
}

int traducir_bloque_inodo(unsigned int ninodo, unsigned int nblogico, unsigned char reservar) {

    
    unsigned int ptr, ptr_ant, salvar_inodo;
    int nRangoBL, nivel_punteros, indice;
    unsigned int buffer[NPUNTEROS];
    struct inodo inodo;

    ptr = 0, ptr_ant = 0, salvar_inodo = 0;

    if (leer_inodo(ninodo, &inodo) == FALLO) {
        perror("Error leyendo inodo");
        return FALLO;
    }

    // Determinamos rango del bloque lógico
    nRangoBL = obtener_nRangoBL(&inodo, nblogico, &ptr);  //0:D, 1:I0, 2:I1, 3:I2 
    nivel_punteros = nRangoBL; //el nivel_punteros +alto es el que cuelga directamente del inodo

    while (nivel_punteros > 0) {//iterar para cada nivel de punteros indirectos 
        if (ptr == 0) { //no cuelgan bloques de punteros
            if (reservar == 0) {// bloque inexistente 
                return FALLO;
            }

             //reservar bloques de punteros y crear enlaces desde el  inodo hasta el bloque de datos
            ptr = reservar_bloque();//de punteros  
            inodo.numBloquesOcupados++;
            inodo.ctime = time(NULL);//fecha actual
            salvar_inodo = 1;

            if (nivel_punteros == nRangoBL) {//el bloque cuelga directamente del inodo

                inodo.punterosIndirectos[nRangoBL - 1] = ptr;
                fprintf(stderr,
                "[traducir_bloque_inodo()→ inodo.punterosIndirectos[%d] = %d (reservado BF %d para punteros_nivel%d)]\n",
                nRangoBL - 1, ptr, ptr, nivel_punteros);

            } else { //el bloque cuelga de otro bloque de punteros 
                buffer[indice] = ptr;
                bwrite(ptr_ant, buffer);//salvamos en el dispositivo el buffer de punteros modificado  
                fprintf(stderr,
                "[traducir_bloque_inodo()→ punteros_nivel%d[%d] = %d (reservado BF %d para punteros_nivel%d)]\n",
                nivel_punteros + 1, indice, ptr, ptr, nivel_punteros);
            }

            memset(buffer, 0, BLOCKSIZE);//ponemos a 0 todos los punteros del buffer

        } else {
            bread(ptr, buffer);//leemos del dispositivo el bloque de punteros ya existente 
        }

        indice = obtener_indice(nblogico, nivel_punteros);
        ptr_ant = ptr;  //guardamos el puntero actual 
        ptr = buffer[indice]; // y lo desplazamos al siguiente nivel 
        nivel_punteros--;
    } //al salir de este bucle ya estamos al nivel de datos 

    if (ptr == 0) {//no existe bloque de datos
        if (reservar == 0) {
            return FALLO;
        }
        ptr = reservar_bloque();//de datos
        inodo.numBloquesOcupados++;
        inodo.ctime = time(NULL);
        salvar_inodo = 1;

        if (nRangoBL == 0) { //si era un puntero Directo
            inodo.punterosDirectos[nblogico] = ptr;//asignamos la direción del bl. de datos en el inodo 
            fprintf(stderr,
            "[traducir_bloque_inodo()→ inodo.punterosDirectos[%d] = %d (reservado BF %d para BL %d)]\n",
            nblogico, ptr, ptr, nblogico);

        } else {
            buffer[indice] = ptr;//asignamos la dirección del bloque de datos en el buffer
            bwrite(ptr_ant, buffer); //salvamos en el dispositivo el buffer de punteros modificado
            fprintf(stderr,
            "[traducir_bloque_inodo()→ punteros_nivel%d[%d] = %d (reservado BF %d para BL %d)]\n",
            nivel_punteros + 1, indice, ptr, ptr, nblogico);
        }
    }
    //salvar el inodo si se han hecho cambios y se desea no tener un big lock al usar semáforos 
    if (salvar_inodo) {
        escribir_inodo(ninodo, &inodo);  //nº de bloque físico correspondiente al bloque de datos lógico, nblogico 
    }

    return ptr;
}