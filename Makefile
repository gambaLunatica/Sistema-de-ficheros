CC=gcc
CFLAGS=-c -g -Wall -std=gnu17
LDFLAGS=-pthread

# Solo los archivos que existen físicamente
SOURCES=bloques.c mi_mkfs.c ficheros_basico.c ficheros.c leer_sf.c escribir.c leer.c permitir.c truncar.c
LIBRARIES=bloques.o ficheros_basico.o ficheros.o
INCLUDES=bloques.h ficheros_basico.h ficheros.h 
PROGRAMS=mi_mkfs leer_sf escribir leer permitir truncar
OBJS=$(SOURCES:.c=.o)

all: $(OBJS) $(PROGRAMS)

$(PROGRAMS): $(LIBRARIES) $(INCLUDES)
	$(CC) $(LDFLAGS) $(LIBRARIES) $@.o -o $@

%.o: %.c $(INCLUDES)
	$(CC) $(CFLAGS) -o $@ -c $<

.PHONY: clean
clean:
	rm -rf *.o *~ $(PROGRAMS) disco* ext*