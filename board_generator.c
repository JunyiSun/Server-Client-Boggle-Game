#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdio.h>
#include "board_generator.h"

char** construct_board() {
	
	char dice[SIZE][CHOICES] = {"RIFOBX", "IFEHEY", "DENOWS", "UTOKND", 
								"HMSRAO", "LUPETS", "ACITOA", "YLGKUE",
								"QBMJOA", "EHISPN", "VETIGN", "BALIYT", 
								"EZAVND", "RALESC", "UWILRG", "PACEMD"};
	// dynamically allocation
	char *board = (char *) malloc(sizeof(char) * (SIZE+1));
	int is_occupied[SIZE] = {0};
	int i;
	int counter = 0;
	srand(time(NULL));
	while (counter < SIZE) {
		int r = rand() % SIZE;
		while (is_occupied[r] == 1)
			r = rand() % SIZE;
		board[r] = dice[counter++][rand() % CHOICES];
		is_occupied[r] = 1;
	}
	// convert the char array to two dimensional
	char **two_dimensional_board = (char **) malloc(sizeof(char *) * LENGTH);
	counter = 0;
	while (counter < LENGTH) {
		two_dimensional_board[counter] = (char *) malloc(sizeof(char) * LENGTH);
		for (i = 0; i < LENGTH; i++)
			two_dimensional_board[counter][i] = board[LENGTH * counter + i];
		counter++;
	}
	free(board);  
	return two_dimensional_board;
}

void free_board(char **board) {
	int i;
	if (board != NULL) {
		for (i = 0; i < LENGTH; i++) {
			free(board[i]);
		}
	}
}

void display_board(char **board) {
	int i, j;
	for (i = 0; i < LENGTH; i++) {
		for (j = 0; j < LENGTH; j++) {
			fprintf (stdout, "%c ", board[i][j]);
		}
		fprintf (stdout, "\n");
	}
}

void ncontent(char **board, char *content) {
	int i, j;
	for (i = 0; i < LENGTH; i++) {
		for (j = 0; j < LENGTH; j++) {
			content += sprintf(content, "%c", board[i][j]);
		}
		content += sprintf(content, "\r\n");
	}
	*content = '\0';
}