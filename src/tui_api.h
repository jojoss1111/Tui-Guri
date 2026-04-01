#ifndef TUI_LIB_H
#define TUI_LIB_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include <wchar.h>

#ifdef _WIN32
    #include <windows.h>
    #include <conio.h>
#else
    #include <unistd.h>
    #include <termios.h>
    #include <sys/ioctl.h>
    #include <sys/select.h>
    #include <signal.h>
    #include <time.h>
#endif

// Tamanho máximo de string de cor ANSI (256 cores: 12 bytes | True Color: 19 bytes)
#define COLOR_STR_SIZE 24

// Tamanho real do terminal (colunas × linhas)
typedef struct {
    int width;   // número de colunas
    int height;  // número de linhas
} TermSize;
#define C_RESET "\033[0m"
#define C_BOLD  "\033[1m"

// Códigos de tecla unificados (Windows/Linux)
#define KEY_ENTER     13
#define KEY_BACKSPACE  8
#define KEY_ESC       27
#define KEY_UP        72
#define KEY_DOWN      80
#define KEY_LEFT      75
#define KEY_RIGHT     77

#ifdef _WIN32
    #define SLEEP_MS(ms) Sleep(ms)
#else
    #define SLEEP_MS(ms) usleep((ms) * 1000)
#endif

// Região de clipping ativa (coordenadas de terminal, 1-based)
// Quando clip_enabled=false, tudo é renderizado sem restrição
typedef struct {
    int x1, y1;   // canto superior esquerdo (inclusive)
    int x2, y2;   // canto inferior direito  (inclusive)
} ClipRect;

// Buffer de saída para renderização em lote
typedef struct {
    char*  buffer;
    size_t capacity;
    size_t size;
    // Clipping: se enabled, move_cursor filtra linhas/colunas fora dos limites
    bool     clip_enabled;
    ClipRect clip;
    // Posição atual do cursor (rastreada internamente para o clip)
    int cur_x, cur_y;
#ifdef _WIN32
    HANDLE hStdout;
#endif
} Renderer;

// Constantes de formatação da interface
typedef struct {
    const char* B_RESET;
    const char* B_SPACE;
} Interface;

// Estado de entrada do usuário
typedef struct {
    int last_key;
} Inputs;

// Posição e aparência de um jogador na tela
typedef struct {
    int x, y;
    const char* color;
    const char* symbol;
} PlayerInfo;

// --- Terminal raw mode (Linux/macOS apenas) ---

#ifndef _WIN32

static struct termios _tui_orig_termios;

// Restaura o terminal ao modo original
static inline void disableRawMode(void) {
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &_tui_orig_termios);
    printf("\033[?25h");
}

// Ativa raw mode: sem eco, sem buffer de linha, leitura não-bloqueante
static inline void enableRawMode(void) {
    tcgetattr(STDIN_FILENO, &_tui_orig_termios);
    atexit(disableRawMode);

    struct termios raw = _tui_orig_termios;
    raw.c_lflag &= ~(ECHO | ICANON);
    raw.c_cc[VMIN]  = 0;
    raw.c_cc[VTIME] = 0;
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

// Retorna 1 se há tecla disponível para leitura, sem bloquear
static inline int _kbhit(void) {
    struct timeval tv = { 0L, 0L };
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(0, &fds);
    return select(1, &fds, NULL, NULL, &tv) > 0;
}

// Lê um byte do stdin sem eco
static inline int _getch(void) {
    unsigned char c;
    if (read(STDIN_FILENO, &c, 1) < 0) return 0;
    return c;
}

#endif /* !_WIN32 */

// --- Renderer ---
Renderer* renderer_create(void);
void      renderer_destroy(Renderer* r);
void      renderer_add_raw(Renderer* r, const char* dados, size_t tamanho);
void      renderer_add(Renderer* r, const char* content);
void      renderer_move_cursor(Renderer* r, int y, int x);
void      renderer_render(Renderer* r);
void      clear_abs(Renderer* r);

// Clipping — limita o desenho a uma região segura do terminal
void      renderer_set_clip(Renderer* r, int x1, int y1, int x2, int y2);
void      renderer_clear_clip(Renderer* r);

// Cursor — controle explícito de visibilidade
void      renderer_hide_cursor(Renderer* r);
void      renderer_show_cursor(Renderer* r);

// SIGWINCH — verifica e consome flag de redimensionamento (Linux/macOS)
// Retorna 1 se o terminal foi redimensionado desde a última chamada, 0 caso contrário.
// No Windows sempre retorna 0.
int       renderer_was_resized(void);

// --- Interface ---
Interface* interface_create(void);
void       interface_destroy(Interface* i);
int        interface_visible_len(const char* s);
void       interface_move_cursor(Renderer* r, int y, int x);
void       interface_clear(Interface* ui, Renderer* r, int x, int y, int height, int width, const char* bg_color);
void       interface_draw(Interface* ui, Renderer* r, int x, int y, int height, int width,
                          const char* title, const char* content, bool ascii_art,
                          const char* bg_color, const char* border_color, const char* text_color);
void       interface_drawspeak(Interface* ui, Renderer* r, int x, int y, int height, int width,
                               const char* title, const char* texto,
                               const char* bg_color, const char* border_color, const char* text_color,
                               float speed);
void       interface_drawline(Interface* ui, Renderer* r, int x, int y, int height, int width,
                              const char* title, const char* text_line,
                              const char* bg_color, const char* border_color, const char* text_color,
                              const char* border_style);
void       interface_text_speak(Interface* ui, Renderer* r, int x, int y,
                                const char* texto, const char* bg_color, const char* text_color,
                                float speed);
void       interface_text_(Interface* ui, Renderer* r, int x, int y,
                           const char* texto, const char* text_color, const char* bg_color);

// --- Inputs ---
Inputs* inputs_create(void);
void    inputs_destroy(Inputs* input);
char*   inputs_prompt(Inputs* input, Renderer* r, int x, int y, int max_len, const char* input_color);
int     inputs_menu_selector_vertical(Inputs* input, Renderer* r, int x, int y,
                                      char** options, int count,
                                      const char* bg_normal, const char* fg_normal,
                                      const char* bg_select, const char* fg_select,
                                      const char* bg_correct, const char* fg_correct);
int     inputs_menu_selector_horizontal(Inputs* input, Renderer* r, int x, int y,
                                        char** options, int count,
                                        const char* bg_normal, const char* fg_normal,
                                        const char* bg_select, const char* fg_select,
                                        const char* bg_correct, const char* fg_correct);
const char* inputs_get_key(void);

// --- Terminal ---
TermSize interface_get_term_size(void);
int      interface_center_x(int term_width,  int box_width);
int      interface_center_y(int term_height, int box_height);

// --- Cores ---
int   color_hex_to_ansi_id(const char* hex);
void  color_fg(char* buffer, const char* hex);
void  color_bg(char* buffer, const char* hex);
char* color_fg_s(const char* hex);
char* color_bg_s(const char* hex);

// True Color (24-bit) — sequências \033[38;2;R;G;Bm / \033[48;2;R;G;Bm
void  color_fg_true(char* buffer, const char* hex);   // grava em buffer externo (>=24 bytes)
void  color_bg_true(char* buffer, const char* hex);
char* color_fg_true_s(const char* hex);               // buffer estático rotativo (4 slots)
char* color_bg_true_s(const char* hex);

#endif /* TUI_LIB_H */