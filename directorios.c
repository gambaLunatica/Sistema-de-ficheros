/**
 * @author Juana Luna
 * @author Paola Chacín
 * @author Yassin EL Gharsa
 */

#include "directorios.h"

/**
 * Extrae el primer componente del camino, el tipo (directorio o fichero) y el resto del camino
 * @param camino El camino a analizar
 * @param inicial El primer componente del camino (sin el '/')
 * @param final El resto del camino (incluyendo el '/')
 * @param tipo 'd' si el primer componente es un directorio, 'f' si es un fichero
 * @return EXITO si se ha extraído correctamente, o un código de error negativo
 */
int extraer_camino(const char *camino, char *inicial, char *final, char *tipo)
{

    // El camino debe empezar por '/'
    if (camino[0] != '/')
    {
        return ERROR_CAMINO_INCORRECTO;
    }

    // Busca el siguiente '/' después del primer carácter
    char *p = strchr(camino + 1, '/');

    // Si se encuentra un '/', entonces el primer componente es un directorio, sino es un fichero
    if (p != NULL)
    {
        // Hay otro '/', por tanto inicial es directorio
        int len = p - (camino + 1);

        strncpy(inicial, camino + 1, len); // Copia el primer componente (entre el primer '/' y el siguiente '/')
        inicial[len] = '\0';               // Asegura que la cadena inicial esté terminada en null

        strcpy(final, p); // copiamos desde ese '/'

        *tipo = 'd';
    }
    else
    {
        // No hay más '/', es fichero final

        strcpy(inicial, camino + 1);
        strcpy(final, "");

        *tipo = 'f';
    }

    return EXITO;
}

// Usamos mi read y no bread porqeu un directorio es un fichero cuyos datos son estructuras entrada, así que se accede como fichero lógico.

/**
 * Busca una entrada en un directorio dado su camino parcial, y opcionalmente la reserva si no existe
 * @param camino_parcial El camino parcial a buscar
 * @param p_inodo_dir Puntero al inodo del directorio actual
 * @param p_inodo Puntero al inodo de la entrada encontrada o reservada (relativo al array de inodos)
 * @param p_entrada Puntero al número de entrada dentro del directorio donde se encuentra la entrada encontrada o reservada
 * @param reservar Si es 0, solo se busca la entrada. Si es 1, se reserva una nueva entrada si no existe.
 * @param permisos Permisos a asignar a la nueva entrada si se reserva (solo se tiene en cuenta si reservar es 1)
 * @return EXITO si se ha encontrado o reservado la entrada correctamente, o un código de error negativo
 */
int buscar_entrada(const char *camino_parcial, unsigned int *p_inodo_dir, unsigned int *p_inodo, unsigned int *p_entrada, char reservar, unsigned char permisos)
{
    struct inodo inodo_dir; // inodo del directorio actual
    struct entrada entrada; // entrada encontrada o a reservar
    struct superbloque SB;

    char inicial[sizeof(entrada.nombre)];
    char final[strlen(camino_parcial)];
    char tipo;

    int cant_entradas_inodo;
    int num_entrada_inodo = 0;
    // Leer SB
    if (bread(posSB, &SB) < 0)
    {
        fprintf(stderr, "Error en la lectura del superbloque\n");
        return FALLO;
    }

    // CASO BASE:si el camino es "/" devolvemos directamente la raíz
    if (strcmp(camino_parcial, "/") == 0)
    {
        *p_inodo = SB.posInodoRaiz;
        *p_entrada = 0;
        return EXITO;
    }

    // separa el camino en "inicial" y "final"
    if (extraer_camino(camino_parcial, inicial, final, &tipo) < 0)
    {
        return ERROR_CAMINO_INCORRECTO;
    }

    // Leemos el inodo del directorio actual
    if (leer_inodo(*p_inodo_dir, &inodo_dir) == FALLO)
    {
        return FALLO;
    }

    /// comprobamos permiso de lectura (bit 4)
    if ((inodo_dir.permisos & 4) != 4)
    {
        return ERROR_PERMISO_LECTURA;
    }

    // Número total de entradas del directorio
    cant_entradas_inodo = inodo_dir.tamEnBytesLog / sizeof(struct entrada);

    // Buscar entrada por nombre
    while (num_entrada_inodo < cant_entradas_inodo)
    {

        // leemos una entrada concreta
        if (mi_read_f(*p_inodo_dir, &entrada, num_entrada_inodo * sizeof(struct entrada), sizeof(struct entrada)) == FALLO)
        {
            return FALLO;
        }

        // Si coincide el nombre, encontrada
        if (strcmp(inicial, entrada.nombre) == 0)
        {
            break;
        }

        num_entrada_inodo++;
    }

    // Si no existe la entrada
    if (num_entrada_inodo == cant_entradas_inodo)
    {

        // Si solo queríamos consultar -> error
        if (reservar == 0)
        {
            return ERROR_NO_EXISTE_ENTRADA_CONSULTA;
        }

        // no se puede crear dentro de un fichero
        if (inodo_dir.tipo == 'f')
        {
            return ERROR_NO_SE_PUEDE_CREAR_ENTRADA_EN_UN_FICHERO;
        }

        // Hace falta permiso de escritura para crear
        if ((inodo_dir.permisos & 2) != 2)
        {
            return ERROR_PERMISO_ESCRITURA;
        }

        // Creamos nueva entrada
        strcpy(entrada.nombre, inicial);

        // Reservamos inodo según tipo
        if (tipo == 'd')
        {
            // si no estamos en el último nivel → error
            if (strcmp(final, "/") != 0)
            {
                return ERROR_NO_EXISTE_DIRECTORIO_INTERMEDIO;
            }
            entrada.ninodo = reservar_inodo('d', permisos); // reservamos inodo tipo directorio
        }
        else
        {
            entrada.ninodo = reservar_inodo('f', permisos); // reservamos inodo tipo fichero
        }

        if (entrada.ninodo == FALLO)
        {
            return FALLO;
        }

        // escribimos la entrada al final del directorio
        if (mi_write_f(*p_inodo_dir, &entrada, num_entrada_inodo * sizeof(struct entrada), sizeof(struct entrada)) == FALLO)
        {
            return FALLO;
        }
    }

    // si ya no queda más ruta
    if (strcmp(final, "") == 0 || strcmp(final, "/") == 0)
    {
        // si estamos creando y ya existía devuelve error
        if (num_entrada_inodo < cant_entradas_inodo && reservar == 1)
        {
            return ERROR_ENTRADA_YA_EXISTENTE;
        }
        *p_inodo = entrada.ninodo;
        *p_entrada = num_entrada_inodo;

        return EXITO;
    }

    // Si queda más camino seguimos buscando dentro del subdirectorio encontrado
    *p_inodo_dir = entrada.ninodo;

    return buscar_entrada(final, p_inodo_dir, p_inodo, p_entrada, reservar, permisos);
}
/**
 * Muestra un mensaje de error específico según el código de error devuelto por buscar_entrada
 * @param error El código de error devuelto por buscar_entrada
 */
void mostrar_error_buscar_entrada(int error)
{
    switch (error)
    {
    case -2:
        fprintf(stderr, RED "Error: Camino incorrecto.\n");
        break;
    case -3:
        fprintf(stderr, RED "Error: Permiso denegado de lectura.\n");
        break;
    case -4:
        fprintf(stderr, RED "Error: No existe el archivo o el directorio.\n");
        break;
    case -5:
        fprintf(stderr, RED "Error: No existe algún directorio intermedio.\n");
        break;
    case -6:
        fprintf(stderr, RED "Error: Permiso denegado de escritura.\n");
        break;
    case -7:
        fprintf(stderr, RED "Error: El archivo ya existe.\n");
        break;
    case -8:
        fprintf(stderr, RED "Error: No es un directorio.\n");
        break;
    }
    fprintf(stderr, WHITE);
}

/**
 * Crea un nuevo fichero o directorio según el camino especificado, con los permisos indicados
 * @param camino El camino del nuevo fichero o directorio a crear
 * @param permisos Los permisos a asignar al nuevo fichero o directorio (bit 4 para lectura, bit 2 para escritura, bit 1 para ejecución)
 * @return EXITO si se ha creado correctamente, o un código de error negativo si no se ha podido crear
 */
int mi_creat(const char *camino, unsigned char permisos)
{
    // Comprobamos que el modo de permisos es válido (entre 0 y 7)
    if (permisos > 7 || permisos < 0)
    {
        perror("Permisos inválidos");
        return FALLO;
    }

    unsigned int p_inodo_dir = 0; // raíz
    unsigned int p_inodo = 0;
    unsigned int p_entrada = 0;

    int error = buscar_entrada(camino, &p_inodo_dir, &p_inodo, &p_entrada, 1, permisos); // reservar = 1 para crear si no existe
    if (error < 0)
    {
        mostrar_error_buscar_entrada(error); // Mostrar mensaje de error específico
        return FALLO;
    }

    return EXITO;
}
/** Lista el contenido de un directorio dado su camino, guardando los nombres de las entradas en el buffer proporcionado
 * @param camino El camino del directorio a listar
 * @param buffer El buffer donde se guardarán los nombres de las entradas
 * @return El número de entradas listadas o un código de error negativo si no se ha podido listar
 */
int mi_dir(const char *camino, char *buffer)
{
    unsigned int p_inodo_dir = 0;
    unsigned int p_inodo = 0;
    unsigned int p_entrada = 0;
    char tamBytes[16];
    struct inodo inodo;
    struct entrada entrada;

    buffer[0] = '\0';

    // Buscar ruta
    int error = buscar_entrada(camino, &p_inodo_dir, &p_inodo, &p_entrada, 0, 0);

    if (error < 0)
        return error;

    // Leer inodo encontrado
    if (leer_inodo(p_inodo, &inodo) == FALLO)
    {
        return FALLO;
    }
    if (inodo.tipo != 'd')
    {
        return FALLO;
    }

    // Comprobar permiso lectura
    if ((inodo.permisos & 4) != 4)
    {
        return ERROR_PERMISO_LECTURA;
    }

    // Número de entradas
    int nentradas = inodo.tamEnBytesLog / sizeof(struct entrada);

    // Recorrer entradas
    for (int i = 0; i < nentradas; i++)
    {

        if (mi_read_f(p_inodo, &entrada, i * sizeof(struct entrada), sizeof(struct entrada)) == FALLO)
        {
            return FALLO;
        }
        if (inodo.tipo == 'd')
        {
            strcat(buffer, GREEN);
        }
        else
        {
            strcat(buffer, CYAN);
        }

        // Para cada entrada concatenamos su nombre al buffer e incorporamos la información del inodo
        sprintf(tipo, "%c", inodo.tipo);
        strcat(buffer, tipo);
        strcat(buffer, "\t\t");

        if ((inodo.permisos & 4) == 4)
        {
            strcat(buffer, "r");
        }
        else
        {
            strcat(buffer, "-");
        }

        if ((inodo.permisos & 2) == 2)
        {
            strcat(buffer, "w");
        }
        else
        {
            strcat(buffer, "-");
        }

        if ((inodo.permisos & 1) == 1)
        {
            strcat(buffer, "x");
        }
        else
        {
            strcat(buffer, "-");
        }

        strcat(buffer, "\t");

        struct tm *ts;
        char mtime[80];
        ts = localtime(&inodo.mtime);
        strftime(mtime, sizeof(mtime), "%a %Y-%m-%d %H:%M:%S", ts);
        strcat(buffer, mtime);
        sprintf(tamBytes, "\t\t%d", inodo.tamEnBytesLog);
        strcat(buffer, tamBytes);
        strcat(buffer, "\t\t");
        strcat(buffer, entrada.nombre);
        strcat(buffer, RESET);
        strcat(buffer, "\n");
    }

    return nentradas;
}