#ifndef BOARD_GENERATOR_H
#define BOARD_GENERATOR_H

#define LENGTH 4
#define SIZE 16
#define CHOICES 6
#define MAX_LINE 100

char** construct_board();
void free_board(char **board);
void display_board(char **board);
void ncontent(char **board, char *content);
#endif