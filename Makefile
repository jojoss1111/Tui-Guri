# Makefile — tui_api

CC      = gcc
CFLAGS  = -Wall -Wextra -std=c11 -O2
SRC     = tui_api.c
OBJ     = tui_api.o
LIB     = libtui.a

# Alvo padrão: compila a biblioteca estática
all: $(LIB)

# Compila o objeto
$(OBJ): $(SRC) tui_api.h
	$(CC) $(CFLAGS) -c $(SRC) -o $(OBJ)

# Empacota o objeto em biblioteca estática
$(LIB): $(OBJ)
	ar rcs $(LIB) $(OBJ)

# Compila um executável de exemplo (main.c deve existir)
example: $(LIB)
	$(CC) $(CFLAGS) main.c -L. -ltui -o example

# Remove artefatos gerados
clean:
	rm -f $(OBJ) $(LIB) example

.PHONY: all example clean