#include "dictionary.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#define MAX_LINE 100

void strupr(char **str_ptr) {
	int i;
	for (i = 0; i < strlen(*str_ptr); i++)
		(*str_ptr)[i] = toupper((*str_ptr)[i]);
}

DNode **construct_dictionary_with_path(char *path) {

	DNode** dictionary = malloc(sizeof(DNode*) * BIG_HASH_SIZE);
	int i;
	for (i = 0; i < BIG_HASH_SIZE; i++)
		dictionary[i] = NULL;
	FILE *in;
	char *line = (char *) malloc(MAX_LINE);

	if(!(in = fopen(path, "r")))    {
        fprintf(stderr, "Invalid Path %s ", path);
        return NULL;
    }

	while(fgets(line, MAX_LINE, in)!=NULL ) {
		line[strcspn(line, "\r\n")] = '\0';  //trim new line characters
		strupr(&line);
		insert (dictionary, BIG_HASH_SIZE, line);
		
	}

	free(line);
	fclose(in);
	return dictionary;
}

DNode **construct_empty_dictionary() {
	DNode** dictionary = malloc(sizeof(DNode*) * SMALL_HASH_SIZE);
	int i;
	for (i = 0; i < SMALL_HASH_SIZE; i++)
		dictionary[i] = NULL;
	return dictionary;
}