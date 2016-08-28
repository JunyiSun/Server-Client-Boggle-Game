#ifndef WORD_CHECKER_H
#define WORD_CHECKER_H

#include "dictionary.h"

int exists_path(char *word, char **board);
int exists_helper(char *word, char **board, int x, int y);


int check_word_valid(char *word, DNode **appeared_words, DNode **dictionary, char **board);
#endif