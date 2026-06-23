#ifndef PACMAN
#define PACMAN

#define MAP_SIZE 40

#define TREASURE_MAX 2

extern int t1;  //txt
extern int t2;  //jpg
extern int t3;  //mp4

// mostra a tela inicial do jogo
void print_title(int lines, int cols);
// mostra game clear
void print_gameclear(int lines, int cols);
// mostra game over
void print_gameover(int lines, int cols);
// mostra as informacoes do jogo
void game_info(int x, int y, int t1, int t2, int t3, int life);
// mostra a tela do jogo
void pacman_game(int lines, int cols, int socket);

#endif