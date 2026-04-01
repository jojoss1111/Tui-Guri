# =============================================================================
# Makefile — tui_api
#
#   make C-tui   → compila a biblioteca estática C  (libtui.a)
#   make P-tui   → compila o módulo Python/Cython    (tui.*.so)
#   make clean   → remove todos os artefatos gerados
# =============================================================================

CC      = gcc
CFLAGS  = -Wall -Wextra -std=c11 -O2
SRC     = tui_api.c
OBJ     = tui_api.o
LIB     = libtui.a

# -----------------------------------------------------------------------------
# C-tui: compila tui_api.c → objeto → biblioteca estática libtui.a
# -----------------------------------------------------------------------------
.PHONY: C-tui
C-tui: $(LIB)
	@echo "✔  Biblioteca C compilada: $(LIB)"

$(OBJ): $(SRC) tui_api.h
	$(CC) $(CFLAGS) -c $(SRC) -o $(OBJ)

$(LIB): $(OBJ)
	ar rcs $(LIB) $(OBJ)

# -----------------------------------------------------------------------------
# P-tui: gera o módulo Python/Cython  (tui.<platform>.so)
# Dependências: Cython e um compilador C instalados.
# -----------------------------------------------------------------------------
.PHONY: P-tui
P-tui:
	python setup.py build_ext --inplace
	@echo "✔  Módulo Python compilado. Importe com:  import tui"

# -----------------------------------------------------------------------------
# Compila um executável de exemplo (requer main.c)
# -----------------------------------------------------------------------------
.PHONY: example
example: C-tui
	$(CC) $(CFLAGS) main.c -L. -ltui -o example

# -----------------------------------------------------------------------------
# Limpeza
# -----------------------------------------------------------------------------
.PHONY: clean
clean:
	rm -f $(OBJ) $(LIB) example
	rm -f tui.c tui.*.so
	rm -rf build __pycache__ *.egg-info
	@echo "✔  Artefatos removidos."