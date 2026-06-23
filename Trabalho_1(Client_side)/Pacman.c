#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>
#include <sys/select.h>
#include "Pacman.h"
#include "Client_socket.h"

#define A_BOLD (1<<8)
#define COLOR_PAIR(x) (x)
#define stdscr NULL
#define keypad(...)
#define noecho(...)
#define init_pair(...)

#define KEY_UP 'w'
#define KEY_DOWN 's'
#define KEY_LEFT 'a'
#define KEY_RIGHT 'd'
#define KEY_F(n) 'q'

static const char* get_color(int id) {
    switch(id) {
        case 1: return ANSI_COLOR_RED;
        case 2: return ANSI_COLOR_YELLOW; 
        case 3: return ANSI_COLOR_GREEN;
        case 4: return ANSI_COLOR_BLUE;
        case 5: return ANSI_COLOR_YELLOW;
        case 6: return ANSI_COLOR_RED;
        case 7: return ANSI_COLOR_GREEN;
        case 8: return ANSI_COLOR_BLUE;
        case 9: return ANSI_COLOR_WHITE;
        case 10: return ANSI_COLOR_RESET; 
        default: return ANSI_COLOR_RESET;
    }
}

static void attron(int attr) {
    int color_id = attr & 0xFF;
    int is_bold = (attr & A_BOLD) != 0;
    if (is_bold) printf("\x1b[1m");
    printf("%s", get_color(color_id));
}

static void attroff(int attr) {
    (void)attr;
    printf("\x1b[0m");
}

#define mvprintw(r, c, fmt, ...) printf("\x1b[%d;%dH" fmt, (r)+1, (c)+1, ##__VA_ARGS__)
#define mvaddch(r, c, ch) printf("\x1b[%d;%dH%c", (r)+1, (c)+1, ch)
#define printw(...) printf(__VA_ARGS__)
#define refresh() fflush(stdout)

void clear_screen(void) {
    printf("\033[H\033[2J");
    fflush(stdout);
}

static void flushinp(void) {
    struct timeval tv = {0, 0};
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(STDIN_FILENO, &fds);
    while (select(STDIN_FILENO + 1, &fds, NULL, NULL, &tv) > 0) {
        getchar();
    }
}

int my_getch(void) {
    struct termios oldt, newt;
    int ch;
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    ch = getchar();
    if (ch == 27) {
        struct timeval tv = {0, 50000};
        fd_set fds;
        FD_ZERO(&fds);
        FD_SET(STDIN_FILENO, &fds);
        if (select(STDIN_FILENO + 1, &fds, NULL, NULL, &tv) > 0) {
            int ch2 = getchar();
            if (ch2 == '[') {
                FD_ZERO(&fds);
                FD_SET(STDIN_FILENO, &fds);
                if (select(STDIN_FILENO + 1, &fds, NULL, NULL, &tv) > 0) {
                    int ch3 = getchar();
                    switch(ch3) {
                        case 'A': ch = 'w'; break;
                        case 'B': ch = 's'; break;
                        case 'C': ch = 'd'; break;
                        case 'D': ch = 'a'; break;
                    }
                }
            }
        }
    }
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    if (ch >= 'A' && ch <= 'Z') ch += 32;
    return ch;
}

int pacman_life;
int t1 = 0; //txt
int t2 = 0; //jpg
int t3 = 0; //mp4
int seq_send = 0;

void print_title(int lines, int cols) {
    char* inicio = "Digite algo para iniciar...";

    char *titulo[] = {
        "######    ###     ######     ##   ##    ###    ##   ##",
        "##   ##  ## ##   ##          ### ###   ## ##   ###  ##",
        "######  #######  ##          ## # ##  #######  ## # ##",
        "##      ##   ##  ##          ##   ##  ##   ##  ##  ###",
        "##      ##   ##   ######     ##   ##  ##   ##  ##   ##"
    };
  
    int x = lines / 2;
    int y = (cols - strlen(inicio)) / 2;

    mvprintw(x + 5, y, "%s", inicio);
    mvprintw(x + 22, y + 80, "MADE BY: MAURICIO & VIKTOR");

    init_pair(2, COLOR_YELLOW, COLOR_BLACK);
    attron(COLOR_PAIR(2)|A_BOLD);

    for(int i = 0; i < 5; i++)
    {
        mvprintw(x - 5 + i,
            (int)((cols - strlen(titulo[i])) / 2),
            "%s", titulo[i]);
    }
    attroff(COLOR_PAIR(2)|A_BOLD);
    refresh();
}

void print_gameclear(int lines, int cols){
    char *g[] = {
    " ######    ###    ##   ## ######",
    "##        ## ##   ### ### ##    ",
    "##  #### #######  ## # ## ######",
    "##   ##  ##   ##  ##   ## ##    ",
    " ######  ##   ##  ##   ## ######"
    };

    char *c[]= {
    "###### ##      #######    ###    ######",
    "##     ##      ##       ## ##   ##   ##",
    "##     ##      ######  #######  ###### ",
    "##     ##      ##      ##   ##  ## ##  ",
    "###### ####### ####### ##   ##  ##  ###"
    };

    int x = lines / 2;
    int y = (cols) / 2;

    attron(COLOR_PAIR(2)|A_BOLD);
    for(int i = 0; i < 5; i++){
        mvprintw(x - 10 + i,
            y + 25,
            "%s", g[i]);

        mvprintw(x - 3 + i,
            y + 21,
            "%s", c[i]);
    }
    attroff(COLOR_PAIR(2)|A_BOLD);
    refresh();
}

void print_gameover(int lines, int cols){
    char *g[] = {
    " ######    ###    ##   ## ######",
    "##        ## ##   ### ### ##    ",
    "##  #### #######  ## # ## ######",
    "##   ##  ##   ##  ##   ## ##    ",
    " ######  ##   ##  ##   ## ######"
    };
    
    char *o[] = {
    "####### ##   ## ####### ###### ",
    "##   ## ##   ## ##      ##   ##",
    "##   ## ##   ## ######  ###### ",
    "##   ##  ## ##  ##      ## ##  ",
    "#######   ###   ####### ##  ###"
    };

    int x = lines / 2;
    int y = (cols) / 2;

    attron(COLOR_PAIR(2)|A_BOLD);
    for(int i = 0; i < 5; i++){
        mvprintw(x - 10 + i,
            y + 25,
            "%s", g[i]);

        mvprintw(x - 3 + i,
            y + 25,
            "%s", o[i]);
    }
    attroff(COLOR_PAIR(2)|A_BOLD);
    refresh();
}

void game_info(int x, int y, int t1, int t2, int t3, int life){
    attron(COLOR_PAIR(9)|A_BOLD);
    mvprintw(x - 21, y - 40, "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA");
    mvprintw(x + 20, y - 40, "VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV");

    for(int i = 0; i < MAP_SIZE; i++) {
        mvprintw(x - 20 + i, y - 40, "{");
        mvprintw(x - 20 + i, y + 1, "}");
    }
    attroff(COLOR_PAIR(9)|A_BOLD);

    mvprintw(x - 19, y - 25, "GAME INFO");
    attron(COLOR_PAIR(2)|A_BOLD);
    mvprintw(x - 15, y - 35, "C ");
    attroff(COLOR_PAIR(2)|A_BOLD);
    printw(": Player");

    attron(COLOR_PAIR(3)|A_BOLD);
    mvprintw(x - 13, y - 35, "A ");
    attroff(COLOR_PAIR(3)|A_BOLD);
    printw(": Green Ghost");

    attron(COLOR_PAIR(4)|A_BOLD);
    mvprintw(x - 11, y - 35, "A ");
    attroff(COLOR_PAIR(4)|A_BOLD);
    printw(": Blue Ghost");

    attron(COLOR_PAIR(1)|A_BOLD);
    mvprintw(x - 9, y - 35, "A ");
    attroff(COLOR_PAIR(1)|A_BOLD);
    printw(": Red Ghost");

    attron(COLOR_PAIR(2)|A_BOLD);
    mvprintw(x - 7, y - 35, "A ");
    attroff(COLOR_PAIR(2)|A_BOLD);
    printw(": Yellow Ghost");

    attron(COLOR_PAIR(2)|A_BOLD);
    mvprintw(x - 5, y - 35, "# ");
    attroff(COLOR_PAIR(2)|A_BOLD);
    printw(": Treasure");

    mvprintw(x + 18, y - 35, "F1 : Sair do jogo");

    mvprintw(x + 8, y - 35, "TREASURE TYPE 1 : %d/%d",t1, TREASURE_MAX); // arquivo txt
    mvprintw(x + 10, y - 35, "TREASURE TYPE 1 : %d/%d",t2, TREASURE_MAX); // arquivo jpg
    mvprintw(x + 12, y - 35, "TREASURE TYPE 1 : %d/%d",t3, TREASURE_MAX); // arquivo mp4
    mvprintw(x + 14, y - 35, "LIFE : ");
    attron(COLOR_PAIR(2)|A_BOLD);
    for(int i = 0; i < life; i++){
        mvprintw(x + 14, y - 28 + i, "C");
    }
    attroff(COLOR_PAIR(2)|A_BOLD);
}

void pacman_game(int lines, int cols, int socket) {
    int  light = 1, counter = 0;;    // luz do pacman

    int x = lines / 2;
    int y = cols / 2;  

    init_pair(1, COLOR_RED,    COLOR_BLACK);    // enemy info
    init_pair(2, 226, COLOR_BLACK);             // pacman and enemy info
    init_pair(3, COLOR_GREEN,  COLOR_BLACK);    // enemy info
    init_pair(4, COLOR_BLUE,   COLOR_BLACK);    // enemy info
    
    init_pair(5, 226, 240);        // yellow in game
    init_pair(6, COLOR_RED, 240);    // red in game
    init_pair(7, COLOR_GREEN, 240);  // green in game
    init_pair(8, COLOR_BLUE, 240);   // blue in game

    init_pair(9, COLOR_WHITE,  COLOR_WHITE);    // paredes
    init_pair(10, COLOR_BLACK,  240);    // cor para limitar a visão do pacman

    keypad(stdscr, FALSE);
    noecho();
    
    char game_map[MAP_SIZE * MAP_SIZE];
    // receber o mapa do servidor 
    do{
        Enviar_p_servidor(socket, INICIALIZACAO, 0);
        printf("======================================================\n");
    }while(Receber_d_servidor(socket, game_map) <= 0);

    int pacman_x = 0, pacman_y = 0;
// receber o dado necessario para o jogo (life do pacman)
/*
    do{
        Enviar_p_servidor(socket, VIDA_PACMAN, 0);
    }while((pacman_life = Receber_d_servidor(socket) <= 0);
*/

    int ch, n = 1;

    while(1){
        // quadro de info
        game_info(x, y, t1, t2, t3, pacman_life);

        // desenha o quadro do jogo
        attron(COLOR_PAIR(9));
        mvprintw(x - 21, y + 20, "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA");
        mvprintw(x + 20, y + 20, "VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV");

        for(int i = 0; i < MAP_SIZE; i++) {
            mvprintw(x - 20 + i, y + 20, "{");
            mvprintw(x - 20 + i, y + 61, "}");
        }
        attroff(COLOR_PAIR(9));

        // desenha parte visivel do pacman
        for(int i = 0; i < MAP_SIZE; i++){
            for(int j = 0; j < MAP_SIZE; j++){
                if(game_map[i * MAP_SIZE + j] == 'P'){
                    // atualiza a posição do pacman
                    pacman_x = i;
                    pacman_y = j;
            
                    goto fim;
                }
            }
        }
        fim:

        for(int i = light * (-1); i <= light; i++){
            for(int j = light * (-1); j <= light; j++){
                if(x - 20 + pacman_x + i >= x - 20 && y + 21 + pacman_y + j >= y +21 &&
                x - 20 + pacman_x + i < x + 20 && y + 21 + pacman_y + j < y + 61){
                    if(game_map[(pacman_x + i) * MAP_SIZE + pacman_y + j] == 'X'){    // desenha as paredes
                        attron(COLOR_PAIR(9));
                        mvaddch(x - 20 + pacman_x + i, y + 21 + pacman_y + j, 'A');
                        attroff(COLOR_PAIR(9));
                    }
                    // desenha outros personagens do jogo
                    else if(game_map[(pacman_x + i) * MAP_SIZE + pacman_y + j] == 'C'){   // desenha o pacman
                        attron(COLOR_PAIR(5)|A_BOLD);
                        mvaddch(x - 20 + pacman_x + i, y + 21 + pacman_y + j, 'C');
                        attroff(COLOR_PAIR(5)|A_BOLD);
                    }
                    else if(game_map[(pacman_x + i) * MAP_SIZE +pacman_y + j] == 'G'){   // desenha os F_GREEN
                        attron(COLOR_PAIR(7)|A_BOLD);
                        mvaddch(x - 20 + pacman_x + i, y + 21 + pacman_y + j, 'A');
                        attroff(COLOR_PAIR(7)|A_BOLD);
                    }
                    else if(game_map[(pacman_x + i) * MAP_SIZE + pacman_y + j] == 'R'){   // desenha os F_RED
                        attron(COLOR_PAIR(6)|A_BOLD);
                        mvaddch(x - 20 + pacman_x + i, y + 21 + pacman_y + j, 'A');
                        attroff(COLOR_PAIR(6)|A_BOLD);
                    }
                    else if(game_map[(pacman_x + i) + MAP_SIZE + pacman_y + j] == 'B'){   // desenha os F_BLUE
                        attron(COLOR_PAIR(8)|A_BOLD);
                        mvaddch(x - 20 + pacman_x + i, y + 21 + pacman_y + j, 'A');
                        attroff(COLOR_PAIR(8)|A_BOLD);
                    }
                    else if(game_map[(pacman_x + i) * MAP_SIZE + pacman_y + j] == 'Y'){   // desenha os F_YELLOW
                        attron(COLOR_PAIR(5)|A_BOLD);
                        mvaddch(x - 20 + pacman_x + i, y + 21 + pacman_y + j, 'A');
                        attroff(COLOR_PAIR(5)|A_BOLD);
                    }
                    else if(game_map[(pacman_x + i) * MAP_SIZE + pacman_y + j] == 'T'){   // desenha os tesouros
                        attron(COLOR_PAIR(5)|A_BOLD);
                        mvaddch(x - 20 + pacman_x + i, y + 21 + pacman_y + j, '#');
                        attroff(COLOR_PAIR(5)|A_BOLD);
                    }
                    else{                                                   // desenha o espaço vazio vizivel
                        attron(COLOR_PAIR(10));
                        mvaddch(x - 20 + pacman_x + i, y + 21 + pacman_y + j, ' ');
                        attroff(COLOR_PAIR(10));
                    }
                }
            }
        }

        mvprintw(x*2, y*2, " ");
        refresh();
        keypad(stdscr, TRUE);
        flushinp();
        ch = my_getch();
        keypad(stdscr, FALSE);

        // se ch != F1, enviar a seta e atualizar o mapa
        switch(ch){
            case KEY_UP:
                do{
                    if(n <= 0)  seq_send--;
                    Enviar_p_servidor(socket, CIMA, seq_send);
                    seq_send++;
                }while((n = Receber_d_servidor(socket, game_map)) <= 0);
                break;
            case KEY_DOWN:
                do{
                    if(n <= 0) seq_send--;
                    Enviar_p_servidor(socket, BAIXO, seq_send);
                    seq_send++;
                }while((n = Receber_d_servidor(socket, game_map)) <= 0);
                break;
            case KEY_LEFT:
                do{
                    if(n <= 0) seq_send--;
                    Enviar_p_servidor(socket, ESQUERDA, seq_send);
                    seq_send++;
                }while((n = Receber_d_servidor(socket, game_map)) <= 0);
                break;
            case KEY_RIGHT:
                do{
                    if(n <= 0) seq_send--;
                    Enviar_p_servidor(socket, DIREITA, seq_send);
                    seq_send++;
                }while((n = Receber_d_servidor(socket, game_map)) <= 0);
                break;
            case KEY_F(1):  
                // avisar o servidor o fim do jogo e espera ACK
                do{
                    if(n <= 0) seq_send--;
                    Enviar_p_servidor(socket, FIM_TRANSMISSAO, seq_send);
                    seq_send++;
                }while((n = Receber_d_servidor(socket, game_map)) <= 0);
                return; // sair do jogo
        }

        // quando receber fim_transmissao do servidor
        if(n == 2){ return;}
        else if(n == 9){
            //printar game clear na tela
            print_gameclear(lines, cols);
            keypad(stdscr, TRUE);
            my_getch();
            break;
        }
        else if(n == 14){
            //printar game over na tela
            print_gameover(lines, cols);
            keypad(stdscr, TRUE);
            my_getch();
            break;
        }

        // calcula a luz do pacman
        if(counter == 5 - 1 && light < 5){
            light++;
            counter = 0;
        }
        else{
            counter++;
        }

        clear_screen(); //limpa a tela uma vez para atualizar ela
    }
}