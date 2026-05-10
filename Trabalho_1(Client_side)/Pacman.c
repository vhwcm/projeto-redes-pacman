#include <stdio.h>

#include <allegro5/allegro5.h>
#include <allegro5/allegro_primitives.h>

#include "Pacman.h"
#include "Client_socket.h"

// cria a base do jogo
char* game_table(){
    char *table;

    if(!(table = malloc(sizeof(char)*MAP_SIZE*MAP_SIZE))){
        printf("ERROR: malloc table\n");
        return NULL;
    }

    for (int i = 0; i < MAP_SIZE; i++)
    {
        for (int j = 0; j < MAP_SIZE; j++)
        {
            table[MAP_SIZE*i + j] = LABIRINTO[i][j];
        }
    }
    
    return table;
}

character* create_character(int socket){
    character* list;
    if(!(list = malloc(sizeof(character)))){
        printf("ERROR: create character\n");
        return NULL;
    }

    //recv()    //recebe os dados do servidor
    
    return list;
}

//printa a tabela base
void print_table(char* table){
    for(int j = 0; j < MAP_SIZE; j++){
        for(int i = 0; i < MAP_SIZE; i++){
            if(table[MAP_SIZE*i + j] == 'X')
                al_draw_filled_rectangle(SQUARE*j, SQUARE*i, SQUARE*j + SQUARE, SQUARE*i+SQUARE, al_map_rgb(255, 255, 255));
        }
    }
    
}

//printa os personagens/items
void print_ch(character* list){
    //pac
    al_draw_filled_circle(SQUARE*list->Pac[0] + (SQUARE/2), SQUARE*list->Pac[1] * (SQUARE/2), 10,  al_map_rgb(255, 255, 0));

    //red
    al_draw_filled_rectangle(SQUARE*list->Red[1], SQUARE*list->Red[0], SQUARE*list->Red[1] + SQUARE, SQUARE*list->Red[0]+SQUARE, al_map_rgb(255, 0, 0));

    //blue
    al_draw_filled_rectangle(SQUARE*list->Blue[1], SQUARE*list->Blue[0], SQUARE*list->Blue[1] + SQUARE, SQUARE*list->Blue[0]+SQUARE, al_map_rgb(255, 0, 0));

    //green
    al_draw_filled_rectangle(SQUARE*list->Green[1], SQUARE*list->Green[0], SQUARE*list->Green[1] + SQUARE, SQUARE*list->Green[0]+SQUARE, al_map_rgb(255, 0, 0));

    //yellow
    al_draw_filled_rectangle(SQUARE*list->Yellow[1], SQUARE*list->Yellow[0], SQUARE*list->Yellow[1] + SQUARE, SQUARE*list->Yellow[0]+SQUARE, al_map_rgb(255, 0, 0));

    //item1

    //item2

    //item3

    //item4

    //item5

    //item6
}

// roda o jogo
void pacman(char* table, character *list){
    al_init();
    al_init_primitives_addon();
    //al_install_keyboard();

    // abre a tela do jogo
    ALLEGRO_DISPLAY* disp = al_create_display(X_SCREEN, Y_SCREEN);

    // printa os dados na tela do jogo
    while(1){
        
        print_table(table);
        print_ch(list);

        al_flip_display();

        //receber os dados do teclado

        //caso receber o fim do jogo; break;
    }

    al_destroy_display(disp);
}