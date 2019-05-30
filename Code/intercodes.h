#ifndef _INTERCODES_H
#define _INTERCODES_H

#include "syntaxtree.h"
#include "intercode.h"

#include <stdio.h>

void intercodes_translate(treenode_t *root);
int has_translate_error();

void fprint_intercodes(FILE *fp);
iclist_t *get_intercodes();

#endif