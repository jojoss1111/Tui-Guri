# tui_api.pxd — Declarações Cython da API C (tui_api.h)
# Importado por tui.pyx como: cimport tui_api as _c

from libc.stddef cimport size_t

cdef extern from "tui_api.h":

    # ------------------------------------------------------------------
    # Structs
    # ------------------------------------------------------------------

    ctypedef struct Renderer:
        char*  buffer
        size_t capacity
        size_t size

    ctypedef struct Interface:
        const char* B_RESET
        const char* B_SPACE

    ctypedef struct Inputs:
        int last_key

    # ------------------------------------------------------------------
    # Renderer
    # ------------------------------------------------------------------

    Renderer* renderer_create()
    void      renderer_destroy(Renderer* r)
    void      renderer_add_raw(Renderer* r, const char* dados, size_t tamanho)
    void      renderer_add(Renderer* r, const char* content)
    void      renderer_move_cursor(Renderer* r, int y, int x)
    void      renderer_render(Renderer* r)
    void      clear_abs(Renderer* r)

    # ------------------------------------------------------------------
    # Interface
    # ------------------------------------------------------------------

    Interface* interface_create()
    void       interface_destroy(Interface* i)
    int        interface_visible_len(const char* s)
    void       interface_move_cursor(Renderer* r, int y, int x)
    void       interface_clear(Interface* ui, Renderer* r,
                               int x, int y, int height, int width,
                               const char* bg_color)
    void       interface_draw(Interface* ui, Renderer* r,
                              int x, int y, int height, int width,
                              const char* title, const char* content,
                              bint ascii_art,
                              const char* bg_color, const char* border_color,
                              const char* text_color)
    void       interface_drawspeak(Interface* ui, Renderer* r,
                                   int x, int y, int height, int width,
                                   const char* title, const char* texto,
                                   const char* bg_color, const char* border_color,
                                   const char* text_color, float speed)
    void       interface_drawline(Interface* ui, Renderer* r,
                                  int x, int y, int height, int width,
                                  const char* title, const char* text_line,
                                  const char* bg_color, const char* border_color,
                                  const char* text_color, const char* border_style)
    void       interface_text_speak(Interface* ui, Renderer* r,
                                    int x, int y,
                                    const char* texto, const char* bg_color,
                                    const char* text_color, float speed)
    void       interface_text_(Interface* ui, Renderer* r,
                               int x, int y,
                               const char* texto, const char* text_color,
                               const char* bg_color)

    # ------------------------------------------------------------------
    # Inputs
    # ------------------------------------------------------------------

    Inputs* inputs_create()
    void    inputs_destroy(Inputs* input)
    char*   inputs_prompt(Inputs* input, Renderer* r,
                          int x, int y, int max_len,
                          const char* input_color)
    int     inputs_menu_selector_vertical(Inputs* input, Renderer* r,
                                          int x, int y,
                                          char** options, int count,
                                          const char* bg_normal, const char* fg_normal,
                                          const char* bg_select, const char* fg_select,
                                          const char* bg_correct, const char* fg_correct)
    int     inputs_menu_selector_horizontal(Inputs* input, Renderer* r,
                                            int x, int y,
                                            char** options, int count,
                                            const char* bg_normal, const char* fg_normal,
                                            const char* bg_select, const char* fg_select,
                                            const char* bg_correct, const char* fg_correct)
    const char* inputs_get_key()

    # ------------------------------------------------------------------
    # Cores
    # ------------------------------------------------------------------

    int   color_hex_to_ansi_id(const char* hex)
    void  color_fg(char* buffer, const char* hex)
    void  color_bg(char* buffer, const char* hex)
    char* color_fg_s(const char* hex)
    char* color_bg_s(const char* hex)
