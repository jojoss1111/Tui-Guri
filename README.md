# 📟 TUI Guri — Documentação Completa

> Biblioteca C para criar interfaces de terminal (TUI) com cores, caixas, menus e animações de texto.
> Funciona em **Linux**, **macOS** e **Windows** com suporte completo a UTF-8 e cores ANSI 256.

---

## Sumário

1. [Como funciona](#como-funciona)
2. [Começando — Exemplo mínimo](#começando--exemplo-mínimo)
3. [Renderer](#renderer)
4. [Interface](#interface)
5. [Inputs](#inputs)
6. [Cores](#cores)
7. [Constantes e Teclas](#constantes-e-teclas)
8. [Referência rápida](#referência-rápida)

---

## Como funciona

A TUI Guri é dividida em quatro módulos principais que trabalham juntos:

- **Renderer** — buffer de saída. Tudo que você quer exibir é escrito primeiro no buffer e depois enviado ao terminal de uma vez, evitando flickering.
- **Interface** — funções de desenho: caixas, texto, animações.
- **Inputs** — leitura de teclado, campo de texto e menus interativos.
- **Cores** — conversão de hex `#RRGGBB` para sequências ANSI 256.

O fluxo básico é sempre:

```
1. Criar Renderer e Interface/Inputs
2. Montar o que quer exibir (renderer_add, interface_draw, ...)
3. Chamar renderer_render() para de fato mostrar na tela
4. Destruir tudo ao final
```

---

## Começando — Exemplo mínimo

```c
#include "tui_api.h"

int main(void) {
    Renderer*  r  = renderer_create();
    Interface* ui = interface_create();
    Inputs*    in = inputs_create();

    // Limpa a tela
    clear_abs(r);

    // Desenha uma caixa com título e texto
    interface_draw(ui, r,
        2, 2,           // coluna, linha (posição na tela)
        5, 40,          // altura, largura
        "Olá Mundo",    // título da caixa
        "Bem-vindo à TUI Guri!\nUse as setas para navegar.",
        false,          // false = word-wrap automático
        "\033[40m",     // cor de fundo (preto)
        "\033[36m",     // cor da borda (ciano)
        "\033[97m"      // cor do texto (branco brilhante)
    );

    renderer_render(r);

    // Menu simples
    char* opcoes[] = { "Jogar", "Opções", "Sair" };
    int escolha = inputs_menu_selector_vertical(in, r,
        2, 9,           // posição do menu
        opcoes, 3,      // opções e quantidade
        "\033[40m", "\033[90m",   // fundo/texto normal
        "\033[44m", "\033[97m",   // fundo/texto selecionado
        "\033[42m", "\033[30m"    // fundo/texto ao confirmar
    );

    // Limpa e mostra resultado
    clear_abs(r);
    char msg[64];
    sprintf(msg, "Você escolheu a opção %d", escolha);
    interface_text_(ui, r, 2, 2, msg, "\033[97m", "");
    renderer_render(r);

    inputs_destroy(in);
    interface_destroy(ui);
    renderer_destroy(r);
    return 0;
}
```

**Compilando no Linux:**
```bash
gcc main.c tui_api.c -o meu_programa
./meu_programa
```

**Compilando no Windows (MinGW):**
```bash
gcc main.c tui_api.c -o meu_programa.exe
meu_programa.exe
```

---

## Renderer

O `Renderer` é o núcleo da biblioteca. Ele acumula todo o texto e sequências ANSI num buffer interno e só envia para o terminal quando você chama `renderer_render()`. Isso elimina o piscar (flickering) comum em aplicações TUI.

### Criação e destruição

```c
Renderer* renderer_create(void);
void      renderer_destroy(Renderer* r);
```

- `renderer_create` — aloca o buffer (64 KB inicial, expande automaticamente), ativa o raw mode no Linux/macOS e configura UTF-8 no Windows.
- `renderer_destroy` — libera o buffer e restaura o terminal ao estado original.

```c
Renderer* r = renderer_create();
// ... usa o renderer ...
renderer_destroy(r);
```

### Adicionar conteúdo ao buffer

```c
void renderer_add(Renderer* r, const char* content);
void renderer_add_raw(Renderer* r, const char* dados, size_t tamanho);
```

- `renderer_add` — adiciona uma string (com `\0` terminador) ao buffer.
- `renderer_add_raw` — adiciona exatamente `tamanho` bytes, útil para strings UTF-8 ou dados binários.

```c
renderer_add(r, "\033[32m");   // ativa cor verde
renderer_add(r, "Texto verde");
renderer_add(r, "\033[0m");    // reseta cores
```

### Mover o cursor

```c
void renderer_move_cursor(Renderer* r, int y, int x);
```

Adiciona ao buffer a sequência ANSI que move o cursor para a linha `y` e coluna `x`. **Atenção:** no terminal, linhas e colunas começam em `1`, não em `0`.

```c
renderer_move_cursor(r, 5, 10);  // move para linha 5, coluna 10
renderer_add(r, "Aqui!");
```

### Enviar para a tela

```c
void renderer_render(Renderer* r);
```

Escreve todo o conteúdo acumulado no terminal e zera o buffer. Chame sempre que quiser que o usuário veja as mudanças.

### Limpar a tela completamente

```c
void clear_abs(Renderer* r);
```

Apaga toda a tela, reseta as cores e move o cursor para o início. Internamente chama `renderer_render` automaticamente.

```c
clear_abs(r);  // tela limpa, pronta para começar
```

---

## Interface

O módulo `Interface` oferece funções de alto nível para desenhar elementos visuais: caixas com bordas, texto com animação de digitação, e texto simples posicionado.

### Criação e destruição

```c
Interface* interface_create(void);
void       interface_destroy(Interface* i);
```

```c
Interface* ui = interface_create();
// ...
interface_destroy(ui);
```

### Medir texto visível

```c
int interface_visible_len(const char* s);
```

Retorna o número de caracteres **visíveis** de uma string, ignorando sequências de escape ANSI. Útil para calcular alinhamento.

```c
const char* s = "\033[32mOlá\033[0m";
int len = interface_visible_len(s);  // retorna 3, não 14
```

### Mover cursor (via Interface)

```c
void interface_move_cursor(Renderer* r, int y, int x);
```

Equivalente a `renderer_move_cursor`. Existe nos dois módulos por conveniência.

### Limpar uma região

```c
void interface_clear(Interface* ui, Renderer* r,
                     int x, int y, int height, int width,
                     const char* bg_color);
```

Preenche com espaços a região `(x, y)` de tamanho `(width+2) × (height+2)` na cor de fundo especificada. Útil para apagar uma caixa antes de redesenhá-la.

| Parâmetro  | Descrição |
|------------|-----------|
| `x, y`     | Coluna e linha do canto superior esquerdo |
| `height`   | Altura da região |
| `width`    | Largura da região |
| `bg_color` | Sequência ANSI de fundo (ex: `"\033[40m"`). Se `NULL`, usa preto. |

```c
// Limpa uma área de 5 linhas × 30 colunas na posição (2, 2)
interface_clear(ui, r, 2, 2, 5, 30, "\033[40m");
renderer_render(r);
```

### Desenhar caixa com conteúdo

```c
void interface_draw(Interface* ui, Renderer* r,
                    int x, int y, int height, int width,
                    const char* title, const char* content, bool ascii_art,
                    const char* bg_color, const char* border_color, const char* text_color);
```

Desenha uma caixa com **borda dupla** (╔═╗║╚╝), título opcional no topo e conteúdo com quebra de linha automática.

| Parâmetro      | Descrição |
|----------------|-----------|
| `x, y`         | Posição do canto superior esquerdo |
| `height`       | Número de linhas de conteúdo (sem contar bordas) |
| `width`        | Largura interna em caracteres (sem contar bordas) |
| `title`        | Texto exibido no topo da borda. `NULL` para sem título. |
| `content`      | Texto do conteúdo. `NULL` ou `""` para caixa vazia. |
| `ascii_art`    | `true` = respeita `\n` sem quebrar palavras. `false` = word-wrap automático. |
| `bg_color`     | Cor de fundo ANSI. `NULL` = sem fundo. |
| `border_color` | Cor da borda ANSI. `NULL` = branco. |
| `text_color`   | Cor do texto ANSI. `NULL` = branco. |

```c
// Caixa simples
interface_draw(ui, r, 2, 2, 8, 40,
    "Status",
    "HP: 100/100\nMP: 50/50\nGold: 320",
    false,
    "\033[40m", "\033[33m", "\033[97m"
);
renderer_render(r);
```

```c
// Caixa com arte ASCII (respeita as quebras de linha exatas)
interface_draw(ui, r, 5, 1, 6, 20,
    NULL,
    "  /\\_/\\\n"
    " ( o.o )\n"
    "  > ^ <\n",
    true,   // ascii_art = true
    "\033[40m", "\033[35m", "\033[97m"
);
renderer_render(r);
```

### Caixa com texto animado (efeito máquina de escrever)

```c
void interface_drawspeak(Interface* ui, Renderer* r,
                         int x, int y, int height, int width,
                         const char* title, const char* texto,
                         const char* bg_color, const char* border_color, const char* text_color,
                         float speed);
```

Exibe o texto dentro de uma caixa caractere por caractere, simulando uma máquina de escrever. Quando o texto excede `height` linhas, aguarda uma tecla para exibir a próxima página. Pressionar qualquer tecla pula a animação da página atual.

| Parâmetro | Descrição |
|-----------|-----------|
| `speed`   | Tempo entre cada caractere em segundos. `0.03` é um bom valor. `0.0` exibe tudo de uma vez. |

```c
interface_drawspeak(ui, r, 2, 2, 4, 40,
    "NPC",
    "Olá, aventureiro! Bem-vindo à vila de Pedra Alta. "
    "Cuidado com os monstros na floresta ao norte.",
    "\033[40m", "\033[36m", "\033[97m",
    0.04f  // 40ms entre caracteres
);
```

### Caixa com linhas pré-formatadas

```c
void interface_drawline(Interface* ui, Renderer* r,
                        int x, int y, int height, int width,
                        const char* title, const char* text_line,
                        const char* bg_color, const char* border_color, const char* text_color,
                        const char* border_style);
```

Semelhante a `interface_draw`, mas respeita sempre os `\n` do texto (sem word-wrap) e permite escolher o estilo da borda.

| Parâmetro      | Valores de `border_style` |
|----------------|--------------------------|
| `"single"`     | Borda simples: ┌─┐│└┘ |
| Qualquer outro | Borda dupla: ╔═╗║╚╝ (padrão) |

```c
// Tabela simples com borda fina
interface_drawline(ui, r, 2, 2, 4, 30,
    "Inventário",
    "Espada      x1\nPoção       x3\nChave       x1\nGold        320",
    "\033[40m", "\033[37m", "\033[97m",
    "single"
);
renderer_render(r);
```

### Texto animado sem caixa

```c
void interface_text_speak(Interface* ui, Renderer* r,
                          int x, int y,
                          const char* texto, const char* bg_color, const char* text_color,
                          float speed);
```

Exibe texto animado diretamente na tela, sem caixa. `\n` avança para a linha seguinte mantendo a coluna `x`. Pressionar qualquer tecla pula o restante.

```c
interface_text_speak(ui, r, 5, 10,
    "Carregando...\nPor favor aguarde.",
    "\033[40m", "\033[93m",
    0.05f
);
```

### Texto estático posicionado

```c
void interface_text_(Interface* ui, Renderer* r,
                     int x, int y,
                     const char* texto, const char* text_color, const char* bg_color);
```

Renderiza texto multilinha na posição `(x, y)` sem animação. Cada `\n` avança uma linha mantendo a coluna `x`. Strings `NULL` ou vazias nas cores desativam a sequência de cor.

```c
interface_text_(ui, r, 3, 15,
    "Pressione ENTER para continuar\nou ESC para sair.",
    "\033[90m", ""
);
renderer_render(r);
```

---

## Inputs

O módulo `Inputs` gerencia a leitura do teclado: campo de texto livre, menus verticais e horizontais, e leitura de teclas não-bloqueante.

### Criação e destruição

```c
Inputs* inputs_create(void);
void    inputs_destroy(Inputs* input);
```

```c
Inputs* in = inputs_create();
// ...
inputs_destroy(in);
```

### Campo de texto

```c
char* inputs_prompt(Inputs* input, Renderer* r,
                    int x, int y, int max_len,
                    const char* input_color);
```

Exibe um cursor na posição `(x, y)` e aguarda o usuário digitar. Suporta backspace e caracteres UTF-8. Ao pressionar Enter, retorna a string digitada.

> **Importante:** A string retornada é alocada com `malloc`. Você deve chamar `free()` quando não precisar mais dela.

| Parâmetro     | Descrição |
|---------------|-----------|
| `max_len`     | Número máximo de caracteres visíveis. `0` usa 255. |
| `input_color` | Cor do texto digitado. `NULL` ou `""` para cor padrão. |

```c
renderer_add(r, "\033[97mNome do personagem: ");
renderer_render(r);

char* nome = inputs_prompt(in, r, 22, 5, 20, "\033[93m");
// usa o nome...
free(nome);
```

### Menu vertical

```c
int inputs_menu_selector_vertical(Inputs* input, Renderer* r,
                                  int x, int y,
                                  char** options, int count,
                                  const char* bg_normal, const char* fg_normal,
                                  const char* bg_select, const char* fg_select,
                                  const char* bg_correct, const char* fg_correct);
```

Exibe uma lista de opções navegável com as setas ↑↓. Retorna o índice da opção confirmada com Enter, ou `-1` se o usuário pressionar ESC.

| Parâmetro    | Descrição |
|--------------|-----------|
| `options`    | Array de strings com as opções |
| `count`      | Número de opções |
| `bg_normal / fg_normal` | Cores dos itens não selecionados |
| `bg_select / fg_select` | Cores do item em destaque |
| `bg_correct / fg_correct` | Cores do item ao confirmar (feedback visual) |

```c
char* opcoes[] = { "Nova Partida", "Carregar", "Opções", "Sair" };

int escolha = inputs_menu_selector_vertical(in, r,
    10, 5,
    opcoes, 4,
    "\033[40m", "\033[90m",    // normal: fundo preto, texto cinza
    "\033[44m", "\033[97m",    // selecionado: fundo azul, texto branco
    "\033[42m", "\033[30m"     // confirmado: fundo verde, texto preto
);

if (escolha == 3) {
    // Sair
}
```

### Menu horizontal

```c
int inputs_menu_selector_horizontal(Inputs* input, Renderer* r,
                                    int x, int y,
                                    char** options, int count,
                                    const char* bg_normal, const char* fg_normal,
                                    const char* bg_select, const char* fg_select,
                                    const char* bg_correct, const char* fg_correct);
```

Igual ao menu vertical, mas as opções ficam lado a lado e a navegação é com ←→.

```c
char* sim_nao[] = { "Sim", "Não" };

int resp = inputs_menu_selector_horizontal(in, r,
    15, 10,
    sim_nao, 2,
    "\033[40m", "\033[90m",
    "\033[41m", "\033[97m",
    "\033[42m", "\033[30m"
);
```

### Leitura de tecla sem bloqueio

```c
const char* inputs_get_key(void);
```

Retorna o nome da tecla pressionada como string, ou `""` se nenhuma tecla estiver disponível. **Não bloqueia** — ideal para uso em game loops.

| Retorno possível | Tecla |
|------------------|-------|
| `"UP"`           | Seta para cima |
| `"DOWN"`         | Seta para baixo |
| `"LEFT"`         | Seta para esquerda |
| `"RIGHT"`        | Seta para direita |
| `"ENTER"`        | Enter |
| `"BACKSPACE"`    | Backspace |
| `"ESC"`          | Escape |
| `"TAB"`          | Tab |
| `"SPACE"`        | Espaço |
| `"a"` … `"z"`   | Letra correspondente |
| `""`             | Nenhuma tecla pressionada |

```c
// Game loop simples
while (1) {
    const char* tecla = inputs_get_key();

    if (strcmp(tecla, "UP") == 0)    { /* move para cima */ }
    if (strcmp(tecla, "DOWN") == 0)  { /* move para baixo */ }
    if (strcmp(tecla, "ESC") == 0)   break;

    // desenha e renderiza...
    renderer_render(r);
    SLEEP_MS(16);  // ~60 fps
}
```

---

## Cores

A TUI Guri usa sequências ANSI 256 cores, geradas a partir de valores hexadecimais `#RRGGBB`. Isso permite usar qualquer cor sem memorizar códigos ANSI.

### Gravar em buffer externo

```c
void color_fg(char* buffer, const char* hex);  // cor do texto (foreground)
void color_bg(char* buffer, const char* hex);  // cor de fundo (background)
```

Escrevem a sequência ANSI no `buffer` fornecido. O buffer deve ter pelo menos `COLOR_STR_SIZE` (20) bytes.

```c
char fg[COLOR_STR_SIZE];
char bg[COLOR_STR_SIZE];

color_fg(fg, "#FF6600");  // laranja
color_bg(bg, "#1A1A2E");  // azul escuro

renderer_add(r, fg);
renderer_add(r, bg);
renderer_add(r, "Texto colorido");
renderer_add(r, C_RESET);
```

### Versões com buffer estático (mais convenientes)

```c
char* color_fg_s(const char* hex);
char* color_bg_s(const char* hex);
```

Retornam a sequência em um buffer estático rotativo (4 slots). **Não precisa alocar buffer**, mas não use mais de 4 valores ao mesmo tempo numa única expressão.

```c
renderer_add(r, color_fg_s("#00FF88"));  // verde neon
renderer_add(r, color_bg_s("#0D0D0D"));  // quase preto
renderer_add(r, "Texto estiloso");
renderer_add(r, C_RESET);
```

### Obter índice ANSI 256

```c
int color_hex_to_ansi_id(const char* hex);
```

Converte `#RRGGBB` para o índice inteiro (0–255) da paleta ANSI 256. Útil se você precisar montar sequências manualmente.

```c
int id = color_hex_to_ansi_id("#FF0000");  // ~196 (vermelho puro)
char seq[32];
sprintf(seq, "\033[38;5;%dm", id);
```

### Constantes prontas

Definidas em `tui_api.h` para uso rápido:

```c
C_RESET   // "\033[0m"  — reseta todas as cores e estilos
C_BOLD    // "\033[1m"  — texto em negrito
```

---

## Constantes e Teclas

### Códigos de tecla unificados

Usados internamente pelos menus. Disponíveis para uso próprio com `get_unified_key` (função interna):

```c
KEY_ENTER      // 13
KEY_BACKSPACE  //  8
KEY_ESC        // 27
KEY_UP         // 72
KEY_DOWN       // 80
KEY_LEFT       // 75
KEY_RIGHT      // 77
```

### Sleep multiplataforma

```c
SLEEP_MS(ms)  // dorme por `ms` milissegundos (funciona no Linux e Windows)
```

```c
SLEEP_MS(16);   // ~60 fps
SLEEP_MS(33);   // ~30 fps
SLEEP_MS(1000); // 1 segundo
```

---

## Referência rápida

### Renderer

| Função | O que faz |
|--------|-----------|
| `renderer_create()` | Cria o renderer e ativa raw mode |
| `renderer_destroy(r)` | Libera memória e restaura o terminal |
| `renderer_add(r, str)` | Adiciona string ao buffer |
| `renderer_add_raw(r, dados, n)` | Adiciona `n` bytes ao buffer |
| `renderer_move_cursor(r, y, x)` | Posiciona o cursor (linha, coluna) |
| `renderer_render(r)` | Envia o buffer para o terminal |
| `clear_abs(r)` | Limpa toda a tela |

### Interface

| Função | O que faz |
|--------|-----------|
| `interface_create()` | Cria o módulo de interface |
| `interface_destroy(ui)` | Libera o módulo |
| `interface_visible_len(s)` | Conta caracteres visíveis (ignora ANSI) |
| `interface_move_cursor(r, y, x)` | Posiciona cursor (mesma que renderer) |
| `interface_clear(ui, r, x, y, h, w, bg)` | Limpa uma região da tela |
| `interface_draw(...)` | Desenha caixa com borda dupla e texto |
| `interface_drawspeak(...)` | Caixa com texto animado + paginação |
| `interface_drawline(...)` | Caixa com linhas exatas, borda single/dupla |
| `interface_text_speak(...)` | Texto animado sem caixa |
| `interface_text_(...)` | Texto estático posicionado |

### Inputs

| Função | O que faz |
|--------|-----------|
| `inputs_create()` | Cria o módulo de inputs |
| `inputs_destroy(in)` | Libera o módulo |
| `inputs_prompt(...)` | Campo de texto. Retorna `char*` (fazer `free`) |
| `inputs_menu_selector_vertical(...)` | Menu com setas ↑↓. Retorna índice ou `-1` |
| `inputs_menu_selector_horizontal(...)` | Menu com setas ←→. Retorna índice ou `-1` |
| `inputs_get_key()` | Lê tecla sem bloquear. Retorna string do nome |

### Cores

| Função | O que faz |
|--------|-----------|
| `color_fg(buf, "#RRGGBB")` | Grava sequência foreground em buffer externo |
| `color_bg(buf, "#RRGGBB")` | Grava sequência background em buffer externo |
| `color_fg_s("#RRGGBB")` | Retorna sequência foreground (buffer estático) |
| `color_bg_s("#RRGGBB")` | Retorna sequência background (buffer estático) |
| `color_hex_to_ansi_id("#RRGGBB")` | Retorna índice ANSI 256 como `int` |
