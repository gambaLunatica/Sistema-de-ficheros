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


    unsigned int ninodo = SB.posPrimerInodoLibre;
    struct inodo inodos[BLOCKSIZE / INODOSIZE];

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

    bumount();

    return EXITO;
}