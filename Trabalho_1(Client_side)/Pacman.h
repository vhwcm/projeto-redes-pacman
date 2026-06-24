#ifndef PACMAN
#define PACMAN

#define MAP_SIZE 40

#define TREASURE_MAX 2

extern int t1;  //txt
extern int t2;  //jpg
extern int t3;  //mp4
extern int pacman_life;

void print_title(int lines, int cols);
void print_gameclear(int lines, int cols);
void print_gameover(int lines, int cols);
void game_info(int x, int y, int t1, int t2, int t3, int life);
void pacman_game(int lines, int cols, int socket);
void print_game_map_raw(char game_map[MAP_SIZE * MAP_SIZE]);

#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_BLUE    "\x1b[34m"
#define ANSI_COLOR_WHITE   "\x1b[37m"
#define ANSI_COLOR_RESET   "\x1b[0m"

int my_getch(void);
void clear_screen(void);

#endif