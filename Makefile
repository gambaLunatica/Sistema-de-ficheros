CC=gcc
CFLAGS=-Wall -Wextra -std=gnu17 -g

SOURCES=mi_mkfs.c bloques.c leer_sf.c

LIBRARIES=bloques.o ficheros_basico.o 

INCLUDES=bloques.h ficheros_basico.h 

PROGRAMS=mi_mkfs leer_sf

OBJS=$(SOURCES:.c=.o) 

all: $(OBJS) $(PROGRAMS) 
$(PROGRAMS): $(LIBRARIES) $(INCLUDES) 
	$(CC) $(LDFLAGS) $(LIBRARIES) $@.o -o $@ -lm  
%.o: %.c $(INCLUDES) 
	$(CC) $(CFLAGS) -o $@ -c $< 
.PHONY: clean 
clean: 
	rm -rf *.o *~ $(PROGRAMS) disco* ext* 