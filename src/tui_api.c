// tui_api.c

// Ativa extensões POSIX necessárias para sigaction, SIGWINCH, usleep
#ifndef _WIN32
#  define _POSIX_C_SOURCE 200809L
#  define _DEFAULT_SOURCE
#endif

#include "tui_api.h"

#ifndef _WIN32
#include <signal.h>
// Flag atômica: setada em 1 pelo handler SIGWINCH, zerada após o redraw
static volatile sig_atomic_t _tui_winch_flag = 0;
static void _tui_sigwinch_handler(int sig) { (void)sig; _tui_winch_flag = 1; }
#endif

// --- RENDERER ---

Renderer* renderer_create(void) {
    Renderer* r = (Renderer*)malloc(sizeof(Renderer));
    if (!r) return NULL;

#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
    r->hStdout = GetStdHandle(STD_OUTPUT_HANDLE);
    // Ativa processamento de sequências ANSI/VT no Windows 10+
    if (r->hStdout != INVALID_HANDLE_VALUE) {
        DWORD mode = 0;
        if (GetConsoleMode(r->hStdout, &mode))
            SetConsoleMode(r->hStdout, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
    }
#else
    enableRawMode();
    // Captura SIGWINCH para detectar redimensionamento do terminal
    struct sigaction sa;
    sa.sa_handler = _tui_sigwinch_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    sigaction(SIGWINCH, &sa, NULL);
#endif

    r->capacity     = 65536;
    r->buffer       = (char*)malloc(r->capacity);
    r->size         = 0;
    r->clip_enabled = false;
    r->cur_x        = 1;
    r->cur_y        = 1;

    if (!r->buffer) {
        fprintf(stderr, "Erro: Falha na alocacao do buffer.\n");
        free(r);
        return NULL;
    }
    return r;
}

void renderer_destroy(Renderer* r) {
    if (!r) return;
#ifndef _WIN32
    disableRawMode();
#endif
    free(r->buffer);
    free(r);
}

// Acrescenta bytes brutos ao buffer, expandindo se necessário
void renderer_add_raw(Renderer* r, const char* dados, size_t tamanho) {
    if (r->size + tamanho >= r->capacity) {
        while (r->size + tamanho >= r->capacity)
            r->capacity *= 2;

        char* temp = (char*)realloc(r->buffer, r->capacity);
        if (!temp) {
            fprintf(stderr, "Erro fatal: Falha ao expandir buffer.\n");
            exit(1);
        }
        r->buffer = temp;
    }
    memcpy(r->buffer + r->size, dados, tamanho);
    r->size += tamanho;
}

// Acrescenta string ao buffer
void renderer_add(Renderer* r, const char* content) {
    if (content) renderer_add_raw(r, content, strlen(content));
}

// Emite sequência ANSI de posicionamento do cursor.
// Se clipping estiver ativo, ignora movimentos fora da região definida —
// isso evita que textos maiores que a tela quebrem o layout.
void renderer_move_cursor(Renderer* r, int y, int x) {
    r->cur_x = x;
    r->cur_y = y;
    if (r->clip_enabled) {
        if (x < r->clip.x1 || x > r->clip.x2 ||
            y < r->clip.y1 || y > r->clip.y2)
            return;  // fora da região: descarta o movimento
    }
    char buf[32];
    int len = sprintf(buf, "\033[%d;%dH", y, x);
    renderer_add_raw(r, buf, len);
}

// Define a região de clipping. Somente coordenadas dentro de
// (x1,y1)–(x2,y2) serão efetivamente escritas no buffer.
// Útil para proteger o layout quando x+width ou y+height
// ultrapassam as dimensões reais do terminal.
void renderer_set_clip(Renderer* r, int x1, int y1, int x2, int y2) {
    if (!r) return;
    r->clip_enabled = true;
    r->clip.x1 = x1; r->clip.y1 = y1;
    r->clip.x2 = x2; r->clip.y2 = y2;
}

// Remove a região de clipping — volta a renderizar sem restrições.
void renderer_clear_clip(Renderer* r) {
    if (!r) return;
    r->clip_enabled = false;
}

// Esconde o cursor do terminal (ideal durante animações e menus).
void renderer_hide_cursor(Renderer* r) {
    renderer_add(r, "\033[?25l");
}

// Exibe o cursor do terminal.
void renderer_show_cursor(Renderer* r) {
    renderer_add(r, "\033[?25h");
}

// Retorna 1 se o terminal foi redimensionado desde a última chamada
// e zera a flag internamente. No Windows sempre retorna 0.
int renderer_was_resized(void) {
#ifndef _WIN32
    if (_tui_winch_flag) {
        _tui_winch_flag = 0;
        return 1;
    }
#endif
    return 0;
}

// Descarrega o buffer inteiro para o stdout e zera o tamanho
void renderer_render(Renderer* r) {
    if (!r || r->size == 0) return;

#ifdef _WIN32
    DWORD written = 0;
    WriteConsoleA(r->hStdout, r->buffer, (DWORD)r->size, &written, NULL);
#else
    if (write(STDOUT_FILENO, r->buffer, r->size) == -1) { /* ignora erro */ }
#endif

    r->size = 0;
}

// Limpa a tela completamente e reseta cores
void clear_abs(Renderer* r) {
    if (!r) return;
    renderer_add(r, "\033[0m\033[2J\033[H");
    renderer_render(r);
}

// --- INTERFACE — helpers internos ---

// Divide texto em linhas respeitando '\n' literais
static char** split_preserve_lines(const char* text, int* num_lines) {
    int capacity = 16;
    char** lines = malloc(capacity * sizeof(char*));
    *num_lines = 0;

    const char* start = text;
    const char* p     = text;

    while (*p) {
        if (*p == '\n') {
            size_t len  = p - start;
            char*  line = malloc(len + 1);
            memcpy(line, start, len);
            line[len] = '\0';

            if (*num_lines >= capacity) {
                capacity *= 2;
                lines = realloc(lines, capacity * sizeof(char*));
            }
            lines[(*num_lines)++] = line;
            start = p + 1;
        }
        p++;
    }
    // Última linha sem '\n' final
    if (p != start) {
        size_t len  = p - start;
        char*  line = malloc(len + 1);
        memcpy(line, start, len);
        line[len] = '\0';
        lines[(*num_lines)++] = line;
    }
    return lines;
}

// Quebra texto em linhas de até `width` caracteres visíveis, quebrando em espaços
static char** simple_word_wrap(const char* text, int width, int* num_lines) {
    int capacity = 16;
    char** lines = malloc(capacity * sizeof(char*));
    *num_lines = 0;

    const char* p = text;
    while (*p) {
        int         vis_len    = 0;
        const char* line_start = p;
        const char* last_space = NULL;

        while (*p && *p != '\n') {
            unsigned char c = (unsigned char)*p;
            // Conta apenas bytes iniciais de caractere UTF-8
            if ((c & 0xC0) != 0x80) {
                if (vis_len >= width) break;
                vis_len++;
            }
            if (*p == ' ') last_space = p;
            p++;
        }

        const char* line_end = p;
        // Se excedeu a largura, volta ao último espaço
        if (vis_len >= width && last_space && last_space > line_start) {
            line_end = last_space;
            p = last_space + 1;
        }

        size_t len  = line_end - line_start;
        char*  line = malloc(len + 1);
        memcpy(line, line_start, len);
        line[len] = '\0';

        if (*num_lines >= capacity) {
            capacity *= 2;
            lines = realloc(lines, capacity * sizeof(char*));
        }
        lines[(*num_lines)++] = line;

        if (*p == '\n') p++;
    }
    return lines;
}

// Libera array de linhas alocadas por split/wrap
static void free_wrapped_lines(char** lines, int count) {
    for (int i = 0; i < count; i++) free(lines[i]);
    free(lines);
}

// Retorna o tamanho em bytes de um caractere UTF-8 pelo byte inicial
static int get_utf8_char_len(unsigned char c) {
    if ((c & 0x80) == 0) return 1;
    if ((c & 0xE0) == 0xC0) return 2;
    if ((c & 0xF0) == 0xE0) return 3;
    if ((c & 0xF8) == 0xF0) return 4;
    return 1;
}

// --- INTERFACE — funções públicas ---

Interface* interface_create(void) {
    Interface* i = (Interface*)malloc(sizeof(Interface));
    if (!i) return NULL;
    i->B_RESET = "\033[0m";
    i->B_SPACE = " ";
    return i;
}

void interface_destroy(Interface* i) {
    if (i) free(i);
}

// Retorna o número de caracteres visíveis, ignorando sequências de escape ANSI
int interface_visible_len(const char* s) {
    if (!s) return 0;
    int    length    = 0;
    bool   in_escape = false;
    size_t n         = strlen(s);

    for (size_t i = 0; i < n; i++) {
        unsigned char c = (unsigned char)s[i];
        if (c == 27) {
            in_escape = true;
        } else if (in_escape) {
            if (c == 'm' || (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z'))
                in_escape = false;
        } else {
            if ((c & 0xC0) != 0x80) length++;
        }
    }
    return length;
}

void interface_move_cursor(Renderer* r, int y, int x) {
    char buf[32];
    int  len = sprintf(buf, "\033[%d;%dH", y, x);
    renderer_add_raw(r, buf, len);
}

// Preenche a região (x, y, width+2, height+2) com espaços na cor de fundo
void interface_clear(Interface* ui, Renderer* r, int x, int y,
                     int height, int width, const char* bg_color) {
    if (!bg_color) bg_color = "\033[40m";

    char* spaces = (char*)malloc(width + 3);
    memset(spaces, ' ', width + 2);
    spaces[width + 2] = '\0';

    for (int i = 0; i < height + 2; i++) {
        interface_move_cursor(r, y + i, x);
        renderer_add(r, bg_color);
        renderer_add(r, spaces);
        renderer_add(r, ui->B_RESET);
    }
    free(spaces);
}

// Desenha caixa com borda dupla, título opcional e conteúdo com word-wrap
// Se ascii_art=true, respeita '\n' sem word-wrap
void interface_draw(Interface* ui, Renderer* r,
                    int x, int y, int height, int width,
                    const char* title, const char* content, bool ascii_art,
                    const char* bg_color, const char* border_color, const char* text_color) {

    if (!bg_color)     bg_color     = "";
    if (!border_color) border_color = "\033[37m";
    if (!text_color)   text_color   = "\033[37m";

    // Caracteres de borda dupla (UTF-8)
    const char* TL = "\xE2\x95\x94";
    const char* H  = "\xE2\x95\x90";
    const char* TR = "\xE2\x95\x97";
    const char* V  = "\xE2\x95\x91";
    const char* BL = "\xE2\x95\x9A";
    const char* BR = "\xE2\x95\x9D";

    // Linha do topo
    interface_move_cursor(r, y, x);
    renderer_add(r, bg_color);
    renderer_add(r, border_color);
    renderer_add(r, TL);

    int tlen = title ? interface_visible_len(title) : 0;
    if (title && tlen > 0 && tlen + 2 < width) {
        renderer_add(r, " "); renderer_add(r, title); renderer_add(r, " ");
        for (int i = 0; i < width - tlen - 2; i++) renderer_add(r, H);
    } else {
        for (int i = 0; i < width; i++) renderer_add(r, H);
    }
    renderer_add(r, TR);
    renderer_add(r, ui->B_RESET);

    // Linhas de conteúdo
    int    num_lines = 0;
    char** lines     = NULL;
    if (content && *content) {
        lines = ascii_art
            ? split_preserve_lines(content, &num_lines)
            : simple_word_wrap(content, width, &num_lines);
    }

    for (int i = 0; i < height; i++) {
        interface_move_cursor(r, y + i + 1, x);
        renderer_add(r, bg_color);
        renderer_add(r, border_color);
        renderer_add(r, V);
        renderer_add(r, text_color);

        if (i < num_lines) {
            renderer_add(r, lines[i]);
            int pad = width - interface_visible_len(lines[i]);
            while (pad-- > 0) renderer_add(r, " ");
        } else {
            for (int j = 0; j < width; j++) renderer_add(r, " ");
        }
        renderer_add(r, border_color);
        renderer_add(r, V);
        renderer_add(r, ui->B_RESET);
    }

    if (lines) free_wrapped_lines(lines, num_lines);

    // Linha do rodapé
    interface_move_cursor(r, y + height + 1, x);
    renderer_add(r, bg_color);
    renderer_add(r, border_color);
    renderer_add(r, BL);
    for (int i = 0; i < width; i++) renderer_add(r, H);
    renderer_add(r, BR);
    renderer_add(r, ui->B_RESET);
}

// Exibe texto animado caractere por caractere dentro de uma caixa.
// Pagina automaticamente quando o conteúdo excede `height` linhas.
// Pressionar qualquer tecla pula a animação da página atual.
void interface_drawspeak(Interface* ui, Renderer* r,
                         int x, int y, int height, int width,
                         const char* title, const char* texto,
                         const char* bg_color, const char* border_color, const char* text_color,
                         float speed) {

    if (!bg_color)   bg_color   = "\033[40m";
    if (!text_color) text_color = "\033[37m";

    int    total_lines   = 0;
    char** wrapped_lines = simple_word_wrap(texto, width, &total_lines);
    unsigned int sleep_ms    = (unsigned int)(speed * 1000);
    bool         pular_anim  = false;

    for (int i = 0; i < total_lines; i += height) {
        pular_anim = false;

        // Desenha caixa vazia para a página atual
        interface_draw(ui, r, x, y, height, width, title, "",
                       false, bg_color, border_color, text_color);

        // Anima linha por linha dentro da página
        for (int l = 0; l < height && (i + l) < total_lines; l++) {
            const char* line    = wrapped_lines[i + l];
            size_t      line_n  = strlen(line);
            size_t      ci      = 0;

            interface_move_cursor(r, y + l + 1, x + 1);
            renderer_add(r, bg_color);
            renderer_add(r, text_color);
            renderer_render(r);

            while (ci < line_n) {
                unsigned char c        = (unsigned char)line[ci];
                int           char_len = get_utf8_char_len(c);
                renderer_add_raw(r, line + ci, char_len);
                renderer_render(r);
                ci += char_len;

                if (!pular_anim) {
                    if (_kbhit()) { _getch(); pular_anim = true; }
                    else SLEEP_MS(sleep_ms);
                }
            }
        }

        renderer_add(r, ui->B_RESET);
        renderer_render(r);

        // Aguarda tecla para avançar à próxima página
        if (i + height < total_lines) {
            while (!_kbhit()) SLEEP_MS(50);
            _getch();
        }
    }

    free_wrapped_lines(wrapped_lines, total_lines);
}

// Desenha caixa com borda simples ou dupla, exibindo linhas pré-formatadas.
// border_style="single" usa ─│┌┐└┘; qualquer outro valor usa borda dupla.
void interface_drawline(Interface* ui, Renderer* r,
                        int x, int y, int height, int width,
                        const char* title, const char* text_line,
                        const char* bg_color, const char* border_color, const char* text_color,
                        const char* border_style) {

    bool single = border_style && strcmp(border_style, "single") == 0;

    const char* TL = single ? "\xE2\x94\x8C" : "\xE2\x95\x94";
    const char* H  = single ? "\xE2\x94\x80" : "\xE2\x95\x90";
    const char* TR = single ? "\xE2\x94\x90" : "\xE2\x95\x97";
    const char* V  = single ? "\xE2\x94\x82" : "\xE2\x95\x91";
    const char* BL = single ? "\xE2\x94\x94" : "\xE2\x95\x9A";
    const char* BR = single ? "\xE2\x94\x98" : "\xE2\x95\x9D";

    if (!bg_color)     bg_color     = "";
    if (!border_color) border_color = "\033[37m";
    if (!text_color)   text_color   = "\033[37m";

    // Topo
    interface_move_cursor(r, y, x);
    renderer_add(r, bg_color); renderer_add(r, border_color); renderer_add(r, TL);
    int tlen = title ? interface_visible_len(title) : 0;
    if (title && tlen > 0 && tlen + 2 < width) {
        renderer_add(r, " "); renderer_add(r, title); renderer_add(r, " ");
        for (int i = 0; i < width - tlen - 2; i++) renderer_add(r, H);
    } else {
        for (int i = 0; i < width; i++) renderer_add(r, H);
    }
    renderer_add(r, TR); renderer_add(r, ui->B_RESET);

    // Conteúdo: text_line separado por '\n'
    int    nlines = 0;
    char** lines  = split_preserve_lines(text_line ? text_line : "", &nlines);

    for (int i = 0; i < height; i++) {
        interface_move_cursor(r, y + i + 1, x);
        renderer_add(r, bg_color); renderer_add(r, border_color); renderer_add(r, V);
        renderer_add(r, text_color);

        if (i < nlines) {
            renderer_add(r, lines[i]);
            int pad = width - interface_visible_len(lines[i]);
            while (pad-- > 0) renderer_add(r, " ");
        } else {
            for (int j = 0; j < width; j++) renderer_add(r, " ");
        }
        renderer_add(r, border_color); renderer_add(r, V); renderer_add(r, ui->B_RESET);
    }

    free_wrapped_lines(lines, nlines);

    // Rodapé
    interface_move_cursor(r, y + height + 1, x);
    renderer_add(r, bg_color); renderer_add(r, border_color); renderer_add(r, BL);
    for (int i = 0; i < width; i++) renderer_add(r, H);
    renderer_add(r, BR); renderer_add(r, ui->B_RESET);
}

// Exibe texto animado no terminal, fora de caixa.
// '\n' avança para a linha seguinte na mesma coluna x.
// Pressionar qualquer tecla pula o restante da animação.
void interface_text_speak(Interface* ui, Renderer* r,
                          int x, int y,
                          const char* texto, const char* bg_color, const char* text_color,
                          float speed) {

    if (!bg_color)   bg_color   = "\033[40m";
    if (!text_color) text_color = "\033[37m";

    unsigned int sleep_ms = (unsigned int)(speed * 1000);
    bool         pular    = false;
    size_t       n        = strlen(texto);

    interface_move_cursor(r, y, x);
    renderer_add(r, bg_color);
    renderer_add(r, text_color);
    renderer_render(r);

    size_t i = 0;
    while (i < n) {
        unsigned char c = (unsigned char)texto[i];

        if (c == '\n') {
            y++;
            interface_move_cursor(r, y, x);
            renderer_add(r, bg_color);
            renderer_add(r, text_color);
            renderer_render(r);
            i++;
            continue;
        }

        int char_len = get_utf8_char_len(c);
        renderer_add_raw(r, (char*)&texto[i], char_len);
        renderer_render(r);
        i += char_len;

        if (!pular) {
            if (_kbhit()) { _getch(); pular = true; }
            else SLEEP_MS(sleep_ms);
        }
    }
    renderer_add(r, ui->B_RESET);
    renderer_render(r);
}

// Renderiza texto multilinha na posição (x, y).
// Cada '\n' avança uma linha mantendo a coluna x.
void interface_text_(Interface* ui, Renderer* r,
                     int x, int y,
                     const char* texto, const char* text_color, const char* bg_color) {

    if (!bg_color)   bg_color   = "";
    if (!text_color) text_color = "";

    const char* start     = texto;
    const char* end;
    int         current_y = y;

    while ((end = strchr(start, '\n')) != NULL) {
        int len = (int)(end - start);
        interface_move_cursor(r, current_y, x);
        if (*bg_color)   renderer_add(r, bg_color);
        if (*text_color) renderer_add(r, text_color);
        renderer_add_raw(r, start, len);
        renderer_add(r, ui->B_RESET);
        start = end + 1;
        current_y++;
    }
    if (*start) {
        interface_move_cursor(r, current_y, x);
        if (*bg_color)   renderer_add(r, bg_color);
        if (*text_color) renderer_add(r, text_color);
        renderer_add(r, start);
        renderer_add(r, ui->B_RESET);
    }
}

// --- INPUTS — helpers internos ---

// Conta caracteres visíveis ignorando escape ANSI
static int inputs_visible_len(const char* s) {
    if (!s) return 0;
    int  l      = 0;
    bool in_esc = false;
    for (int i = 0; s[i] != '\0'; i++) {
        if (s[i] == '\x1b') {
            in_esc = true;
        } else if (in_esc) {
            if (isalpha((unsigned char)s[i])) in_esc = false;
        } else {
            if ((s[i] & 0xC0) != 0x80) l++;
        }
    }
    return l;
}

// Retorna true se o byte é continuação de sequência UTF-8
static bool is_utf8_continuation(char c) {
    return (c & 0xC0) == 0x80;
}

// Lê uma tecla e normaliza o código entre Windows e Linux
static int get_unified_key(void) {
    int ch = _getch();

    if (ch == 0 || ch == 0xE0) {   // prefixo de seta no Windows
        return _getch();
    }

#ifndef _WIN32
    // Sequência ESC [ X para setas no Linux/macOS
    if (ch == 27 && _kbhit()) {
        int next = _getch();
        if (next == 91) {
            int code = _getch();
            switch (code) {
                case 'A': return KEY_UP;
                case 'B': return KEY_DOWN;
                case 'C': return KEY_RIGHT;
                case 'D': return KEY_LEFT;
            }
        }
    }
#endif

    return ch;
}

// --- INPUTS — funções públicas ---

Inputs* inputs_create(void) {
    Inputs* inp = (Inputs*)malloc(sizeof(Inputs));
    if (inp) inp->last_key = 0;
    return inp;
}

void inputs_destroy(Inputs* input) {
    if (input) free(input);
}

// Lê texto digitado pelo usuário na posição (x, y).
// Retorna string alocada (caller deve fazer free).
// max_len=0 usa 255. Suporta backspace e UTF-8.
char* inputs_prompt(Inputs* input, Renderer* r,
                    int x, int y, int max_len, const char* input_color) {
    if (max_len <= 0) max_len = 255;

    renderer_move_cursor(r, y, x);
    if (input_color && *input_color) renderer_add(r, input_color);
    renderer_render(r);

    char* buffer       = (char*)calloc((max_len * 4) + 1, sizeof(char));
    int   current_bytes = 0;
    int   visual_len    = 0;

    while (true) {
        int ch = _getch();

        if (ch == KEY_ENTER || ch == 10) {
            // Limpa a área de input e retorna o buffer
            renderer_move_cursor(r, y, x);
            for (int k = 0; k < visual_len; k++) renderer_add(r, " ");
            renderer_move_cursor(r, y, x);
            renderer_add(r, "\033[0m");
            renderer_render(r);
            return buffer;
        }
        else if (ch == KEY_BACKSPACE || ch == 127) {
            if (current_bytes > 0) {
                // Remove o último caractere UTF-8 byte a byte
                do { current_bytes--; }
                while (current_bytes > 0 && is_utf8_continuation(buffer[current_bytes]));
                buffer[current_bytes] = '\0';
                visual_len--;
                renderer_add(r, "\b \b");
                renderer_render(r);
            }
        }
        else if (ch == 27) {
            // Descarta sequência de escape (ex: setas) sem poluir o input
            if (_kbhit()) {
                _getch();
                if (_kbhit()) _getch();
            }
        }
        else if (ch >= 32) {
            if (visual_len < max_len) {
                buffer[current_bytes++] = (char)ch;
                buffer[current_bytes]   = '\0';
                if (!is_utf8_continuation((char)ch)) visual_len++;
                char temp[2] = { (char)ch, '\0' };
                renderer_add(r, temp);
                renderer_render(r);
            }
        }
    }
}

// Menu vertical com setas ↑↓. Retorna índice selecionado ou -1 (ESC).
int inputs_menu_selector_vertical(Inputs* input, Renderer* r,
                                  int x, int y, char** options, int count,
                                  const char* bg_normal, const char* fg_normal,
                                  const char* bg_select, const char* fg_select,
                                  const char* bg_correct, const char* fg_correct) {

    if (!bg_normal) bg_normal = "\033[40m";
    if (!fg_normal) fg_normal = "\033[90m";
    if (!bg_select) bg_select = "\033[44m";
    if (!fg_select) fg_select = "\033[37m";
    if (!bg_correct) bg_correct = "\033[42m";
    if (!fg_correct) fg_correct = "\033[30m";

    // Calcula a largura máxima das opções para padding uniforme
    int max_len = 0;
    for (int i = 0; i < count; i++) {
        int l = inputs_visible_len(options[i]);
        if (l > max_len) max_len = l;
    }
    max_len += 4;

    int  current = 0;
    bool redraw  = true;

    renderer_hide_cursor(r);

    while (true) {
        // Redimensionamento detectado: força redraw imediato
        if (renderer_was_resized()) redraw = true;

        if (redraw) {
            for (int i = 0; i < count; i++) {
                renderer_move_cursor(r, y + i, x);
                if (i == current) {
                    renderer_add(r, bg_select); renderer_add(r, fg_select);
                    renderer_add(r, "\033[1m"); renderer_add(r, "> ");
                } else {
                    renderer_add(r, bg_normal); renderer_add(r, fg_normal);
                    renderer_add(r, "  ");
                }
                renderer_add(r, options[i]);

                int pad = max_len - (inputs_visible_len(options[i]) + 2);
                for (int p = 0; p < pad; p++) renderer_add(r, " ");
                renderer_add(r, "\033[0m");
            }
            renderer_render(r);
            redraw = false;
        }

        int ch = get_unified_key();

        if (ch == KEY_UP) {
            current = (current - 1 + count) % count;
            redraw  = true;
        } else if (ch == KEY_DOWN) {
            current = (current + 1) % count;
            redraw  = true;
        } else if (ch == KEY_ENTER || ch == 10) {
            // Destaca a opção confirmada e retorna
            renderer_move_cursor(r, y + current, x);
            renderer_add(r, bg_correct); renderer_add(r, fg_correct);
            renderer_add(r, "\033[1m"); renderer_add(r, "> ");
            renderer_add(r, options[current]);

            int pad = max_len - (inputs_visible_len(options[current]) + 2);
            for (int p = 0; p < pad; p++) renderer_add(r, " ");
            renderer_add(r, "\033[0m");
            renderer_render(r);

            SLEEP_MS(150);
            renderer_show_cursor(r);
            return current;
        } else if (ch == KEY_ESC) {
            renderer_show_cursor(r);
            return -1;
        }
    }
}

// Menu horizontal com setas ←→. Retorna índice selecionado ou -1 (ESC).
int inputs_menu_selector_horizontal(Inputs* input, Renderer* r,
                                    int x, int y, char** options, int count,
                                    const char* bg_normal, const char* fg_normal,
                                    const char* bg_select, const char* fg_select,
                                    const char* bg_correct, const char* fg_correct) {

    if (!bg_normal) bg_normal = "\033[40m";
    if (!fg_normal) fg_normal = "\033[90m";
    if (!bg_select) bg_select = "\033[44m";
    if (!fg_select) fg_select = "\033[37m";
    if (!bg_correct) bg_correct = "\033[42m";
    if (!fg_correct) fg_correct = "\033[30m";

    int  current = 0;
    bool redraw  = true;

    renderer_hide_cursor(r);

    while (true) {
        if (renderer_was_resized()) redraw = true;

        if (redraw) {
            renderer_move_cursor(r, y, x);
            for (int i = 0; i < count; i++) {
                if (i == current) {
                    renderer_add(r, bg_select); renderer_add(r, fg_select);
                    renderer_add(r, "\033[1m"); renderer_add(r, "> ");
                } else {
                    renderer_add(r, bg_normal); renderer_add(r, fg_normal);
                    renderer_add(r, "  ");
                }
                renderer_add(r, options[i]);
                renderer_add(r, "  ");
                renderer_add(r, "\033[0m");
            }
            renderer_render(r);
            redraw = false;
        }

        int ch = get_unified_key();

        if (ch == KEY_LEFT) {
            current = (current - 1 + count) % count;
            redraw  = true;
        } else if (ch == KEY_RIGHT) {
            current = (current + 1) % count;
            redraw  = true;
        } else if (ch == KEY_ENTER || ch == 10) {
            // Calcula posição x da opção selecionada para destacar
            int temp_x = x;
            for (int i = 0; i < current; i++)
                temp_x += 4 + inputs_visible_len(options[i]);

            renderer_move_cursor(r, y, temp_x);
            renderer_add(r, bg_correct); renderer_add(r, fg_correct);
            renderer_add(r, "\033[1m"); renderer_add(r, "> ");
            renderer_add(r, options[current]);
            renderer_add(r, "  ");
            renderer_add(r, "\033[0m");
            renderer_render(r);

            SLEEP_MS(150);
            renderer_show_cursor(r);
            return current;
        } else if (ch == KEY_ESC) {
            renderer_show_cursor(r);
            return -1;
        }
    }
}

// Lê uma tecla sem bloquear. Retorna string como "UP", "ENTER", letra, etc.
// Retorna "" se nenhuma tecla disponível.
const char* inputs_get_key(void) {
    static char key_buffer[16];

    if (!_kbhit()) return "";

    int ch = _getch();

    // Prefixo de seta no Windows (0x00 ou 0xE0)
    if (ch == 0 || ch == 0xE0) {
        ch = _getch();
        switch (ch) {
            case 72: return "UP";
            case 80: return "DOWN";
            case 75: return "LEFT";
            case 77: return "RIGHT";
            default: return "UNKNOWN";
        }
    }

    // Sequência ESC [ X no Linux/macOS
    if (ch == 27) {
        if (_kbhit()) {
            int next = _getch();
            if (next == 91 && _kbhit()) {
                int code = _getch();
                switch (code) {
                    case 'A': return "UP";
                    case 'B': return "DOWN";
                    case 'D': return "LEFT";
                    case 'C': return "RIGHT";
                }
            }
            return "ESC";
        }
        return "ESC";
    }

    if (ch == KEY_ENTER || ch == 10)        return "ENTER";
    if (ch == KEY_BACKSPACE || ch == 127)   return "BACKSPACE";
    if (ch == 9)                            return "TAB";
    if (ch == 32)                           return "SPACE";

    if (ch > 32 && ch < 127) {
        key_buffer[0] = (char)ch;
        key_buffer[1] = '\0';
        return key_buffer;
    }
    return "";
}

// --- TAMANHO DO TERMINAL ---

// Retorna o tamanho real do terminal (colunas × linhas).
// Em caso de falha, retorna {80, 24} como fallback seguro.
TermSize interface_get_term_size(void) {
    TermSize ts = { 80, 24 };  // fallback universal

#ifdef _WIN32
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
    if (h != INVALID_HANDLE_VALUE && GetConsoleScreenBufferInfo(h, &csbi)) {
        ts.width  = csbi.srWindow.Right  - csbi.srWindow.Left + 1;
        ts.height = csbi.srWindow.Bottom - csbi.srWindow.Top  + 1;
    }
#else
    struct winsize ws;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0 && ws.ws_col > 0) {
        ts.width  = (int)ws.ws_col;
        ts.height = (int)ws.ws_row;
    }
#endif

    return ts;
}

// Retorna a coluna x para centralizar horizontalmente uma caixa de `box_width` colunas.
// Considera as duas colunas de borda (+ 2) automaticamente.
int interface_center_x(int term_width, int box_width) {
    int total = box_width + 2;           // +2 pelas bordas esquerda e direita
    int x = (term_width - total) / 2 + 1; // terminais começam em coluna 1
    return x < 1 ? 1 : x;
}

// Retorna a linha y para centralizar verticalmente uma caixa de `box_height` linhas.
// Considera as duas linhas de borda (+ 2) automaticamente.
int interface_center_y(int term_height, int box_height) {
    int total = box_height + 2;
    int y = (term_height - total) / 2 + 1;
    return y < 1 ? 1 : y;
}

// --- CORES ---

// Converte string hex "#RRGGBB" para componentes inteiros r, g, b
static void _hex_to_rgb(const char* hex, int* r, int* g, int* b) {
    if (!hex) { *r = *g = *b = 0; return; }
    if (*hex == '#') hex++;
    if (strlen(hex) < 6) { *r = *g = *b = 0; return; }

    char r_str[3] = { hex[0], hex[1], '\0' };
    char g_str[3] = { hex[2], hex[3], '\0' };
    char b_str[3] = { hex[4], hex[5], '\0' };

    *r = (int)strtol(r_str, NULL, 16);
    *g = (int)strtol(g_str, NULL, 16);
    *b = (int)strtol(b_str, NULL, 16);
}

// --- ANSI 256 cores (fallback / compatibilidade) ---

// Mapeia valor de canal (0-255) para índice da paleta ANSI 256
static int _scale(int x) {
    if (x < 48)  return 0;
    if (x < 115) return 1;
    return (x - 35) / 40;
}

// Converte RGB para índice ANSI 256
static int _rgb_to_ansi256(int r, int g, int b) {
    return 16 + (36 * _scale(r)) + (6 * _scale(g)) + _scale(b);
}

// Retorna índice ANSI 256 correspondente à cor hex
int color_hex_to_ansi_id(const char* hex) {
    int r, g, b;
    _hex_to_rgb(hex, &r, &g, &b);
    return _rgb_to_ansi256(r, g, b);
}

// Escreve sequência ANSI 256 de foreground em buffer (>= COLOR_STR_SIZE bytes)
void color_fg(char* buffer, const char* hex) {
    snprintf(buffer, COLOR_STR_SIZE, "\033[38;5;%dm", color_hex_to_ansi_id(hex));
}

// Escreve sequência ANSI 256 de background em buffer (>= COLOR_STR_SIZE bytes)
void color_bg(char* buffer, const char* hex) {
    snprintf(buffer, COLOR_STR_SIZE, "\033[48;5;%dm", color_hex_to_ansi_id(hex));
}

// Retorna sequência ANSI 256 de foreground em buffer estático rotativo (4 slots)
char* color_fg_s(const char* hex) {
    static char bufs[4][COLOR_STR_SIZE];
    static int  idx = 0;
    char* b = bufs[idx];
    idx = (idx + 1) % 4;
    color_fg(b, hex);
    return b;
}

// Retorna sequência ANSI 256 de background em buffer estático rotativo (4 slots)
char* color_bg_s(const char* hex) {
    static char bufs[4][COLOR_STR_SIZE];
    static int  idx = 0;
    char* b = bufs[idx];
    idx = (idx + 1) % 4;
    color_bg(b, hex);
    return b;
}

// --- True Color (24-bit) — \033[38;2;R;G;Bm / \033[48;2;R;G;Bm ---
// Requer terminal com suporte a True Color (ex: kitty, alacritty, Windows Terminal,
// iTerm2, GNOME Terminal >= 3.16, tmux >= 2.2 com set-option -g default-terminal).
// Em terminais sem suporte, as sequências são ignoradas silenciosamente.

// Escreve sequência True Color de foreground em buffer (>= COLOR_STR_SIZE bytes)
void color_fg_true(char* buffer, const char* hex) {
    int r, g, b;
    _hex_to_rgb(hex, &r, &g, &b);
    snprintf(buffer, COLOR_STR_SIZE, "\033[38;2;%d;%d;%dm", r, g, b);
}

// Escreve sequência True Color de background em buffer (>= COLOR_STR_SIZE bytes)
void color_bg_true(char* buffer, const char* hex) {
    int r, g, b;
    _hex_to_rgb(hex, &r, &g, &b);
    snprintf(buffer, COLOR_STR_SIZE, "\033[48;2;%d;%d;%dm", r, g, b);
}

// Retorna sequência True Color de foreground em buffer estático rotativo (4 slots)
char* color_fg_true_s(const char* hex) {
    static char bufs[4][COLOR_STR_SIZE];
    static int  idx = 0;
    char* b = bufs[idx];
    idx = (idx + 1) % 4;
    color_fg_true(b, hex);
    return b;
}

// Retorna sequência True Color de background em buffer estático rotativo (4 slots)
char* color_bg_true_s(const char* hex) {
    static char bufs[4][COLOR_STR_SIZE];
    static int  idx = 0;
    char* b = bufs[idx];
    idx = (idx + 1) % 4;
    color_bg_true(b, hex);
    return b;
}