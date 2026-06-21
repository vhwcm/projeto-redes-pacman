#include <string.h>
#include <ncurses.h>
#include "Pacman.h"
#include "Client_socket.h"

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
            (cols - strlen(titulo[i])) / 2,
            "%s", titulo[i]);
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
    int t1 = 0, t2 = 0, t3 = 0, // treasures 
        life_p = LIFE_DEFAULT,
        light = 1, counter = 0;;    // luz do pacman

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
    }while(Receber_d_servidor(socket, game_map) <= 0);


    int pacman_x, pacman_y;
    int ch;

    while(1){
        // quadro de info
        game_info(x, y, t1, t2, t3, life_p);

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

        // aceita o input do usuário
        keypad(stdscr, TRUE);
        ch = getch();
        keypad(stdscr, FALSE);  // desativa o teclado

        // se ch != F1, enviar a seta e atualizar o mapa
        switch(ch){
            case KEY_UP:
                do{
                    Enviar_p_servidor(socket, CIMA, 0);
                }while(Receber_d_servidor(socket, game_map) <= 0);
                break;
            case KEY_DOWN:
                do{
                    Enviar_p_servidor(socket, BAIXO, 0);
                }while(Receber_d_servidor(socket, game_map) <= 0);
                break;
            case KEY_LEFT:
                do{
                    Enviar_p_servidor(socket, ESQUERDA, 0);
                }while(Receber_d_servidor(socket, game_map) <= 0);
                break;
            case KEY_RIGHT:
                do{
                    Enviar_p_servidor(socket, DIREITA, 0);
                }while(Receber_d_servidor(socket, game_map) <= 0);
                break;
            case KEY_F(1):  
                // avisar o servidor o fim do jogo e espera ACK
                do{
                    Enviar_p_servidor(socket, FIM_TRANSMISSAO, 0);
                }while(Receber_d_servidor(socket, game_map) <= 0);
                return; // sair do jogo
        }

        // calcula a luz do pacman
        if(counter == 5 - 1 && light < 5){
            light++;
            counter = 0;
        }
        else{
            counter++;
        }

        clear(); //limpa a tela uma vez para atualizar ela
    }
}