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
     int encontrada = 0; 

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
            encontrada = 1;
            break;
        }

        num_entrada_inodo++;
    }

    // Si no existe la entrada
    if (!encontrada)
    {

        // Si solo queríamos consultar
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
            // si falla, liberamos el inodo que acabamos de crear
            liberar_inodo(entrada.ninodo);
            return FALLO;
        }

        // la nueva entrada está en la última posición
        num_entrada_inodo = cant_entradas_inodo;
    }

    // si ya no queda más ruta
    if (strcmp(final, "") == 0 || strcmp(final, "/") == 0)
    {
        // si estamos creando y ya existía devuelve error
        if (encontrada && reservar == 1)
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
        
        // Leer inodo de la entrada
        struct inodo inodo_entrada;
        if (leer_inodo(entrada.ninodo, &inodo_entrada) == FALLO)
        {
            return FALLO;
        }
        
        if (inodo_entrada.tipo == 'd')
        {
            strcat(buffer, GREEN);
        }
        else
        {
            strcat(buffer, CYAN);
        }

        // Para cada entrada concatenamos su nombre al buffer e incorporamos la información del inodo
        char tipo_str[2];
        sprintf(tipo_str, "%c", inodo_entrada.tipo);
        strcat(buffer, tipo_str);
        strcat(buffer, "\t\t");

        if ((inodo_entrada.permisos & 4) == 4)
        {
            strcat(buffer, "r");
        }
        else
        {
            strcat(buffer, "-");
        }

        if ((inodo_entrada.permisos & 2) == 2)
        {
            strcat(buffer, "w");
        }
        else
        {
            strcat(buffer, "-");
        }

        if ((inodo_entrada.permisos & 1) == 1)
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
        ts = localtime(&inodo_entrada.mtime);
        strftime(mtime, sizeof(mtime), "%a %Y-%m-%d %H:%M:%S", ts);
        strcat(buffer, mtime);
        sprintf(tamBytes, "\t\t%d", inodo_entrada.tamEnBytesLog);
        strcat(buffer, tamBytes);
        strcat(buffer, "\t\t");
        strcat(buffer, entrada.nombre);
        strcat(buffer, RESET);
        strcat(buffer, "\n");
    }

    return nentradas;
}

/**  Cambia los permisos de un fichero o directorio dado su camino
 * @param camino El camino del fichero o directorio al que se le quieren cambiar los permisos
 * @param permisos Los nuevos permisos a asignar (bit 4 para lectura, bit 2 para escritura, bit 1 para ejecución)
 * @return EXITO si se han cambiado los permisos correctamente, o un código de error negativo si no se han podido cambiar
 */

int mi_chmod(const char *camino, unsigned char permisos)
{
    unsigned int p_inodo_dir = 0;
    unsigned int p_inodo = 0;
    unsigned int p_entrada = 0;

    // Buscar ruta
    int error = buscar_entrada(camino, &p_inodo_dir, &p_inodo, &p_entrada, 0, 0);

    if (error < 0) {
        return error;
    }

    // Cambiar permisos del inodo encontrado
    return mi_chmod_f(p_inodo, permisos);
}

/** Obtiene los datos del inodo asociado a una ruta
 * @param camino El camino del fichero o directorio del que se quieren obtener los datos
 * @param p_stat Puntero a la estructura donde se almacenarán los datos
 * @return EXITO si se han obtenido los datos correctamente, o un código de error negativo si no se han podido obtener
 */
int mi_stat(const char *camino, struct STAT *p_stat)
{
    unsigned int p_inodo_dir = 0;
    unsigned int p_inodo = 0;
    unsigned int p_entrada = 0;

    // Buscar la ruta
    int error = buscar_entrada(camino, &p_inodo_dir, &p_inodo, &p_entrada,0, 0);

    if (error < 0) {
        return error;
    }

    // Obtener metadata del inodo
    return mi_stat_f(p_inodo, p_stat);
}

//###############NIVEL 9 ################################

//Función para escribir contenido en un fichero, se busca la entrada dela camino con buscar entrada
#if (USARCACHE == 2 || USARCACHE == 3)
    static struct UltimaEntrada UltimasEntradas[CACHE_SIZE];
#elif (USARCACHE == 1)
    static struct UltimaEntrada UltimaEntradaEscritura;
#endif

int mi_write(const char *camino, const void *buf, unsigned int offset, unsigned int nbytes) {
    unsigned int p_inodo_dir = 0, p_inodo = 0, p_entrada = 0;
    int encontrado = -1;

    // --- Se busca la entrada del camino ---
#if (USARCACHE > 0)
    #if (USARCACHE == 1)
        if (strcmp(UltimaEntradaEscritura.camino, camino) == 0) {
            p_inodo = UltimaEntradaEscritura.p_inodo;
            encontrado = 0;
        }
    #elif (USARCACHE == 2 || USARCACHE == 3)
        for (int i = 0; i < CACHE_SIZE; i++) {
            if (strcmp(UltimasEntradas[i].camino, camino) == 0) {
                p_inodo = UltimasEntradas[i].p_inodo;
                encontrado = i;
                #if (USARCACHE == 3) // Actualizar sello de tiempo para LRU
                    gettimeofday(&UltimasEntradas[i].ultima_consulta, NULL);
                #endif
                break;
            }
        }
    #endif
#endif

    // --- Si no está se busca ---
    if (encontrado == -1) {
        int error = buscar_entrada(camino, &p_inodo_dir, &p_inodo, &p_entrada, 0, 0);
        if (error < 0) {
            mostrar_error_buscar_entrada(error);
            return error;
        }

        // --- Actualiza el cache ---
#if (USARCACHE == 1)
        strcpy(UltimaEntradaEscritura.camino, camino);
        UltimaEntradaEscritura.p_inodo = p_inodo;
#elif (USARCACHE == 3)
        // Buscar el hueco libre o la entrada más antigua (LRU)
        int indice_lru = 0;
        struct timeval min_time;
        gettimeofday(&min_time, NULL); // Tiempo actual

        for (int i = 0; i < CACHE_SIZE; i++) {
            // Si hay un hueco vacío (camino vacío)
            if (UltimasEntradas[i].camino[0] == '\0') {
                indice_lru = i;
                break;
            }
            // Si no, buscamos el que tenga el tiempo menor
            if (UltimasEntradas[i].ultima_consulta.tv_sec < min_time.tv_sec ||
               (UltimasEntradas[i].ultima_consulta.tv_sec == min_time.tv_sec && 
                UltimasEntradas[i].ultima_consulta.tv_usec < min_time.tv_usec)) {
                min_time = UltimasEntradas[i].ultima_consulta;
                indice_lru = i;
            }
        }
        // Reemplazar la entrada más antigua
        strcpy(UltimasEntradas[indice_lru].camino, camino);
        UltimasEntradas[indice_lru].p_inodo = p_inodo;
        gettimeofday(&UltimasEntradas[indice_lru].ultima_consulta, NULL);
#endif
    }

    // --- ESCRITURA FINAL ---
    return mi_write_f(p_inodo, buf, offset, nbytes);
}
int mi_read(const char *camino, void *buf, unsigned int offset, unsigned int nbytes) {
    unsigned int p_inodo_dir = 0, p_inodo = 0, p_entrada = 0;
    int indice_cache = -1;

    // --- 1. GESTIÓN DE CACHÉ DE LECTURA ---
#if (USARCACHE > 0)
    for (int i = 0; i < CACHE_SIZE; i++) {
        if (strcmp(UltimasEntradas[i].camino, camino) == 0) {
            p_inodo = UltimasEntradas[i].p_inodo;
            indice_cache = i;
            #if (USARCACHE == 3) // LRU: Actualizar marca de tiempo
                gettimeofday(&UltimasEntradas[i].ultima_consulta, NULL);
            #endif
            break;
        }
    }
#endif

    // --- 2. SI NO ESTÁ EN CACHÉ, BUSCAR ENTRADA ---
    if (indice_cache == -1) {
        int error = buscar_entrada(camino, &p_inodo_dir, &p_inodo, &p_entrada, 0, 0);
        if (error < 0) {
            return error; // El error ya indica si no existe o no hay permisos
        }

        // Actualizar la caché con la nueva entrada (Política LRU o FIFO)
        #if (USARCACHE == 3)
            int pos_reemplazo = 0;
            struct timeval min_time;
            gettimeofday(&min_time, NULL);

            for (int i = 0; i < CACHE_SIZE; i++) {
                if (UltimasEntradas[i].camino[0] == '\0') { // Hueco libre
                    pos_reemplazo = i;
                    break;
                }
                if (UltimasEntradas[i].ultima_consulta.tv_sec < min_time.tv_sec) {
                    min_time = UltimasEntradas[i].ultima_consulta;
                    pos_reemplazo = i;
                }
            }
            strcpy(UltimasEntradas[pos_reemplazo].camino, camino);
            UltimasEntradas[pos_reemplazo].p_inodo = p_inodo;
            gettimeofday(&UltimasEntradas[pos_reemplazo].ultima_consulta, NULL);
        #endif
    }

    // --- 3. LLAMADA A LA CAPA DE FICHEROS ---
    // mi_read_f se encarga de leer los bloques lógicos del inodo
    return mi_read_f(p_inodo, buf, offset, nbytes);
}

//##################### NIVEL 10 ########################
/** Crea un enlace duro desde camino2 a camino1, es decir, camino2 apuntará al mismo inodo que camino1
 * @param camino1 El camino del fichero original al que se quiere crear el enlace
 * @param camino2 El camino del nuevo enlace que se quiere crear
 * @return EXITO si se ha creado el enlace correctamente, o un código de error negativo si no se ha podido crear
 */
int mi_link(const char *camino1, const char *camino2) {

    // Variables necesarias para buscar entradas
    unsigned int p_inodo_dir1, p_inodo1, p_entrada1;
    unsigned int p_inodo_dir2, p_inodo2, p_entrada2;

    struct inodo inodo1;
    struct entrada entrada;

    // Buscamos camino1 
    int error = buscar_entrada(camino1, &p_inodo_dir1, &p_inodo1, &p_entrada1, 0, 0);
    if (error < 0) {
        mostrar_error_buscar_entrada(error);
        return FALLO;
    }

    //Leemos inodo1
    if (leer_inodo(p_inodo1, &inodo1) == FALLO) {
        return FALLO;
    }

    // Comprobamos que es fichero
    if (inodo1.tipo != 'f') {
        fprintf(stderr, "Error: no se permiten enlaces a directorios\n");
        return FALLO;
    }

    // Creamos el camino2 (no debe existir)
    error = buscar_entrada(camino2, &p_inodo_dir2, &p_inodo2, &p_entrada2, 1, 6); // reservar = 1 para crear si no existe, permisos rw- para el nuevo enlace
    if (error < 0) {
        mostrar_error_buscar_entrada(error);
        return FALLO;
    }

    //Leemos la entrada  creada
    int offset = p_entrada2 * sizeof(struct entrada);
    if (mi_read_f(p_inodo_dir2, &entrada, offset, sizeof(struct entrada)) == FALLO) {
        return FALLO;
    }

    // Asociamos al mismo inodo
    entrada.ninodo = p_inodo1;

    // Escribimos la entrada modificada
    if (mi_write_f(p_inodo_dir2, &entrada, offset, sizeof(struct entrada)) == FALLO) {
        return FALLO;
    }

    //Liberamos el inodo que se había creado automáticamente
    if (liberar_inodo(p_inodo2) == FALLO) {
        return FALLO;
    }

    //Incrementamos número de enlaces del inodo original
    inodo1.nlinks++;
    inodo1.ctime = time(NULL);

    //Guardar cambios en el inodo
    if (escribir_inodo(p_inodo1, &inodo1) == FALLO) {
        return FALLO;
    }

    return EXITO;
}

/** Elimina un enlace a un fichero o directorio dado su camino, y si es el último enlace, libera el inodo asociado
 * @param camino El camino del enlace que se quiere eliminar
 * @return EXITO si se ha eliminado el enlace correctamente, o un código de error negativo si no se ha podido eliminar
 */
int mi_unlink(const char *camino) {

    unsigned int p_inodo_dir, p_inodo, p_entrada;
    struct inodo inodo;
    struct entrada ultimaEntrada;
    int error = buscar_entrada(camino, &p_inodo_dir, &p_inodo, &p_entrada, 0, 0);
    //Buscamos la entrada
    if (error < 0) {
        mostrar_error_buscar_entrada(error);
        return FALLO;
    }

    //Leemos el inodo
    if (leer_inodo(p_inodo, &inodo) == FALLO) {
        return FALLO;
    }

    //Si es directorio tenemso que comprobar que esté vacío
    if (inodo.tipo == 'd' && inodo.tamEnBytesLog > 0) {
        fprintf(stderr, RED "El directorio %s no está vacío, no se puede borrar" RESET, camino);
        return FALLO;
    }

    // Leemos el inodo del directorio padre
    struct inodo inodo_dir;
    if (leer_inodo(p_inodo_dir, &inodo_dir) == FALLO) {
        return FALLO;
    }

    //Obtenemos el número total de entradas
    int totalEntradas = inodo_dir.tamEnBytesLog / sizeof(struct entrada);

    // Si no es la última, movemos la última
    if (p_entrada != totalEntradas - 1) {

        int offsetUltima = (totalEntradas - 1) * sizeof(struct entrada);

        // Leemos la última entrada a borrar en memora
        if (mi_read_f(p_inodo_dir, &ultimaEntrada, offsetUltima, sizeof(struct entrada)) == FALLO) {
            return FALLO;
        }

        //La escribimos en la posición de la que borramos
        int offset = p_entrada * sizeof(struct entrada);
        if (mi_write_f(p_inodo_dir, &ultimaEntrada, offset, sizeof(struct entrada)) == FALLO) {
            return FALLO;
        }
    }

    //Reducimos eltamaño del directorio
    if (mi_truncar_f(p_inodo_dir, (totalEntradas - 1) * sizeof(struct entrada)) == FALLO) {
        return FALLO;
    }
    //Reducimos los enlaces del inodo
    inodo.nlinks--;

    //Si no quedan enlaces entonces liberamos el  inodo
    if (inodo.nlinks == 0) {
        if (liberar_inodo(p_inodo) == FALLO) {
            return FALLO;
        }
    } else {
        inodo.ctime = time(NULL);
        if (escribir_inodo(p_inodo, &inodo) == FALLO) {
            return FALLO;
        }
    }

    return EXITO;
}