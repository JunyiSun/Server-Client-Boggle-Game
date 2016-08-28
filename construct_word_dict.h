#ifndef CONSTRUCT_WORD_LIST_H
#define CONSTRUCT_WORD_LIST_H

#include "dictionary.h"

DNode ** construct_dictionary_with_path(char *path);
DNode ** construct_empty_dictionary();

void strupr(char **str_ptr);

#endif