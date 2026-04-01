# tui.pyx — Wrapper Cython para tui_api
# Todas as funções e classes estão em português para facilitar o uso.

from libc.stdlib cimport malloc, free
from libc.string cimport strcpy, strlen
cimport tui_api as _c

# ---------------------------------------------------------------------------
# Helpers internos
# ---------------------------------------------------------------------------

cdef bytes _b(s):
    """Converte str Python → bytes UTF-8 (aceita bytes também)."""
    if s is None:
        return None
    if isinstance(s, bytes):
        return s
    return s.encode("utf-8")

cdef const char* _p(s):
    """Retorna ponteiro C para string Python/bytes, ou NULL se None."""
    if s is None:
        return NULL
    if isinstance(s, str):
        s = s.encode("utf-8")
    return <const char*>s


# ===========================================================================
# Classe Renderizador
# Responsável pelo buffer de saída e renderização no terminal.
# ===========================================================================
cdef class Renderizador:
    cdef _c.Renderer* _ptr

    def __cinit__(self):
        self._ptr = _c.renderer_create()
        if self._ptr is NULL:
            raise MemoryError("Falha ao criar Renderizador.")

    def __dealloc__(self):
        if self._ptr is not NULL:
            _c.renderer_destroy(self._ptr)
            self._ptr = NULL

    def adicionar_bruto(self, dados: bytes):
        """Adiciona bytes brutos ao buffer."""
        _c.renderer_add_raw(self._ptr, dados, len(dados))

    def adicionar(self, conteudo: str):
        """Adiciona uma string ao buffer."""
        cdef bytes b = _b(conteudo)
        _c.renderer_add(self._ptr, b)

    def mover_cursor(self, linha: int, coluna: int):
        """Move o cursor para (linha, coluna)."""
        _c.renderer_move_cursor(self._ptr, linha, coluna)

    def renderizar(self):
        """Descarrega o buffer no terminal."""
        _c.renderer_render(self._ptr)

    def limpar_tudo(self):
        """Limpa toda a tela e reseta as cores."""
        _c.clear_abs(self._ptr)


# ===========================================================================
# Classe Interface
# Desenha caixas, textos, animações e outros elementos visuais no terminal.
# ===========================================================================
cdef class Interface:
    cdef _c.Interface* _ptr

    def __cinit__(self):
        self._ptr = _c.interface_create()
        if self._ptr is NULL:
            raise MemoryError("Falha ao criar Interface.")

    def __dealloc__(self):
        if self._ptr is not NULL:
            _c.interface_destroy(self._ptr)
            self._ptr = NULL

    def comprimento_visivel(self, texto: str) -> int:
        """Retorna o número de caracteres visíveis (ignora escapes ANSI)."""
        cdef bytes b = _b(texto)
        return _c.interface_visible_len(b)

    def mover_cursor(self, renderizador: Renderizador, linha: int, coluna: int):
        """Move o cursor para (linha, coluna)."""
        _c.interface_move_cursor(renderizador._ptr, linha, coluna)

    def limpar(self, renderizador: Renderizador,
               x: int, y: int, altura: int, largura: int,
               cor_fundo: str = None):
        """
        Limpa uma região retangular com a cor de fundo indicada.

        Parâmetros
        ----------
        x, y        : posição do canto superior esquerdo
        altura      : número de linhas internas
        largura     : número de colunas internas
        cor_fundo   : sequência ANSI de cor de fundo (ex: '\\033[40m')
        """
        cdef bytes bf = _b(cor_fundo)
        _c.interface_clear(
            self._ptr, renderizador._ptr,
            x, y, altura, largura,
            _p(bf),
        )

    def desenhar_caixa(self, renderizador: Renderizador,
                       x: int, y: int, altura: int, largura: int,
                       titulo: str = None, conteudo: str = "",
                       ascii_art: bool = False,
                       cor_fundo: str = None,
                       cor_borda: str = None,
                       cor_texto: str = None):
        """
        Desenha uma caixa com borda dupla, título opcional e conteúdo.

        Parâmetros
        ----------
        x, y        : posição do canto superior esquerdo
        altura      : linhas internas da caixa
        largura     : colunas internas da caixa
        titulo      : texto exibido no topo da borda
        conteudo    : texto interno (word-wrap automático)
        ascii_art   : se True, respeita '\\n' sem word-wrap
        cor_fundo   : sequência ANSI de fundo
        cor_borda   : sequência ANSI de cor da borda
        cor_texto   : sequência ANSI de cor do texto
        """
        cdef bytes bt  = _b(titulo)
        cdef bytes bc  = _b(conteudo)
        cdef bytes bbg = _b(cor_fundo)
        cdef bytes bbr = _b(cor_borda)
        cdef bytes btx = _b(cor_texto)
        _c.interface_draw(
            self._ptr, renderizador._ptr,
            x, y, altura, largura,
            _p(bt), _p(bc), ascii_art,
            _p(bbg), _p(bbr), _p(btx),
        )

    def desenhar_caixa_animada(self, renderizador: Renderizador,
                               x: int, y: int, altura: int, largura: int,
                               titulo: str = None, texto: str = "",
                               cor_fundo: str = None,
                               cor_borda: str = None,
                               cor_texto: str = None,
                               velocidade: float = 0.03):
        """
        Exibe o conteúdo da caixa caractere por caractere (efeito "máquina de escrever").
        Pressionar qualquer tecla pula a animação da página atual.

        Parâmetros
        ----------
        velocidade  : segundos por caractere (ex: 0.03 = rápido, 0.1 = lento)
        """
        cdef bytes bt  = _b(titulo)
        cdef bytes bx  = _b(texto)
        cdef bytes bbg = _b(cor_fundo)
        cdef bytes bbr = _b(cor_borda)
        cdef bytes btx = _b(cor_texto)
        _c.interface_drawspeak(
            self._ptr, renderizador._ptr,
            x, y, altura, largura,
            _p(bt), _p(bx),
            _p(bbg), _p(bbr), _p(btx),
            velocidade,
        )

    def desenhar_linhas(self, renderizador: Renderizador,
                        x: int, y: int, altura: int, largura: int,
                        titulo: str = None, linhas: str = "",
                        cor_fundo: str = None,
                        cor_borda: str = None,
                        cor_texto: str = None,
                        estilo_borda: str = "double"):
        """
        Desenha uma caixa exibindo linhas pré-formatadas (separadas por '\\n').

        Parâmetros
        ----------
        estilo_borda : 'single' para borda simples; qualquer outro valor usa dupla
        """
        cdef bytes bt  = _b(titulo)
        cdef bytes bl  = _b(linhas)
        cdef bytes bbg = _b(cor_fundo)
        cdef bytes bbr = _b(cor_borda)
        cdef bytes btx = _b(cor_texto)
        cdef bytes be  = _b(estilo_borda)
        _c.interface_drawline(
            self._ptr, renderizador._ptr,
            x, y, altura, largura,
            _p(bt), _p(bl),
            _p(bbg), _p(bbr), _p(btx),
            _p(be),
        )

    def texto_animado(self, renderizador: Renderizador,
                      x: int, y: int,
                      texto: str,
                      cor_fundo: str = None,
                      cor_texto: str = None,
                      velocidade: float = 0.03):
        """
        Exibe texto no terminal com efeito "máquina de escrever", fora de caixa.
        '\\n' avança para a linha seguinte na mesma coluna x.
        Pressionar qualquer tecla pula o restante da animação.
        """
        cdef bytes bx  = _b(texto)
        cdef bytes bbg = _b(cor_fundo)
        cdef bytes btx = _b(cor_texto)
        _c.interface_text_speak(
            self._ptr, renderizador._ptr,
            x, y,
            _p(bx), _p(bbg), _p(btx),
            velocidade,
        )

    def texto(self, renderizador: Renderizador,
              x: int, y: int,
              texto: str,
              cor_texto: str = None,
              cor_fundo: str = None):
        """
        Renderiza texto multilinha na posição (x, y) sem animação.
        '\\n' avança uma linha mantendo a coluna x.
        """
        cdef bytes bx  = _b(texto)
        cdef bytes btx = _b(cor_texto)
        cdef bytes bbg = _b(cor_fundo)
        _c.interface_text_(
            self._ptr, renderizador._ptr,
            x, y,
            _p(bx), _p(btx), _p(bbg),
        )


# ===========================================================================
# Classe Entradas
# Lê teclado, exibe menus interativos e captura texto digitado pelo usuário.
# ===========================================================================
cdef class Entradas:
    cdef _c.Inputs* _ptr

    def __cinit__(self):
        self._ptr = _c.inputs_create()
        if self._ptr is NULL:
            raise MemoryError("Falha ao criar Entradas.")

    def __dealloc__(self):
        if self._ptr is not NULL:
            _c.inputs_destroy(self._ptr)
            self._ptr = NULL

    def prompt(self, renderizador: Renderizador,
               x: int, y: int,
               tamanho_maximo: int = 255,
               cor_entrada: str = None) -> str:
        """
        Lê texto digitado pelo usuário na posição (x, y).

        Parâmetros
        ----------
        tamanho_maximo : número máximo de caracteres visíveis (0 = 255)
        cor_entrada    : sequência ANSI de cor para o texto digitado

        Retorno
        -------
        str com o texto digitado pelo usuário.
        """
        cdef bytes bce = _b(cor_entrada)
        cdef char* resultado = _c.inputs_prompt(
            self._ptr, renderizador._ptr,
            x, y, tamanho_maximo,
            _p(bce),
        )
        if resultado is NULL:
            return ""
        py_str = resultado.decode("utf-8", errors="replace")
        free(resultado)
        return py_str

    def menu_vertical(self, renderizador: Renderizador,
                      x: int, y: int,
                      opcoes: list,
                      fundo_normal: str = None, texto_normal: str = None,
                      fundo_selecionado: str = None, texto_selecionado: str = None,
                      fundo_confirmado: str = None, texto_confirmado: str = None) -> int:
        """
        Exibe um menu de seleção vertical (↑↓ para navegar, Enter para confirmar).

        Parâmetros
        ----------
        opcoes : lista de strings com as opções do menu

        Retorno
        -------
        Índice da opção selecionada, ou -1 se ESC foi pressionado.
        """
        cdef int n = len(opcoes)
        cdef char** arr = <char**>malloc(n * sizeof(char*))
        if arr is NULL:
            raise MemoryError("Falha ao alocar menu vertical.")

        encoded = [o.encode("utf-8") for o in opcoes]
        for i in range(n):
            arr[i] = <char*>(<bytes>encoded[i])

        cdef bytes bbn = _b(fundo_normal)
        cdef bytes bfn = _b(texto_normal)
        cdef bytes bbs = _b(fundo_selecionado)
        cdef bytes bfs = _b(texto_selecionado)
        cdef bytes bbc = _b(fundo_confirmado)
        cdef bytes bfc = _b(texto_confirmado)

        resultado = _c.inputs_menu_selector_vertical(
            self._ptr, renderizador._ptr,
            x, y, arr, n,
            _p(bbn), _p(bfn),
            _p(bbs), _p(bfs),
            _p(bbc), _p(bfc),
        )
        free(arr)
        return resultado

    def menu_horizontal(self, renderizador: Renderizador,
                        x: int, y: int,
                        opcoes: list,
                        fundo_normal: str = None, texto_normal: str = None,
                        fundo_selecionado: str = None, texto_selecionado: str = None,
                        fundo_confirmado: str = None, texto_confirmado: str = None) -> int:
        """
        Exibe um menu de seleção horizontal (←→ para navegar, Enter para confirmar).

        Parâmetros
        ----------
        opcoes : lista de strings com as opções do menu

        Retorno
        -------
        Índice da opção selecionada, ou -1 se ESC foi pressionado.
        """
        cdef int n = len(opcoes)
        cdef char** arr = <char**>malloc(n * sizeof(char*))
        if arr is NULL:
            raise MemoryError("Falha ao alocar menu horizontal.")

        encoded = [o.encode("utf-8") for o in opcoes]
        for i in range(n):
            arr[i] = <char*>(<bytes>encoded[i])

        cdef bytes bbn = _b(fundo_normal)
        cdef bytes bfn = _b(texto_normal)
        cdef bytes bbs = _b(fundo_selecionado)
        cdef bytes bfs = _b(texto_selecionado)
        cdef bytes bbc = _b(fundo_confirmado)
        cdef bytes bfc = _b(texto_confirmado)

        resultado = _c.inputs_menu_selector_horizontal(
            self._ptr, renderizador._ptr,
            x, y, arr, n,
            _p(bbn), _p(bfn),
            _p(bbs), _p(bfs),
            _p(bbc), _p(bfc),
        )
        free(arr)
        return resultado

    @staticmethod
    def obter_tecla() -> str:
        """
        Lê uma tecla sem bloquear a execução.

        Retorno
        -------
        String com o nome da tecla: 'UP', 'DOWN', 'LEFT', 'RIGHT',
        'ENTER', 'BACKSPACE', 'TAB', 'SPACE', 'ESC', um caractere,
        ou '' se nenhuma tecla estiver disponível.
        """
        cdef const char* k = _c.inputs_get_key()
        if k is NULL or k[0] == b'\x00':
            return ""
        return k.decode("utf-8", errors="replace")


# ===========================================================================
# Funções de Cor
# Convertem cores hexadecimais em sequências ANSI.
# ===========================================================================

def cor_hex_para_id_ansi(hex_cor: str) -> int:
    """
    Converte uma cor '#RRGGBB' para o índice correspondente na paleta ANSI 256.

    Exemplo
    -------
    >>> cor_hex_para_id_ansi('#FF5733')
    202
    """
    cdef bytes b = _b(hex_cor)
    return _c.color_hex_to_ansi_id(b)


def cor_frente(buffer_saida: bytearray, hex_cor: str):
    """
    Escreve a sequência ANSI de cor de frente (foreground) em *buffer_saida*.
    O buffer deve ter pelo menos 20 bytes.
    """
    cdef bytes b = _b(hex_cor)
    _c.color_fg(<char*>(<bytearray>buffer_saida), b)


def cor_fundo(buffer_saida: bytearray, hex_cor: str):
    """
    Escreve a sequência ANSI de cor de fundo (background) em *buffer_saida*.
    O buffer deve ter pelo menos 20 bytes.
    """
    cdef bytes b = _b(hex_cor)
    _c.color_bg(<char*>(<bytearray>buffer_saida), b)


def sequencia_frente(hex_cor: str) -> str:
    """
    Retorna a sequência ANSI de foreground para a cor '#RRGGBB' indicada.

    Exemplo
    -------
    >>> sequencia_frente('#FF0000')
    '\\x1b[38;5;196m'
    """
    cdef bytes b = _b(hex_cor)
    cdef char* resultado = _c.color_fg_s(b)
    if resultado is NULL:
        return ""
    return resultado.decode("utf-8")


def sequencia_fundo(hex_cor: str) -> str:
    """
    Retorna a sequência ANSI de background para a cor '#RRGGBB' indicada.

    Exemplo
    -------
    >>> sequencia_fundo('#0000FF')
    '\\x1b[48;5;21m'
    """
    cdef bytes b = _b(hex_cor)
    cdef char* resultado = _c.color_bg_s(b)
    if resultado is NULL:
        return ""
    return resultado.decode("utf-8")
