#ifndef _INTERCODES_H
#define _INTERCODES_H

#include "syntaxtree.h"
#include <stdio.h>

void intercodes_translate(treenode_t *root);
void print_intercodes(FILE *fp);

#endif