# 📟 TUI Guri — Documentação Completa

> Biblioteca C para criar interfaces de terminal (TUI) com cores, caixas, menus e animações de texto.
> Funciona em **Linux**, **macOS** e **Windows** com suporte completo a UTF-8 e cores ANSI 256.
> Pode ser usada diretamente em **C** ou como **módulo Python** via Cython.

---

## Sumário

1. [Como funciona](#como-funciona)
2. [Compilando](#compilando)
3. [Começando em C — Exemplo mínimo](#começando-em-c--exemplo-mínimo)
4. [Começando em Python — Exemplo mínimo](#começando-em-python--exemplo-mínimo)
5. [Renderer / Renderizador](#renderer--renderizador)
6. [Interface](#interface)
7. [Inputs / Entradas](#inputs--entradas)
8. [Cores](#cores)
9. [Constantes e Teclas](#constantes-e-teclas)
10. [Referência rápida — C](#referência-rápida--c)
11. [Referência rápida — Python](#referência-rápida--python)

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

## Compilando

O projeto usa um `Makefile` com dois alvos principais:

### `make C-tui` — Biblioteca estática C

Compila `tui_api.c` e gera `libtui.a`. Use para linkar em projetos C.

```bash
make C-tui
```

Para compilar e rodar um programa C diretamente:

```bash
# Linux / macOS
gcc main.c tui_api.c -o meu_programa
./meu_programa

# Windows (MinGW)
gcc main.c tui_api.c -o meu_programa.exe
meu_programa.exe

# Linkando com a biblioteca estática (após make C-tui)
gcc main.c -L. -ltui -o meu_programa
```

---

### `make P-tui` — Módulo Python (Cython)

Compila o wrapper Cython e gera `tui.*.so` (Linux/macOS) ou `tui.*.pyd` (Windows), que pode ser importado diretamente no Python.

**Pré-requisitos:**
```bash
pip install cython setuptools
```

**Compilando:**
```bash
make P-tui
```

Isso roda internamente:
```bash
python setup.py build_ext --inplace
```

Após compilar, importe normalmente:
```python
import tui
```

> O arquivo `.so`/`.pyd` gerado deve estar na mesma pasta do seu script Python,
> ou em um diretório que esteja no `sys.path`.

---

### Limpeza

```bash
make clean   # remove libtui.a, tui.*.so, build/, __pycache__ e demais artefatos
```

---

## Começando em C — Exemplo mínimo

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

---

## Começando em Python — Exemplo mínimo

```python
import tui

r  = tui.Renderizador()
ui = tui.Interface()
en = tui.Entradas()

# Limpa a tela
r.limpar_tudo()

# Desenha uma caixa com título e texto
ui.desenhar_caixa(r,
    x=2, y=2,
    altura=5, largura=40,
    titulo="Olá Mundo",
    conteudo="Bem-vindo à TUI Guri!\nUse as setas para navegar.",
    ascii_art=False,
    cor_fundo="\033[40m",
    cor_borda="\033[36m",
    cor_texto="\033[97m",
)
r.renderizar()

# Menu simples
opcoes = ["Jogar", "Opções", "Sair"]
escolha = en.menu_vertical(r,
    x=2, y=9,
    opcoes=opcoes,
    fundo_normal="\033[40m",    texto_normal="\033[90m",
    fundo_selecionado="\033[44m", texto_selecionado="\033[97m",
    fundo_confirmado="\033[42m",  texto_confirmado="\033[30m",
)

# Limpa e mostra resultado
r.limpar_tudo()
ui.texto(r, x=2, y=2,
    texto=f"Você escolheu: {opcoes[escolha]}",
    cor_texto="\033[97m",
)
r.renderizar()
```

---

## Renderer / Renderizador

O `Renderer` é o núcleo da biblioteca. Ele acumula todo o texto e sequências ANSI num buffer interno e só envia para o terminal quando você chama `renderer_render()` / `.renderizar()`. Isso elimina o piscar (flickering) comum em aplicações TUI.

### Criação

```c
// C
Renderer* r = renderer_create();
renderer_destroy(r);
```

```python
# Python
r = tui.Renderizador()
# destruído automaticamente pelo garbage collector
```

`renderer_create` / `Renderizador()` — aloca o buffer (64 KB inicial, expande automaticamente), ativa o raw mode no Linux/macOS e configura UTF-8 no Windows.

### Adicionar conteúdo ao buffer

```c
// C
void renderer_add(Renderer* r, const char* content);
void renderer_add_raw(Renderer* r, const char* dados, size_t tamanho);
```

```python
# Python
r.adicionar("texto ou \033[32msequência ANSI\033[0m")
r.adicionar_bruto(b"\x1b[32m")   # bytes brutos
```

### Mover o cursor

```c
// C — (y=linha, x=coluna)
renderer_move_cursor(r, 5, 10);
```

```python
# Python — (linha, coluna)
r.mover_cursor(5, 10)
```

> **Atenção:** no terminal, linhas e colunas começam em `1`, não em `0`.

### Enviar para a tela

```c
// C
renderer_render(r);
```

```python
# Python
r.renderizar()
```

Descarrega o buffer inteiro no terminal e o zera. Chame sempre que quiser que o usuário veja as mudanças.

### Limpar a tela completamente

```c
// C
clear_abs(r);
```

```python
# Python
r.limpar_tudo()
```

Apaga toda a tela, reseta as cores e move o cursor para o início.

---

## Interface

O módulo `Interface` oferece funções de alto nível para desenhar elementos visuais: caixas com bordas, texto com animação de digitação, e texto simples posicionado.

### Criação

```c
// C
Interface* ui = interface_create();
interface_destroy(ui);
```

```python
# Python
ui = tui.Interface()
```

### Medir texto visível

```c
// C
int len = interface_visible_len("\033[32mOlá\033[0m");  // retorna 3
```

```python
# Python
tamanho = ui.comprimento_visivel("\033[32mOlá\033[0m")  # retorna 3
```

Retorna o número de caracteres visíveis, ignorando sequências ANSI.

### Limpar uma região

```c
// C
interface_clear(ui, r, x, y, height, width, "\033[40m");
renderer_render(r);
```

```python
# Python
ui.limpar(r, x=2, y=2, altura=5, largura=30, cor_fundo="\033[40m")
r.renderizar()
```

Preenche com espaços a região `(x, y)` de tamanho `(largura+2) × (altura+2)` na cor de fundo especificada. Útil para apagar uma caixa antes de redesenhá-la.

### Desenhar caixa com conteúdo

```c
// C
interface_draw(ui, r,
    2, 2, 8, 40,
    "Status",
    "HP: 100/100\nMP: 50/50\nGold: 320",
    false,
    "\033[40m", "\033[33m", "\033[97m"
);
renderer_render(r);
```

```python
# Python
ui.desenhar_caixa(r,
    x=2, y=2, altura=8, largura=40,
    titulo="Status",
    conteudo="HP: 100/100\nMP: 50/50\nGold: 320",
    ascii_art=False,
    cor_fundo="\033[40m",
    cor_borda="\033[33m",
    cor_texto="\033[97m",
)
r.renderizar()
```

Desenha uma caixa com **borda dupla** (╔═╗║╚╝), título opcional no topo e conteúdo com quebra de linha automática.

| Parâmetro C      | Parâmetro Python  | Descrição |
|------------------|-------------------|-----------|
| `x, y`           | `x, y`            | Posição do canto superior esquerdo |
| `height`         | `altura`          | Linhas de conteúdo (sem bordas) |
| `width`          | `largura`         | Colunas internas (sem bordas) |
| `title`          | `titulo`          | Texto no topo da borda. `NULL`/`None` = sem título |
| `content`        | `conteudo`        | Texto interno. `""` = caixa vazia |
| `ascii_art`      | `ascii_art`       | `true`/`True` = respeita `\n` sem word-wrap |
| `bg_color`       | `cor_fundo`       | Cor de fundo ANSI. `NULL`/`None` = sem fundo |
| `border_color`   | `cor_borda`       | Cor da borda ANSI |
| `text_color`     | `cor_texto`       | Cor do texto ANSI |

**Exemplo com arte ASCII (Python):**
```python
ui.desenhar_caixa(r,
    x=5, y=1, altura=6, largura=20,
    conteudo="  /\\_/\\\n ( o.o )\n  > ^ <",
    ascii_art=True,
    cor_fundo="\033[40m", cor_borda="\033[35m", cor_texto="\033[97m",
)
r.renderizar()
```

### Caixa com texto animado (efeito máquina de escrever)

```c
// C
interface_drawspeak(ui, r, 2, 2, 4, 40,
    "NPC",
    "Olá, aventureiro! Bem-vindo à vila de Pedra Alta.",
    "\033[40m", "\033[36m", "\033[97m",
    0.04f
);
```

```python
# Python
ui.desenhar_caixa_animada(r,
    x=2, y=2, altura=4, largura=40,
    titulo="NPC",
    texto="Olá, aventureiro! Bem-vindo à vila de Pedra Alta.",
    cor_fundo="\033[40m", cor_borda="\033[36m", cor_texto="\033[97m",
    velocidade=0.04,
)
```

Exibe o texto caractere por caractere. Quando excede `altura` linhas, aguarda tecla para a próxima página. Pressionar qualquer tecla pula a animação da página atual.

| Parâmetro C | Parâmetro Python | Descrição |
|-------------|-----------------|-----------|
| `speed`     | `velocidade`    | Segundos por caractere. `0.03` = rápido, `0.1` = lento, `0.0` = imediato |

### Caixa com linhas pré-formatadas

```c
// C
interface_drawline(ui, r, 2, 2, 4, 30,
    "Inventário",
    "Espada      x1\nPoção       x3\nChave       x1\nGold        320",
    "\033[40m", "\033[37m", "\033[97m",
    "single"
);
renderer_render(r);
```

```python
# Python
ui.desenhar_linhas(r,
    x=2, y=2, altura=4, largura=30,
    titulo="Inventário",
    linhas="Espada      x1\nPoção       x3\nChave       x1\nGold        320",
    cor_fundo="\033[40m", cor_borda="\033[37m", cor_texto="\033[97m",
    estilo_borda="single",
)
r.renderizar()
```

Respeita sempre os `\n` do texto (sem word-wrap). Escolha o estilo da borda:

| Valor           | Estilo   | Caracteres |
|-----------------|----------|------------|
| `"single"`      | Simples  | ┌─┐│└┘     |
| Qualquer outro  | Dupla    | ╔═╗║╚╝     |

### Texto animado sem caixa

```c
// C
interface_text_speak(ui, r, 5, 10,
    "Carregando...\nPor favor aguarde.",
    "\033[40m", "\033[93m", 0.05f
);
```

```python
# Python
ui.texto_animado(r,
    x=5, y=10,
    texto="Carregando...\nPor favor aguarde.",
    cor_fundo="\033[40m", cor_texto="\033[93m",
    velocidade=0.05,
)
```

Exibe texto animado diretamente na tela, sem caixa. `\n` avança para a linha seguinte mantendo a coluna `x`. Pressionar qualquer tecla pula o restante.

### Texto estático posicionado

```c
// C
interface_text_(ui, r, 3, 15,
    "Pressione ENTER para continuar\nou ESC para sair.",
    "\033[90m", ""
);
renderer_render(r);
```

```python
# Python
ui.texto(r,
    x=3, y=15,
    texto="Pressione ENTER para continuar\nou ESC para sair.",
    cor_texto="\033[90m",
)
r.renderizar()
```

Renderiza texto multilinha na posição `(x, y)` sem animação. Cada `\n` avança uma linha mantendo a coluna `x`.

---

## Inputs / Entradas

O módulo `Inputs` / `Entradas` gerencia a leitura do teclado: campo de texto livre, menus verticais e horizontais, e leitura de teclas não-bloqueante.

### Criação

```c
// C
Inputs* in = inputs_create();
inputs_destroy(in);
```

```python
# Python
en = tui.Entradas()
```

### Campo de texto

```c
// C — retorna char* alocado: chame free() depois
char* nome = inputs_prompt(in, r, 22, 5, 20, "\033[93m");
// usa o nome...
free(nome);
```

```python
# Python — retorna str Python, sem precisar de free()
nome = en.prompt(r, x=22, y=5, tamanho_maximo=20, cor_entrada="\033[93m")
```

Exibe um cursor na posição `(x, y)` e aguarda o usuário digitar. Suporta backspace e UTF-8. Confirmado com Enter.

| Parâmetro C   | Parâmetro Python  | Descrição |
|---------------|-------------------|-----------|
| `max_len`     | `tamanho_maximo`  | Máx. de caracteres visíveis. `0` = 255 |
| `input_color` | `cor_entrada`     | Cor do texto digitado |

> **C:** A string retornada é alocada com `malloc`. Você **deve** chamar `free()`.
> **Python:** Retorna uma `str` normal, sem necessidade de liberação manual.

### Menu vertical

```c
// C
char* opcoes[] = { "Nova Partida", "Carregar", "Opções", "Sair" };
int escolha = inputs_menu_selector_vertical(in, r,
    10, 5, opcoes, 4,
    "\033[40m", "\033[90m",
    "\033[44m", "\033[97m",
    "\033[42m", "\033[30m"
);
```

```python
# Python
opcoes = ["Nova Partida", "Carregar", "Opções", "Sair"]
escolha = en.menu_vertical(r,
    x=10, y=5,
    opcoes=opcoes,
    fundo_normal="\033[40m",      texto_normal="\033[90m",
    fundo_selecionado="\033[44m", texto_selecionado="\033[97m",
    fundo_confirmado="\033[42m",  texto_confirmado="\033[30m",
)

if escolha == 3:
    pass  # Sair
```

Navega com ↑↓. Retorna o índice da opção confirmada com Enter, ou `-1` se ESC.

| Parâmetros C               | Parâmetros Python                          | Descrição |
|----------------------------|--------------------------------------------|-----------|
| `bg_normal / fg_normal`    | `fundo_normal / texto_normal`              | Cores dos itens não selecionados |
| `bg_select / fg_select`    | `fundo_selecionado / texto_selecionado`    | Cores do item em destaque |
| `bg_correct / fg_correct`  | `fundo_confirmado / texto_confirmado`      | Cores ao confirmar (feedback visual) |

### Menu horizontal

```c
// C
char* sim_nao[] = { "Sim", "Não" };
int resp = inputs_menu_selector_horizontal(in, r,
    15, 10, sim_nao, 2,
    "\033[40m", "\033[90m",
    "\033[41m", "\033[97m",
    "\033[42m", "\033[30m"
);
```

```python
# Python
resp = en.menu_horizontal(r,
    x=15, y=10,
    opcoes=["Sim", "Não"],
    fundo_normal="\033[40m",      texto_normal="\033[90m",
    fundo_selecionado="\033[41m", texto_selecionado="\033[97m",
    fundo_confirmado="\033[42m",  texto_confirmado="\033[30m",
)
```

Igual ao menu vertical, mas as opções ficam lado a lado e a navegação é com ←→.

### Leitura de tecla sem bloqueio

```c
// C — ideal para game loops
const char* tecla = inputs_get_key();
if (strcmp(tecla, "UP") == 0)  { /* move para cima */ }
if (strcmp(tecla, "ESC") == 0) { /* sair */ }
```

```python
# Python — ideal para game loops
tecla = tui.Entradas.obter_tecla()
if tecla == "UP":   ...  # move para cima
if tecla == "ESC":  ...  # sair
```

**Não bloqueia** — retorna imediatamente `""` se nenhuma tecla estiver disponível.

| Retorno       | Tecla            |
|---------------|------------------|
| `"UP"`        | Seta para cima   |
| `"DOWN"`      | Seta para baixo  |
| `"LEFT"`      | Seta para esquerda |
| `"RIGHT"`     | Seta para direita |
| `"ENTER"`     | Enter            |
| `"BACKSPACE"` | Backspace        |
| `"ESC"`       | Escape           |
| `"TAB"`       | Tab              |
| `"SPACE"`     | Espaço           |
| `"a"` … `"z"`| Letra correspondente |
| `""`          | Nenhuma tecla    |

**Exemplo — game loop em Python:**
```python
import tui, time

r  = tui.Renderizador()
ui = tui.Interface()

while True:
    tecla = tui.Entradas.obter_tecla()

    if tecla == "UP":    ...  # lógica de movimento
    if tecla == "DOWN":  ...
    if tecla == "ESC":   break

    # redesenha a cena
    r.renderizar()
    time.sleep(0.016)   # ~60 fps
```

---

## Cores

A TUI Guri usa sequências ANSI 256 cores, geradas a partir de valores hexadecimais `#RRGGBB`. Isso permite usar qualquer cor sem memorizar códigos ANSI.

### Versões com buffer estático (mais convenientes)

```c
// C
renderer_add(r, color_fg_s("#00FF88"));  // verde neon (foreground)
renderer_add(r, color_bg_s("#0D0D0D"));  // quase preto (background)
renderer_add(r, "Texto estiloso");
renderer_add(r, C_RESET);
```

```python
# Python
r.adicionar(tui.sequencia_frente("#00FF88"))   # foreground
r.adicionar(tui.sequencia_fundo("#0D0D0D"))    # background
r.adicionar("Texto estiloso")
r.adicionar("\033[0m")
```

### Gravar em buffer externo (C apenas)

```c
char fg[COLOR_STR_SIZE];
char bg[COLOR_STR_SIZE];
color_fg(fg, "#FF6600");
color_bg(bg, "#1A1A2E");

renderer_add(r, fg);
renderer_add(r, bg);
renderer_add(r, "Texto colorido");
renderer_add(r, C_RESET);
```

### Obter índice ANSI 256

```c
// C
int id = color_hex_to_ansi_id("#FF0000");  // ~196 (vermelho puro)
char seq[32];
sprintf(seq, "\033[38;5;%dm", id);
```

```python
# Python
id = tui.cor_hex_para_id_ansi("#FF0000")   # ~196 (vermelho puro)
seq = f"\033[38;5;{id}m"
```

### Constantes prontas (C)

```c
C_RESET   // "\033[0m"  — reseta todas as cores e estilos
C_BOLD    // "\033[1m"  — texto em negrito
```

No Python, use as strings diretamente:
```python
RESET = "\033[0m"
NEGRITO = "\033[1m"
```

---

## Constantes e Teclas

### Códigos de tecla unificados (C)

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
// C
SLEEP_MS(16);    // ~60 fps
SLEEP_MS(33);    // ~30 fps
SLEEP_MS(1000);  // 1 segundo
```

```python
# Python — use time.sleep()
import time
time.sleep(0.016)   # ~60 fps
time.sleep(0.033)   # ~30 fps
time.sleep(1.0)     # 1 segundo
```

---

## Referência rápida — C

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
| `interface_move_cursor(r, y, x)` | Posiciona cursor |
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

---

## Referência rápida — Python

### Renderizador

| Método / Função | O que faz |
|-----------------|-----------|
| `tui.Renderizador()` | Cria o renderizador e ativa raw mode |
| `r.adicionar(str)` | Adiciona string ao buffer |
| `r.adicionar_bruto(bytes)` | Adiciona bytes brutos ao buffer |
| `r.mover_cursor(linha, coluna)` | Posiciona o cursor |
| `r.renderizar()` | Envia o buffer para o terminal |
| `r.limpar_tudo()` | Limpa toda a tela |

### Interface

| Método | O que faz |
|--------|-----------|
| `tui.Interface()` | Cria o módulo de interface |
| `ui.comprimento_visivel(str)` | Conta caracteres visíveis (ignora ANSI) |
| `ui.mover_cursor(r, linha, col)` | Posiciona cursor |
| `ui.limpar(r, x, y, altura, largura, cor_fundo)` | Limpa uma região da tela |
| `ui.desenhar_caixa(r, ...)` | Caixa com borda dupla e texto |
| `ui.desenhar_caixa_animada(r, ...)` | Caixa com texto animado + paginação |
| `ui.desenhar_linhas(r, ...)` | Caixa com linhas exatas, borda single/dupla |
| `ui.texto_animado(r, ...)` | Texto animado sem caixa |
| `ui.texto(r, ...)` | Texto estático posicionado |

### Entradas

| Método | O que faz |
|--------|-----------|
| `tui.Entradas()` | Cria o módulo de entradas |
| `en.prompt(r, x, y, ...)` | Campo de texto. Retorna `str` |
| `en.menu_vertical(r, x, y, opcoes, ...)` | Menu com setas ↑↓. Retorna índice ou `-1` |
| `en.menu_horizontal(r, x, y, opcoes, ...)` | Menu com setas ←→. Retorna índice ou `-1` |
| `tui.Entradas.obter_tecla()` | Lê tecla sem bloquear. Retorna `str` do nome |

### Cores

| Função | O que faz |
|--------|-----------|
| `tui.sequencia_frente("#RRGGBB")` | Retorna sequência ANSI de foreground |
| `tui.sequencia_fundo("#RRGGBB")` | Retorna sequência ANSI de background |
| `tui.cor_hex_para_id_ansi("#RRGGBB")` | Retorna índice ANSI 256 como `int` |
| `tui.cor_frente(bytearray, "#RRGGBB")` | Grava sequência foreground em bytearray |
| `tui.cor_fundo(bytearray, "#RRGGBB")` | Grava sequência background em bytearray |
