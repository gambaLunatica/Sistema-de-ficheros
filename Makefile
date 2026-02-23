CC=gcc
CFLAGS=-Wall -Wextra -std=gnu17 -g

SOURCES=mi_mkfs.c bloques.c

LIBRARIES=bloques.o #ficheros_basico.o ficheros.o directorios.o semaforo_mutex_posix.o 

INCLUDES=bloques.h #ficheros_basico.h ficheros.h directorios.h semaforo_mutex_posix.h simulacion.h 

PROGRAMS=mi_mkfs #leer_sf escribir leer truncar permitir mi_mkdir mi_chmod mi_ls mi_link mi_escribir mi_cat mi_stat mi_rm  simulacion verificacion 

OBJS=$(SOURCES:.c=.o) 

all: $(OBJS) $(PROGRAMS) 
$(PROGRAMS): $(LIBRARIES) $(INCLUDES) 
	$(CC) $(LDFLAGS) $(LIBRARIES) $@.o -o $@ -lm  
%.o: %.c $(INCLUDES) 
	$(CC) $(CFLAGS) -o $@ -c $< 
.PHONY: clean 
clean: 
	rm -rf *.o *~ $(PROGRAMS) disco* ext* 