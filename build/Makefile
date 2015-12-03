# MAKE FILE

RM:= rm
CC:= gcc
CFLAGS := -Wall -g

OBJETOS = main.o

all : Subs_Server

Subs_Server : $(OBJETOS)
	$(CC) $(CFLAGS) $(OBJETOS) -o $@ -pthread
	@echo 'Creación exitosa'


# Reglas Generales

%.o : ../src/%.c
	$(CC) -c $(DEFS) $(CFLAGS) $< -o $@

.PHONY: all clean

clean : 
	$(RM) Subs_Server $(OBJETOS)

