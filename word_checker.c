#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "dictionary.h"
#include "word_checker.h"
int mark[16][16] = {0};

// if exist, return 1, else return 0
int exists_path(char *word, char **board) {

	int i, j;
	// check if empty
	if (strlen(word) == 0)
		return 1;

	for (i = 0; i < 4; i++)
		for (j = 0; j < 4; j++)
			if (exists_helper(word, board, i, j))
				return 1;

	return 0;
}

int exists_helper(char *word, char **board, int x, int y) {

	// below is essentially the recursive implementation of DFS

	if (strlen(word) == 0)  // if empty string, the search is successful
		return 1;
	if (x < 0 || x > 3 || y < 0 || y > 3)  // invalid x,y coordinates
		return 0;
	if (mark[x][y] == 1)  // we cannot repeat this block if already used
		return 0;
	if (word[0] != board[x][y])  // if current letter on board doesn't match
		return 0;

	char *rem = word + 1;  // remaining word without first letter
	int result = 0;
	mark[x][y] = 1;  // denote that we've used this block in search
	result |= exists_helper(rem, board, x, y - 1);
	result |= exists_helper(rem, board, x, y + 1);
	result |= exists_helper(rem, board, x + 1, y - 1);
	result |= exists_helper(rem, board, x + 1, y);
	result |= exists_helper(rem, board, x + 1, y + 1);
	result |= exists_helper(rem, board, x - 1, y);
	result |= exists_helper(rem, board, x - 1, y - 1);
	result |= exists_helper(rem, board, x - 1, y + 1);
	mark[x][y] = 0;  // clear the mark for future use
	return result;
}

int check_word_valid(char *word, DNode **appeared_words, DNode **dictionary, char **board) {
	return (lookup(appeared_words, SMALL_HASH_SIZE, word) == NULL
	& lookup(dictionary, BIG_HASH_SIZE, word) != NULL
	& exists_path(word, board));
}
