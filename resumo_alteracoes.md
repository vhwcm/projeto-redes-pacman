# Macro: Refatoração de Arquitetura (Remoção do Ncurses)

O sistema de renderização e controle de entrada do cliente Pacman foi inteiramente reescrito para utilizar as chamadas nativas (escape codes e I/O padrão do POSIX). A biblioteca **Ncurses foi removida**, deixando o código mais limpo e independente de bibliotecas de terceiros complexas.

## Entendendo as Alterações e Impactos

### 1. Build Simplificado (Makefile)
**O que mudou:** Removidas flags `-lncurses -ltinfo`.
**Impacto:** O programa do cliente agora compila sem necessitar da instalação ou link da biblioteca ncurses pelo linker, facilitando a execução em qualquer sistema UNIX nativo.

### 2. Leitura de Teclas e Terminal (Client_main.c)
**O que mudou:** A estrutura que chamava `initscr()`, `cbreak()`, e manipulava o terminal via ncurses foi limpa. No seu lugar, usamos a estrutura `winsize` e `ioctl` (através de `TIOCGWINSZ`) para descobrir dinamicamente os valores de limites de tela (variáveis `LINES` e `COLS`).
**Impacto:** O cliente assume que a tela já existe, limpa usando Escape ANSI e constrói o tamanho base no momento em que roda sem sequestrar a tela.

### 3. Engine de Interface (Pacman.c e Pacman.h)
**O que mudou:** Essa foi a maior mudança. Para que o jogo não precisasse ter cada linha de desenho modificada, foi utilizado um mecanismo de **Macros de Compatibilidade**.
* Onde o jogo chamava cores (`COLOR_PAIR`, `attron`, `attroff`), definimos funções `static void` que traduzem e cospem as cores usando ANSI na saída padrão (`\x1b[31m`).
* Onde o jogo movia a tela com `mvprintw` (do ncurses), um macro substituiu por um `printf("\x1b[%d;%dH...", ...)` (que instrui o terminal a mover o cursor usando ANSI e imprimir logo depois).

### 4. Input Assíncrono (Teclado em Tempo Real)
**O que mudou:** Adicionada a função `my_getch()` baseada na biblioteca nativa `termios`.
**Impacto:** O `getch()` clássico em C faz com que a pessoa tenha que apertar `Enter`. O ncurses cuidava disso silenciosamente. Agora nós cuidamos: o programa instrui o terminal (`tcsetattr`) para o modo não-canônico silenciosamente, lê um byte, e depois volta. Caso o jogador utilize as Setas (que enviam múltiplos códigos e não apenas 1), a função rastreia e traduz para as letras `WASD` antes de devolver pro loop. 

> Com este fluxo inteiramente repaginado, o Client envia os dados TCP/IP baseando-se apenas na saída padronizada do OS Linux.
